#!/usr/bin/env python3
"""
灵犀 MQTT v3 测试推送脚本
推送 display 数据到设备，用于屏端渲染验证

用法:
  python3 push-display-test.py                    # 推送正常场景
  python3 push-display-test.py --alarm            # 带告警
  python3 push-display-test.py --touch-bad        # 触摸异常
  python3 push-display-test.py --empty            # 无人
  python3 push-display-test.py --custom FILE.json # 自定义JSON
"""

import paho.mqtt.client as mqtt
import json
import time
import sys
import argparse

BROKER = "120.24.228.11"
PORT = 1883
DEVICE_ID = "a1b2c3"

def build_payload(args):
    now = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

    if args.custom:
        with open(args.custom, 'r') as f:
            return json.load(f)

    zones = [
        {"z": "bed",    "s": "有人", "c": "green"},
        {"z": "door",   "s": "无人", "c": "gray"},
        {"z": "window", "s": "无人", "c": "gray"},
        {"z": "bath",   "s": "有人", "c": "green"},
    ]

    if args.empty:
        zones = [
            {"z": "bed",    "s": "无人", "c": "gray"},
            {"z": "door",   "s": "无人", "c": "gray"},
            {"z": "window", "s": "无人", "c": "gray"},
            {"z": "bath",   "s": "无人", "c": "gray"},
        ]

    payload = {
        "id": DEVICE_ID,
        "ts": now,
        "energy": {
            "today_saved": 3.2,
            "month_saved": 96.8,
            "saved_rooms": 2,
            "today_usage": 8.5,
            "month_usage": 255.0,
            "temp_set": 26
        },
        "sense": {
            "zones": zones,
            "total_people": 0 if args.empty else 2,
            "status_text": "实时监测",
            "has_alarm": 1 if args.alarm else 0,
            "alarm_text": "门未关" if args.alarm else ""
        },
        "system": {
            "version": "v0621Z",
            "device": "ESP32-S3",
            "screen": "ILI9488 320x480",
            "online": 1,
            "touch_ok": 0 if args.touch_bad else 1,
            "wifi_rssi": -65,
            "uptime_days": 3
        }
    }
    return payload

def push(payload, count=1, interval=0):
    client = mqtt.Client()
    client.connect(BROKER, PORT, 60)
    topic = f"lingxi/v1/{payload['id']}/display"

    for i in range(count):
        payload['ts'] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        json_str = json.dumps(payload, ensure_ascii=False)
        client.publish(topic, json_str)
        print(f"[{i+1}/{count}] Published to {topic}  ({len(json_str)} bytes)")

        if count > 1:
            # 测试场景: 动态变化
            if i % 2 == 0:
                payload['sense']['zones'][0]['s'] = "无人"
                payload['sense']['zones'][0]['c'] = "gray"
                payload['sense']['total_people'] = 1
            else:
                payload['sense']['zones'][0]['s'] = "有人"
                payload['sense']['zones'][0]['c'] = "green"
                payload['sense']['total_people'] = 2

            time.sleep(interval)

    client.disconnect()
    print("Done")

def main():
    parser = argparse.ArgumentParser(description="灵犀 MQTT v3 测试推送")
    parser.add_argument("--alarm", action="store_true", help="带告警场景")
    parser.add_argument("--touch-bad", action="store_true", help="触摸异常场景")
    parser.add_argument("--empty", action="store_true", help="无人场景")
    parser.add_argument("--custom", type=str, help="自定义 JSON 文件")
    parser.add_argument("--count", type=int, default=1, help="推送次数")
    parser.add_argument("--interval", type=int, default=5, help="间隔秒数")
    parser.add_argument("--device", type=str, default=DEVICE_ID, help="设备ID")

    args = parser.parse_args()
    payload = build_payload(args)
    payload['id'] = args.device

    print(f"=== 灵犀 MQTT v3 测试推送 ===")
    print(f"设备: {args.device}")
    print(f"场景: {'告警' if args.alarm else '正常'}"
          f"{' + 触摸异常' if args.touch_bad else ''}"
          f"{' + 无人' if args.empty else ''}")
    print(f"推送 {args.count} 次, 间隔 {args.interval}s")
    print(json.dumps(payload, ensure_ascii=False, indent=2))
    print()

    push(payload, args.count, args.interval)

if __name__ == "__main__":
    main()
