#!/usr/bin/env python3
"""
灵犀 MQTT → WebSocket 桥

功能:
  订阅 MQTT display topic → 通过 WebSocket 推送到浏览器 UI 模拟器
  无需真机硬件即可在浏览器实时看屏端推送效果

用法:
  python3 mqtt-ws-bridge.py                           # MQTT 模式
  python3 mqtt-ws-bridge.py --mock                     # 无 MQTT 自生成模拟数据
  python3 mqtt-ws-bridge.py --mock --device abc123     # 指定设备 ID
  python3 mqtt-ws-bridge.py --mock --port 8888         # 指定 WS 端口

配合:
  浏览器打开 https://licoo12580-source.github.io/lingxi-pages/docs/ui-simulator-v3.html
  HTML 自动连接 ws://localhost:8765，实时反映数据
"""

import asyncio
import json
import sys
import argparse
import time
import threading
from datetime import datetime

import paho.mqtt.client as mqtt
import websockets

# ========== 配置 ==========
BROKER = "120.24.228.11"
PORT = 1883
WS_PORT = 8765

# ========== WebSocket 服务 ==========
connected_clients = set()
latest_display_data = None
loop = None


async def ws_handler(websocket):
    """WebSocket 客户端连接处理"""
    connected_clients.add(websocket)
    addr = websocket.remote_address
    print(f"[WS] 客户端已连接: {addr}  (当前连接数: {len(connected_clients)})")

    # 发送最新数据 (如果有)
    if latest_display_data:
        try:
            await websocket.send(json.dumps(latest_display_data, ensure_ascii=False))
        except Exception:
            pass

    try:
        async for message in websocket:
            try:
                cmd = json.loads(message)
                if cmd.get("cmd") == "ping":
                    await websocket.send(json.dumps({"cmd": "pong", "ts": datetime.utcnow().isoformat() + "Z"}))
                elif cmd.get("cmd") == "resend" and latest_display_data:
                    await websocket.send(json.dumps(latest_display_data, ensure_ascii=False))
            except json.JSONDecodeError:
                pass
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        connected_clients.discard(websocket)
        print(f"[WS] 客户端已断开: {addr}  (当前连接数: {len(connected_clients)})")


async def broadcast(payload: str):
    """向所有已连接 WebSocket 客户端推送消息"""
    if not connected_clients:
        return
    tasks = [client.send(payload) for client in connected_clients.copy()]
    if tasks:
        await asyncio.gather(*tasks, return_exceptions=True)


# ========== MQTT 回调 ==========
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        device_id = userdata.get("device_id", "a1b2c3")
        display_topic = f"lingxi/v1/{device_id}/display"
        print(f"[MQTT] ✓ 已连接 {BROKER}:{PORT}")
        client.subscribe(display_topic, qos=0)
        print(f"[MQTT] ✓ 已订阅 {display_topic}")
    else:
        print(f"[MQTT] ✗ 连接失败: rc={rc}")
        sys.exit(1)


def on_disconnect(client, userdata, rc):
    print(f"[MQTT] ⚠ 断开连接 (rc={rc})，正在重连...")


def on_message(client, userdata, msg):
    """收到 MQTT display 数据 → 广播到所有 WS 客户端"""
    global latest_display_data
    try:
        payload_str = msg.payload.decode("utf-8")
        data = json.loads(payload_str)
        latest_display_data = data

        topic_parts = msg.topic.split("/")
        device = topic_parts[2] if len(topic_parts) >= 3 else "??"
        energy = data.get("energy", {})
        print(f"[MQTT] ← display [{device}] "
              f"省电={energy.get('today_saved', '?')}kWh "
              f"人数={data.get('sense', {}).get('total_people', '?')}")

        asyncio.run_coroutine_threadsafe(
            broadcast(json.dumps(data, ensure_ascii=False)),
            loop
        )
    except json.JSONDecodeError as e:
        print(f"[MQTT] ✗ JSON 解析失败: {e}")
    except Exception as e:
        print(f"[MQTT] ✗ 处理异常: {e}")


# ========== 模拟数据生成器 ==========
ZONES_PATTERNS = [
    [{"z": "bed", "s": "有人", "c": "green"},
     {"z": "door", "s": "无人", "c": "gray"},
     {"z": "window", "s": "无人", "c": "gray"},
     {"z": "bath", "s": "有人", "c": "green"}],
    [{"z": "bed", "s": "有人", "c": "green"},
     {"z": "door", "s": "有人", "c": "green"},
     {"z": "window", "s": "无人", "c": "gray"},
     {"z": "bath", "s": "无人", "c": "gray"}],
    [{"z": "bed", "s": "无人", "c": "gray"},
     {"z": "door", "s": "无人", "c": "gray"},
     {"z": "window", "s": "无人", "c": "gray"},
     {"z": "bath", "s": "无人", "c": "gray"}],
    [{"z": "bed", "s": "有人", "c": "green"},
     {"z": "door", "s": "无人", "c": "gray"},
     {"z": "window", "s": "有人", "c": "green"},
     {"z": "bath", "s": "有人", "c": "green"}],
]


