#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Luckfox Pico Ultra 串口接收验证脚本 (纯接收)
参考: https://wiki.luckfox.com/zh/Luckfox-Pico-Ultra/UART

用途:
    验证 UART3 (/dev/ttyS3) 能否收到数据。参数默认与主程序 rs232.json 一致 (115200 8N1)。
    收到"一阵"数据就打印: 完整帧实时打印并解析字段, 零散/半帧数据打印原始 HEX。
    帧格式与主程序 DealSerialData 的解析保持一致: 0x55 | 电源类型 | 开关量x5 | CRC16x2 (9字节)。

用法:
    python3 uart_rx_monitor.py                        # /dev/ttyS3 @ 115200
    python3 uart_rx_monitor.py /dev/ttyS3 9600        # 指定端口和波特率
    python3 uart_rx_monitor.py --hex-only            # 只打印原始 HEX, 不做解析

运行前排查清单 (来自 Luckfox Wiki):
    1. 先停掉占用该串口的主程序 (CarClean), 否则两个进程抢数据都收不全。
    2. /dev/ttyS3 不存在 -> 板子上运行 luckfox-config -> Advanced Options -> UART 使能 UART3。
       (板子默认只开 UART2 调试串口, UART3 默认未启用)
    3. 权限不足 -> sudo chmod a+rw /dev/ttyS3
    4. 接线: 设备 GND <-> 板子 GND, 设备 TX <-> 板子 UART3_RX_M0 (纯接收可不接板子 TX)。
    5. 波特率必须与对端设备一致 (主程序用的是 115200)。
"""

import argparse
import datetime
import sys

import serial

# 主程序 WashReport 的串口帧协议 (见 src/WashReport.cpp DealSerialData):
#   帧头 0x55 | 电源类型(1B) | 开关量(5B) | CRC16(2B)  -> C 代码按 9 字节解析
#   注释里提到帧尾 0xAA, 但 C 代码不校验; 若硬件实际发 10 字节(带 0xAA 帧尾),
#   脚本切出 9 字节帧后会自动吞掉紧随的 0xAA 尾字节。
FRAME_HEAD = 0x55
FRAME_TAIL = 0xAA
FRAME_LEN = 9

# 攒"一阵"数据的超时: 超过该时间没新字节, 就把缓冲区残余当零散数据打印
IDLE_TIMEOUT = 0.5


def fmt_time():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]


def hex_dump(data):
    return " ".join(f"{b:02X}" for b in data)


def modbus_crc16(data):
    """Modbus CRC16, 主程序 CRC 算法未公开, 此处仅供参考"""
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def print_frame(frame, hex_only):
    if hex_only:
        print(f"[{fmt_time()}] RX frame ({len(frame)}B): {hex_dump(frame)}")
        return

    power_type = frame[1]
    sw = frame[2:7]
    crc_recv = frame[7] | (frame[8] << 8)
    crc_calc = modbus_crc16(frame[1:7])  # 校验范围按常见约定, 与主程序算法可能不同, 仅供参考
    crc_note = "OK" if crc_recv == crc_calc else f"MISMATCH(recv={crc_recv:04X} calc={crc_calc:04X})"

    print(f"[{fmt_time()}] RX frame ({len(frame)}B): {hex_dump(frame)}")
    print(f"    power_type = 0x{power_type:02X} ({power_type})")
    print(f"    switch[5B] = {hex_dump(sw)}   # sw[0]=B点状态(frame[i+2]), sw[3]=水泵状态(frame[i+5])")
    print(f"    CRC16(ref) = {crc_note}")


def main():
    ap = argparse.ArgumentParser(description="Luckfox Pico Ultra 串口接收验证 (纯接收)")
    ap.add_argument("port", nargs="?", default="/dev/ttyS3", help="串口设备节点 (默认 /dev/ttyS3)")
    ap.add_argument("baud", nargs="?", type=int, default=115200, help="波特率 (默认 115200)")
    ap.add_argument("--hex-only", action="store_true", help="不做字段解析, 只打印原始 HEX")
    args = ap.parse_args()

    print(f"打开串口 {args.port} @ {args.baud} 8N1 ...")
    try:
        uart = serial.Serial(
            args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=IDLE_TIMEOUT,
        )
    except Exception as e:
        print(f"打开失败: {e}")
        print("检查: 1) ls /dev/ttyS* 看节点是否存在  2) 是否被主程序占用")
        print("      3) 权限: sudo chmod a+rw /dev/ttyS3  4) luckfox-config 是否使能了 UART3")
        sys.exit(1)

    print("开始监听, Ctrl+C 退出。 持续无输出 = 串口没有数据进来 (接线/电平/波特率问题)。\n")
    buf = bytearray()

    while True:
        try:
            n = uart.in_waiting
            data = uart.read(n if n > 0 else 1)  # 无数据时最多等 IDLE_TIMEOUT 秒
        except KeyboardInterrupt:
            break

        if not data:
            if buf:
                print(f"[{fmt_time()}] 不完整数据 ({len(buf)}B): {hex_dump(buf)}")
                buf.clear()
            continue

        buf.extend(data)

        # 按帧切分: 找到 0x55 就取 9 字节 (与主程序 DealSerialData 的解析方式一致)
        while True:
            idx = buf.find(bytes([FRAME_HEAD]))
            if idx < 0:
                break
            if idx > 0:
                if not args.hex_only:
                    print(f"[{fmt_time()}] 帧外数据 ({idx}B): {hex_dump(buf[:idx])}")
                del buf[:idx]
            if len(buf) < FRAME_LEN:
                break  # 不够一帧, 继续攒
            print_frame(bytes(buf[:FRAME_LEN]), args.hex_only)
            del buf[:FRAME_LEN]
            # 若后面紧跟 0xAA, 说明实际协议可能是 10 字节带帧尾, 吞掉尾字节避免干扰
            if buf and buf[0] == FRAME_TAIL:
                del buf[:1]


if __name__ == "__main__":
    main()
