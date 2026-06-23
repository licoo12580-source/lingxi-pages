#!/usr/bin/env python3
"""
灵犀 MQTT v3 设备模拟器 (Python)

模拟 ML307C 4G 模块行为:
  - 连接 MQTT Broker
  - 订阅 display/command/ota
  - 按周期发送 heartbeat + telemetry
  - 解析 receive 的 display 数据
  - 接收 command 并执行

用于在无硬件时验证云端推送正确性。

用法:
  python3 simulate-device.py                    # 默认设备 a1b2c3
  python3 simulate-device.py --device abc123    # 指定设备 ID
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
import sys
import signal

BROKER = "120.24.228.11"
PORT = 1883

SLEEP_S = 0.5  # 主循环间隔

# 模拟数据
sim_radar_people = 1
sim_temp = 26.5
sim_humidity = 60
sim_uptime = 0


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[OK] Connected to {BROKER}:{PORT}")
        device_id = userdata['device_id']
        # 订阅
        topics = [
            f"lingxi/v1/{device_id}/display",
            f"lingxi/v1/{device_id}/command",
            f"lingxi/v1/{device_id}/ota",
        ]
        for t in topics:
            client.subscribe(t, qos=0)
            print(f"[SUB] {t}")
    else:
        print(f"[ERR] Connection failed: rc={rc}")


def on_disconnect(client, userdata, rc):
    print(f"[WARN] Disconnected (rc={rc})")


def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode('utf-8', errors='replace')
    print(f"\n▼ RX topic={topic}")

    # 提取 type
    parts = topic.split('/')
    if len(parts) >= 4:
        tp = parts[3]
        if tp == 'display':
            display_handler(payload)
        elif tp == 'command':
            command_handler(payload)
        elif tp == 'ota':
            print(f"  [OTA] {payload}")
        else:
            print(f"  [unknown type={tp}]")
    else:
        print(f"  [bad topic] {payload[:100]}")


def display_handler(payload):
    """解析 display 数据"""
    try:
        data = json.loads(payload)
        energy = data.get('energy', {})
        sense = data.get('sense', {})
        system = data.get('system', {})

        print(f"  ┌─ 节能 ─────────────────────")
        print(f"  │ today_saved={energy.get('today_saved')}kWh")
        print(f"  │ month_saved={energy.get('month_saved')}kWh")
        print(f"  │ saved_rooms={energy.get('saved_rooms')}")
        print(f"  │ temp_set={energy.get('temp_set')}°C")
        print(f"  ├─ 感知 ─────────────────────")

        for z in sense.get('zones', []):
            print(f"  │ {z['z']}: {z['s']} ({z['c']})")

        print(f"  │ total_people={sense.get('total_people')}")
        if sense.get('has_alarm'):
            print(f"  │ ⚠ ALARM: {sense.get('alarm_text')}")
        print(f"  ├─ 系统 ─────────────────────")
        print(f"  │ ver={system.get('version')} touch={system.get('touch_ok')}")
        print(f"  │ online={system.get('online')} days={system.get('uptime_days')}")
        print(f"  └────────────────────────────")
    except json.JSONDecodeError as e:
        print(f"  [JSON ERROR] {e}")


def command_handler(payload):
    """解析并执行 command"""
    try:
        cmd_data = json.loads(payload)
        cmd = cmd_data.get('cmd', '')
        params = cmd_data.get('params', {})

        print(f"  [CMD] {cmd} {json.dumps(params)}")

        if cmd == 'reboot':
            print("  >>> SIMULATED REBOOT <<<")
        elif cmd == 'set_temp':
            temp = params.get('temp', 26)
            print(f"  >>> Set temp to {temp}°C")
        elif cmd == 'set_mode':
            mode = params.get('mode', 'eco')
            print(f"  >>> Set mode to {mode}")
        elif cmd == 'display_test':
            pattern = params.get('pattern', 'all')
            print(f"  >>> Display test: {pattern}")
        else:
            print(f"  >>> Unknown command: {cmd}")
    except json.JSONDecodeError:
        print(f"  [CMD JSON ERROR]")


def send_heartbeat(client, device_id):
    """发送心跳"""
    global sim_uptime
    sim_uptime += 30
    payload = {
        "id": device_id,
        "ts": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "online": 1,
        "rssi": -65,
        "uptime": sim_uptime,
        "ver": "v0621Z",
        "health": {
            "ml307": 2,
            "radar": 2,
            "touch": 2,
            "disp": 2
        }
    }
    topic = f"lingxi/v1/{device_id}/heartbeat"
    client.publish(topic, json.dumps(payload))
    print(f"[HB] {topic}")


def send_telemetry(client, device_id):
    """发送遥测"""
    payload = {
        "id": device_id,
        "ts": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "radar": {
            "people": sim_radar_people,
            "moving": 1,
            "zones": [
                {"z": "bed",    "p": 1},
                {"z": "door",   "p": 0},
                {"z": "window", "p": 0},
                {"z": "bath",   "p": 0},
            ]
        },
        "env": {
            "temp": sim_temp,
            "humidity": sim_humidity
        }
    }
    topic = f"lingxi/v1/{device_id}/telemetry"
    client.publish(topic, json.dumps(payload))
    print(f"[TEL] {topic}")


def main():
    global sim_radar_people, sim_temp, sim_humidity

    import argparse
    parser = argparse.ArgumentParser(description="灵犀 MQTT v3 设备模拟器")
    parser.add_argument("--device", type=str, default="a1b2c3", help="设备ID")
    parser.add_argument("--people", type=int, default=1, help="模拟人数")
    parser.add_argument("--temp", type=float, default=26.5, help="模拟温度")
    args = parser.parse_args()

    device_id = args.device
    sim_radar_people = args.people
    sim_temp = args.temp

    print(f"=== 灵犀 MQTT v3 设备模拟器 ===")
    print(f"设备 ID: {device_id}")
    print(f"Broker:  {BROKER}:{PORT}")
    print(f"模拟: 人数={sim_radar_people}, 温度={sim_temp}°C")
    print(f"按 Ctrl+C 退出\n")

    client = mqtt.Client(userdata={'device_id': device_id})
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    try:
        client.connect(BROKER, PORT, 60)
        client.loop_start()
    except Exception as e:
        print(f"[ERR] Connection failed: {e}")
        sys.exit(1)

    # 定时器
    last_hb = time.time()
    last_tel = time.time()

    running = True
    signal.signal(signal.SIGINT, lambda s, f: exit(0))

    try:
        while running:
            now = time.time()

            # 心跳: 30s
            if now - last_hb >= 30:
                send_heartbeat(client, device_id)
                last_hb = now

            # 遥测: 10s
            if now - last_tel >= 10:
                send_telemetry(client, device_id)
                last_tel = now

            time.sleep(SLEEP_S)

    except KeyboardInterrupt:
        print("\n[EXIT]")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
