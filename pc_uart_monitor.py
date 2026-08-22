#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
本机串口助手 - 接收端
实时显示开发板发来的串口数据, HEX + ASCII 双栏, 类似串口助手的接收窗口。

用法:
    python3 pc_uart_monitor.py                       # 默认 /dev/ttyACM0 @ 115200
    python3 pc_uart_monitor.py /dev/ttyUSB0 9600     # 指定端口和波特率

依赖 pyserial, 本机未装时报错提示里有安装方法。
"""

import argparse
import datetime
import sys

try:
    import serial
except ImportError:
    print("缺少 pyserial, 请先安装:")
    print("  sudo apt install python3-serial")
    print("  或: pip3 install --user --break-system-packages pyserial")
    sys.exit(1)


def fmt_time():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]


def to_ascii(data):
    return "".join(chr(b) if 32 <= b <= 126 else "." for b in data)


def main():
    ap = argparse.ArgumentParser(description="本机串口接收助手")
    ap.add_argument("port", nargs="?", default="/dev/ttyACM0", help="串口设备 (默认 /dev/ttyACM0)")
    ap.add_argument("baud", nargs="?", type=int, default=115200, help="波特率 (默认 115200)")
    args = ap.parse_args()

    try:
        uart = serial.Serial(
            args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.3,          # 0.3s 无新数据视为"一阵"结束
        )
    except Exception as e:
        print(f"打开失败: {e}")
        print("检查: 1) 串口工具是否插好 (ls /dev/ttyACM* /dev/ttyUSB*)")
        print("      2) 权限: sudo chmod a+rw /dev/ttyACM0")
        print("      3) 是否被本脚本的另一个实例占用")
        sys.exit(1)

    print(f"监听 {args.port} @ {args.baud} 8N1, Ctrl+C 退出\n")
    buf = bytearray()
    try:
        while True:
            n = uart.in_waiting
            data = uart.read(n if n > 0 else 1)
            if data:
                buf.extend(data)
            elif buf:
                # 一阵结束, 打印
                print(f"[{fmt_time()}] RX {len(buf):3d}B: {buf.hex(' ').upper():<60} | {to_ascii(buf)}")
                buf.clear()
    except KeyboardInterrupt:
        if buf:
            print(f"[{fmt_time()}] RX {len(buf):3d}B: {buf.hex(' ').upper():<60} | {to_ascii(buf)}")
        print("\n已退出")
    finally:
        uart.close()


if __name__ == "__main__":
    main()
