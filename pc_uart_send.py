#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
本机串口助手 - 发送端
向开发板串口发送任意 HEX 数据或文本, 支持命令行一次性发送和交互模式。

用法:
    # 交互模式 (推荐日常调试用)
    python3 pc_uart_send.py

    # 一次性发送一帧 HEX 后退出
    python3 pc_uart_send.py "55 01 00 00 00 00 DE 31 AA"

    # 周期发送
    python3 pc_uart_send.py "55 01 00 00 00 00 DE 31 AA" --repeat 10 --interval 1

    # 指定端口/波特率
    python3 pc_uart_send.py --port /dev/ttyUSB0 --baud 9600 "AA BB CC"

交互模式命令:
    > 55 01 00 00 00 00 DE 31 AA       发送 HEX (空格/逗号分隔, 支持 0x 前缀)
    > txt:hello                        发送文本 (UTF-8)
    > repeat 10 0.5 55 AA BB           以 0.5s 间隔重复发 10 次
    > quit / exit / 空行               退出

依赖 pyserial, 本机未装时报错提示里有安装方法。
"""

import argparse
import datetime
import sys
import time

try:
    import serial
except ImportError:
    print("缺少 pyserial, 请先安装:")
    print("  sudo apt install python3-serial")
    print("  或: pip3 install --user --break-system-packages pyserial")
    sys.exit(1)


def fmt_time():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]


def parse_hex(text):
    """把 '55 AA,0xBB' 之类的字符串解析成 bytes, 非法字符报错"""
    text = text.replace(",", " ").replace("0x", "").replace("0X", "")
    parts = text.split()
    if not parts:
        raise ValueError("空数据")
    out = bytearray()
    for p in parts:
        if len(p) != 2:
            raise ValueError(f"非法字节 '{p}' (每个字节应为 2 位十六进制)")
        out.append(int(p, 16))
    return bytes(out)


def do_send(uart, data):
    uart.write(data)
    uart.flush()
    print(f"[{fmt_time()}] TX {len(data):3d}B: {data.hex(' ').upper()}")


def main():
    ap = argparse.ArgumentParser(description="本机串口发送助手")
    ap.add_argument("--port", default="/dev/ttyACM0", help="串口设备 (默认 /dev/ttyACM0)")
    ap.add_argument("--baud", type=int, default=115200, help="波特率 (默认 115200)")
    ap.add_argument("data", nargs="?", help="要发送的 HEX 字符串, 省略则进入交互模式")
    ap.add_argument("--repeat", type=int, default=1, help="重复发送次数 (默认 1)")
    ap.add_argument("--interval", type=float, default=0.5, help="重复发送间隔秒数 (默认 0.5)")
    args = ap.parse_args()

    try:
        uart = serial.Serial(
            args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.5,
        )
    except Exception as e:
        print(f"打开失败: {e}")
        print("检查: 1) 串口工具是否插好 (ls /dev/ttyACM* /dev/ttyUSB*)")
        print("      2) 权限: sudo chmod a+rw /dev/ttyACM0")
        print("      3) 是否被其他串口程序占用")
        sys.exit(1)

    print(f"已打开 {args.port} @ {args.baud} 8N1")

    # 命令行一次性发送
    if args.data:
        try:
            data = parse_hex(args.data)
        except ValueError as e:
            print(f"数据解析失败: {e}")
            sys.exit(1)
        for _ in range(args.repeat):
            do_send(uart, data)
            if args.repeat > 1:
                time.sleep(args.interval)
        uart.close()
        return

    # 交互模式
    print("交互模式, 命令: HEX数据 | txt:文本 | repeat N 间隔 HEX | quit 退出\n")
    while True:
        try:
            line = input("> ").strip()
        except (KeyboardInterrupt, EOFError):
            print("\n已退出")
            break
        if not line or line in ("quit", "exit"):
            break
        if line.startswith("txt:"):
            do_send(uart, line[4:].encode("utf-8"))
            continue
        if line.startswith("repeat"):
            try:
                _, n, interval, hexpart = line.split(None, 3)
                data = parse_hex(hexpart)
                for _ in range(int(n)):
                    do_send(uart, data)
                    time.sleep(float(interval))
            except (ValueError, TypeError) as e:
                print(f"repeat 用法: repeat 次数 间隔秒 HEX数据   ({e})")
            continue
        try:
            do_send(uart, parse_hex(line))
        except ValueError as e:
            print(f"解析失败: {e}")

    uart.close()


if __name__ == "__main__":
    main()
