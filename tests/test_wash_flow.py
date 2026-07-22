#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WashFlow 手动/自动测试脚本（同机测试）

用法：
    1. 编译测试版本：
       mkdir build && cd build
       cmake .. -DWASH_TEST_MODE=ON && make -j

    2. 把 src/net_cfg.json 里的 remote_server 改成 127.0.0.1，remote_port 改成 9999

    3. 运行被测程序：
       ./CarClean

    4. 运行测试脚本：
       python3 tests/test_wash_flow.py           # 交互式菜单
       python3 tests/test_wash_flow.py 1         # 直接执行场景 1
"""

import json
import sys
import time
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler

import requests

DEVICE_HOST = "127.0.0.1"
DEVICE_PORT = 8080
UPLOAD_PORT = 9999
BASE_URL = f"http://{DEVICE_HOST}:{DEVICE_PORT}"
AI_DELAY_SECONDS = 3  # default_info.json 中的 time_after_b，默认 3


# ==================== Mock 上报服务器 ====================
class UploadReportHandler(BaseHTTPRequestHandler):
    reports = []

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)
        try:
            data = json.loads(body.decode("utf-8"))
        except Exception:
            data = {}
        UploadReportHandler.reports.append(data)
        print(f"[上报] dataType={data.get('dataType')} ztcCph={data.get('ztcCph')} "
              f"alarmType={data.get('alarmType')} cleanRes={data.get('cleanRes')} "
              f"direction={data.get('direction')}")
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b"OK")

    def log_message(self, fmt, *args):
        pass


def start_mock_server():
    UploadReportHandler.reports.clear()
    server = HTTPServer(("127.0.0.1", UPLOAD_PORT), UploadReportHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print(f"[MockServer] 已启动，监听 127.0.0.1:{UPLOAD_PORT}\n")
    return server


# ==================== HTTP 辅助 ====================
def post(path, data):
    return requests.post(f"{BASE_URL}{path}", json=data, timeout=5)


def plate_json(license_plate, direction=4):
    return {
        "AlarmInfoPlate": {
            "channel": 0,
            "deviceName": "IVS",
            "ipaddr": "192.168.1.100",
            "result": {
                "PlateResult": {
                    "colorType": 0,
                    "direction": direction,
                    "gioouts": [],
                    "imageFile": "/9j/4AAQfake",
                    "license": license_plate,
                    "timeStamp": {"Timeval": {"sec": int(time.time()), "usec": 0}},
                    "triggerType": 0,
                    "type": 0
                }
            },
            "serialno": "test",
            "user_data": ""
        }
    }


def ai_json(label):
    return {
        "label": label,
        "img_base64": f"fake_{label}_{int(time.time() * 1000)}",
        "extend": {"alarm_objs": [{"score": 0.95 if label == "clean" else 0.85}]}
    }


def send_serial(power=1, point_b=0, pump=0):
    frame = [0x55, power & 0xFF, point_b & 0xFF, 0x00, 0x00, pump & 0xFF, 0x00, 0x00, 0xAA]
    post("/test/serial", {"frame": frame})


def send_plate(license_plate, direction=4):
    post("/wash_report", plate_json(license_plate, direction))


def send_ai(left="clean", right="clean", tail="clean"):
    post("/aiipc/left", ai_json(left))
    post("/aiipc/right", ai_json(right))
    post("/aiipc/tail", ai_json(tail))


def wait_reports(data_type, timeout=15):
    deadline = time.time() + timeout
    while time.time() < deadline:
        for r in UploadReportHandler.reports[::-1]:
            if r.get("dataType") == data_type:
                return r
        time.sleep(0.2)
    return None


def clear_reports():
    UploadReportHandler.reports.clear()


# ==================== 测试场景 ====================
def scene_clean_car():
    """场景1：正常干净车"""
    print("\n>>> 场景1：正常干净车流程")
    clear_reports()

    send_plate("京A88888")
    time.sleep(0.3)

    send_serial(point_b=1, pump=1)
    time.sleep(0.2)

    for _ in range(8):
        send_ai("clean", "clean", "clean")
        time.sleep(0.2)

    send_serial(point_b=1, pump=0)
    time.sleep(0.2)
    send_serial(point_b=0, pump=0)

    print(f"等待 AI 窗口 {AI_DELAY_SECONDS}s ...")
    time.sleep(AI_DELAY_SECONDS + 1)

    r = wait_reports(1, timeout=10)
    if r and r.get("ztcCph") == "京A88888" and r.get("cleanRes") == 2:
        print(f"[OK] 上报成功:\n{json.dumps(r, ensure_ascii=False, indent=2)}")
    else:
        print(f"[FAIL] 未收到预期上报，reports={UploadReportHandler.reports}")


def scene_dirty_car():
    """场景2：脏车"""
    print("\n>>> 场景2：脏车流程")
    clear_reports()

    send_plate("沪B66666")
    time.sleep(0.3)

    send_serial(point_b=1, pump=1)
    time.sleep(0.2)

    for i in range(8):
        label = "dirty" if i == 4 else "clean"
        send_ai(label, label, label)
        time.sleep(0.2)

    send_serial(point_b=1, pump=0)
    time.sleep(0.2)
    send_serial(point_b=0, pump=0)

    print(f"等待 AI 窗口 {AI_DELAY_SECONDS}s ...")
    time.sleep(AI_DELAY_SECONDS + 1)

    r = wait_reports(1, timeout=10)
    if r and r.get("ztcCph") == "沪B66666" and r.get("cleanRes") == 3:
        print(f"[OK] 上报成功:\n{json.dumps(r, ensure_ascii=False, indent=2)}")
    else:
        print(f"[FAIL] 未收到预期上报，reports={UploadReportHandler.reports}")


def scene_no_pump():
    """场景3：水泵未工作"""
    print("\n>>> 场景3：水泵未工作")
    clear_reports()

    send_plate("粤C00000")
    time.sleep(0.3)

    send_serial(point_b=1, pump=0)
    for _ in range(5):
        send_ai("clean", "clean", "clean")
        time.sleep(0.2)

    send_serial(point_b=0, pump=0)
    print(f"等待 AI 窗口 {AI_DELAY_SECONDS}s ...")
    time.sleep(AI_DELAY_SECONDS + 1)

    r = wait_reports(1, timeout=10)
    if r and r.get("alarmType") == 3:
        print(f"[OK] 上报成功，alarmType=3（未冲洗）:\n{json.dumps(r, ensure_ascii=False, indent=2)}")
    else:
        print(f"[FAIL] 未收到预期上报，reports={UploadReportHandler.reports}")


def scene_no_plate():
    """场景4：抓拍机无车牌，不应上报"""
    print("\n>>> 场景4：抓拍机无车牌")
    clear_reports()

    payload = plate_json("京D11111")
    del payload["AlarmInfoPlate"]["result"]["PlateResult"]["license"]
    post("/wash_report", payload)
    time.sleep(0.3)

    send_serial(point_b=1, pump=1)
    for _ in range(5):
        send_ai("clean", "clean", "clean")
        time.sleep(0.2)
    send_serial(point_b=1, pump=0)
    send_serial(point_b=0, pump=0)

    time.sleep(AI_DELAY_SECONDS + 2)

    if any(r.get("dataType") == 1 for r in UploadReportHandler.reports):
        print(f"[FAIL] 不应收到冲洗上报，但收到了: {UploadReportHandler.reports}")
    else:
        print("[OK] 未收到冲洗上报，符合预期")


def scene_b_before_plate():
    """场景5：B点先触发，抓拍机后推送"""
    print("\n>>> 场景5：B点先触发，抓拍机后推送")
    clear_reports()

    send_serial(point_b=1, pump=0)
    time.sleep(0.5)

    send_plate("湘D99999")
    time.sleep(0.3)

    send_serial(point_b=1, pump=1)
    for _ in range(5):
        send_ai("clean", "clean", "clean")
        time.sleep(0.2)
    send_serial(point_b=1, pump=0)
    send_serial(point_b=0, pump=0)

    print(f"等待 AI 窗口 {AI_DELAY_SECONDS}s ...")
    time.sleep(AI_DELAY_SECONDS + 1)

    r = wait_reports(1, timeout=10)
    if r and r.get("ztcCph") == "湘D99999":
        print(f"[OK] 上报成功:\n{json.dumps(r, ensure_ascii=False, indent=2)}")
    else:
        print(f"[FAIL] 未收到预期上报，reports={UploadReportHandler.reports}")


def query_status():
    """查询当前传感器状态"""
    print("\n>>> 当前传感器状态")
    r = post("/test/status", {})
    print(json.dumps(r.json(), ensure_ascii=False, indent=2))


# ==================== 菜单 ====================
SCENES = {
    "1": ("正常干净车", scene_clean_car),
    "2": ("脏车", scene_dirty_car),
    "3": ("水泵未工作", scene_no_pump),
    "4": ("抓拍机无车牌", scene_no_plate),
    "5": ("B点先触发后拍车牌", scene_b_before_plate),
    "6": ("查询传感器状态", query_status),
}


def show_menu():
    print("\n========== WashFlow 测试菜单 ==========")
    for k, (name, _) in SCENES.items():
        print(f"  [{k}] {name}")
    print("  [0] 退出")
    print("======================================")


def run_scene(key):
    if key not in SCENES:
        print(f"未知选项: {key}")
        return
    _, func = SCENES[key]
    func()


def main():
    print("[WashFlow Test] 同机集成测试脚本")
    start_mock_server()

    try:
        requests.get(f"{BASE_URL}/test", timeout=2)
        print(f"[OK] 已连接到 {BASE_URL}\n")
    except Exception as e:
        print(f"[ERROR] 无法连接 {BASE_URL}: {e}")
        print("请确保 CarClean 已以测试模式启动（cmake .. -DWASH_TEST_MODE=ON）")
        return

    if len(sys.argv) > 1:
        arg = sys.argv[1]
        if arg in SCENES:
            run_scene(arg)
        else:
            print(f"可用参数: {', '.join(SCENES.keys())}")
        return

    while True:
        show_menu()
        choice = input("请选择要执行的场景: ").strip()
        if choice == "0":
            print("退出测试")
            break
        run_scene(choice)


if __name__ == "__main__":
    main()
