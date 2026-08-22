#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Luckfox Pico Ultra 串口发送测试脚本
用途:
每 5 秒向串口发送一包符合协议的数据，用于测试串口发送功能或配合接收脚本进行通信/环回测试。
帧格式与主程序 DealSerialData 的解析保持一致: 0x55 | 电源类型(1B) | 开关量(5B) | CRC16(2B) (共 9 字节)。
用法:
python3 uart_tx_monitor.py                        # /dev/ttyS3 @ 115200
python3 uart_tx_monitor.py /dev/ttyS3 9600        # 指定端口和波特率
运行前排查清单:
1. 确保对端设备正在监听该串口，且波特率一致。
2. /dev/ttyS3 不存在 -> 板子上运行 luckfox-config -> Advanced Options -> UART 使能 UART3。
3. 权限不足 -> sudo chmod a+rw /dev/ttyS3
4. 接线: 设备 GND <-> 板子 GND, 设备 RX <-> 板子 UART3_TX_M0 (如果是环回测试则 TX 接 RX)。
"""
import argparse
import datetime
import sys
import time
import serial

FRAME_HEAD = 0x55
FRAME_LEN = 9
SEND_INTERVAL = 5  # 发送间隔，单位：秒

def fmt_time():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

def hex_dump(data):
    return " ".join(f"{b:02X}" for b in data)

def modbus_crc16(data):
    """Modbus CRC16, 与接收脚本及主程序保持一致"""
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

def build_frame(power_type, switches):
    """构造 9 字节数据帧"""
    # payload 包含: 电源类型(1B) + 开关量(5B) = 6B
    payload = bytes([power_type]) + bytes(switches)
    crc = modbus_crc16(payload)
    # CRC 小端序: 低字节在前，高字节在后 (对应接收端 frame[7] | (frame[8] << 8))
    crc_bytes = bytes([crc & 0xFF, (crc >> 8) & 0xFF])
    return bytes([FRAME_HEAD]) + payload + crc_bytes

def main():
    ap = argparse.ArgumentParser(description="Luckfox Pico Ultra 串口发送测试 (每5秒一包)")
    ap.add_argument("port", nargs="?", default="/dev/ttyS3", help="串口设备节点 (默认 /dev/ttyS3)")
    ap.add_argument("baud", nargs="?", type=int, default=115200, help="波特率 (默认 115200)")
    args = ap.parse_args()

    print(f"打开串口 {args.port} @ {args.baud} 8N1 ...")
    try:
        uart = serial.Serial(
            args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1,
        )
    except Exception as e:
        print(f"打开失败: {e}")
        print("检查: 1) ls /dev/ttyS* 看节点是否存在  2) 是否被其他程序占用")
        print("      3) 权限: sudo chmod a+rw /dev/ttyS3  4) luckfox-config 是否使能了 UART3")
        sys.exit(1)

    print(f"开始发送, 间隔 {SEND_INTERVAL} 秒。Ctrl+C 退出。")
    
    # 模拟数据：电源类型固定为 0x01，开关量用一个计数器做简单变化以便观察
    power_type = 0x01
    counter = 0
    
    try:
        while True:
            # 构造开关量数据 (5字节)，让第一个字节随计数器变化
            switches = [counter & 0xFF, 0x00, 0x00, 0x00, 0x00]
            frame = build_frame(power_type, switches)
            
            uart.write(frame)
            print(f"[{fmt_time()}] TX frame ({len(frame)}B): {hex_dump(frame)}")
            print(f"    power_type = 0x{power_type:02X}, switch[0] = {counter & 0xFF:02X}")
            
            counter += 1
            time.sleep(SEND_INTERVAL)
            
    except KeyboardInterrupt:
        print("\n停止发送。")
    finally:
        uart.close()

if __name__ == "__main__":
    main()