def mock_data_generator(device_id="a1b2c3"):
    """生成随机的 display 模拟数据 (生成器，每次 next 返回一帧)"""
    idx = 0
    while True:
        t = time.time()
        z = ZONES_PATTERNS[idx % len(ZONES_PATTERNS)]
        people = sum(1 for zz in z if zz["s"] == "有人")
        hour = datetime.utcfromtimestamp(t).hour

        saved = 2.5 + (t % 3) * 0.5
        month = 80 + (t % 50)
        usage = 7 + (t % 4)
        temp = 26 if hour < 18 else 24

        data = {
            "id": device_id,
            "ts": datetime.utcfromtimestamp(t).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "energy": {
                "today_saved": round(saved, 1),
                "month_saved": round(month, 1),
                "today_usage": round(usage, 1),
                "month_usage": round(month * 2.5, 1),
                "saved_rooms": max(1, people),
                "temp_set": temp
            },
            "sense": {
                "zones": z,
                "total_people": people,
                "status_text": "🟢 实时监测" if people > 0 else "🔴 全房无人",
                "has_alarm": 0 if people > 0 else 1,
                "alarm_text": "" if people > 0 else "全房无人超 30 分钟"
            },
            "system": {
                "version": "v0621Z",
                "device": "ESP32-S3",
                "screen": "ILI9488 320x480",
                "online": 1,
                "touch_ok": 1,
                "rssi": -65,
                "uptime_days": int(t // 86400 % 30)
            }
        }
        yield data
        idx += 1
        time.sleep(5)  # 5 秒一帧


# ========== 主入口 ==========
def main():
    global loop

    parser = argparse.ArgumentParser(description="灵犀 MQTT → WebSocket 桥")
    parser.add_argument("--device", type=str, default="a1b2c3", help="设备 ID")
    parser.add_argument("--port", type=int, default=WS_PORT, help="WebSocket 端口")
    parser.add_argument("--mock", action="store_true",
                        help="无 MQTT 时自发生成模拟数据（每5秒一帧）")
    args = parser.parse_args()

    ws_port = args.port
    device_id = args.device
    use_mock = args.mock

    print("=" * 50)
    print("  灵犀 MQTT → WebSocket 桥")
    print("=" * 50)
    print(f"  WebSocket:    ws://localhost:{ws_port}")
    print(f"  UI 模拟器:    "
          f"https://licoo12580-source.github.io/lingxi-pages/docs/ui-simulator-v3.html")
    if use_mock:
        print(f"  模式:         模拟数据 (每 5 秒一帧)")
        print(f"  设备 ID:      {device_id}")
        print()
        print("  使用方法:")
        print("  1. 本桥运行后，打开 UI 模拟器")
        print("  2. 浏览器自动接收实时模拟数据")
    else:
        print(f"  MQTT Broker:  {BROKER}:{PORT}")
        print(f"  订阅主题:     lingxi/v1/{device_id}/display")
        print()
        print("  使用方法:")
        print("  1. 本桥运行后，打开 UI 模拟器 (自动连接 WS)")
        print(f"  2. 另开终端: python3 simulate-device.py --device {device_id}")
        print(f"     或: python3 push-display-test.py --device {device_id}")
        print("  3. 浏览器实时显示屏端效果")
    print("=" * 50)
    print()

    # --- WebSocket 服务 (asyncio) ---
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    async def start_ws():
        async with websockets.serve(ws_handler, "0.0.0.0", ws_port):
            print(f"[WS] ✓ WebSocket 服务已启动: ws://0.0.0.0:{ws_port}")

            if use_mock:
                # 模拟数据线程
                def mock_worker():
                    gen = mock_data_generator(device_id)
                    while True:
                        try:
                            data = next(gen)
                            global latest_display_data
                            latest_display_data = data

                            future = asyncio.run_coroutine_threadsafe(
                                broadcast(json.dumps(data, ensure_ascii=False)),
                                loop
                            )
                            future.result(timeout=3)

                            energy = data["energy"]
                            print(f"[MOCK] → display [{device_id}] "
                                  f"省电={energy['today_saved']}kWh "
                                  f"人数={data['sense']['total_people']}")
                        except Exception as e:
                            print(f"[MOCK] ✗ 错误: {e}")

                t = threading.Thread(target=mock_worker, daemon=True)
                t.start()
            else:
                # MQTT 客户端
                mqtt_client = mqtt.Client(userdata={"device_id": device_id})
                mqtt_client.on_connect = on_connect
                mqtt_client.on_disconnect = on_disconnect
                mqtt_client.on_message = on_message
                try:
                    mqtt_client.connect(BROKER, PORT, 60)
                    mqtt_client.loop_start()
                except Exception as e:
                    print(f"[MQTT] ✗ 连接失败: {e}")
                    print("[MQTT]   提示: 用 --mock 可在无 MQTT 时独立运行")

            await asyncio.Future()  # 永久运行

    try:
        loop.run_until_complete(start_ws())
    except KeyboardInterrupt:
        print("\n[EXIT] 正在关闭...")
    finally:
        if not use_mock:
            try:
                mqtt_client.loop_stop()
                mqtt_client.disconnect()
            except Exception:
                pass
        print("[EXIT] 已退出")


if __name__ == "__main__":
    main()
