#!/usr/bin/env python3

import argparse
import json
import math
import socket
import socketserver
import sys
import threading
import time


class AtlasState:
    def __init__(self):
        self.lock = threading.RLock()
        self.clients = set()

        self.hm30_connected = False
        self.a8_connected = False
        self.frozen = False

        self.recording = False
        self.recording_started_ms = 0
        self.zoom_level = 1.0
        self.gimbal_yaw_deg = 0.0
        self.gimbal_pitch_deg = 0.0

        self.ai_enabled = True
        self.sahi_enabled = False
        self.depth_enabled = False
        self.tracking = False
        self.tracking_mode_enabled = False
        self.auto_gimbal = False

        self.ai_model = "YOLO11s"
        self.performance_profile = "BALANCED"
        self.mission_profile = "SEARCH"
        self.stream_mode = "AI"
        self.video_profile = "BALANCED"
        self.ai_classes = ["PERSON"]
        self.confidence = 0.45
        self.ai_fps_target = 18
        self.ai_resolution = 640
        self.sahi_slice = 640

        self.target_valid = False
        self.target_id = -1
        self.target_class = ""
        self.target_confidence = 0.0
        self.target_distance_m = 0.0
        self.target_bearing_deg = 0.0
        self.target_latitude = 0.0
        self.target_longitude = 0.0

        self.force_hot = False
        self.link_bad = False
        self.demo_mode = False

    def add_client(self, client):
        with self.lock:
            self.clients.add(client)

    def remove_client(self, client):
        with self.lock:
            self.clients.discard(client)

    def is_frozen(self):
        with self.lock:
            return self.frozen

    def set_frozen(self, frozen):
        with self.lock:
            self.frozen = frozen

        print(
            "[MOCK] Serviço " + ("CONGELADO" if frozen else "ATIVO"),
            flush=True,
        )

        if not frozen:
            self.broadcast_status()

    def snapshot(self):
        with self.lock:
            now = time.time()

            cpu = 34.0 + 5.0 * math.sin(now / 4.0)
            gpu_base = 66.0 if self.ai_enabled else 8.0
            gpu = gpu_base + 7.0 * math.sin(now / 3.0)
            ram_used = 5.5 + (0.5 if self.sahi_enabled else 0.0) + (0.3 if self.depth_enabled else 0.0)
            temp = 88.0 if self.force_hot else 49.0 + max(0.0, gpu - 50.0) * 0.09
            power = 8.5 + gpu * 0.065

            if self.sahi_enabled:
                ai_fps = min(float(self.ai_fps_target), 4.0)
            elif self.performance_profile == "FAST":
                ai_fps = min(float(self.ai_fps_target), 24.0)
            elif self.performance_profile == "ACCURATE":
                ai_fps = min(float(self.ai_fps_target), 10.0)
            else:
                ai_fps = min(float(self.ai_fps_target), 18.0)

            if not self.ai_enabled:
                ai_fps = 0.0

            signal = 42.0 if self.link_bad else 82.0
            rssi = -89.0 if self.link_bad else -67.0
            loss = 12.0 if self.link_bad else 0.4
            latency = 145.0 if self.link_bad else 38.0

            return {
                "hm30_connected": self.hm30_connected,
                "a8_connected": self.a8_connected,

                "jetson_cpu_percent": round(cpu, 1),
                "jetson_gpu_percent": round(max(0.0, gpu), 1),
                "jetson_ram_used_gb": round(ram_used, 1),
                "jetson_ram_total_gb": 8.0,
                "jetson_temp_c": round(temp, 1),
                "jetson_power_w": round(power, 1),
                "jetson_swap_gb": 0.2,
                "jetson_throttled": self.force_hot,

                "hm30_signal_percent": signal if self.hm30_connected else 0.0,
                "hm30_rssi_dbm": rssi if self.hm30_connected else 0.0,
                "hm30_packet_loss_percent": loss if self.hm30_connected else 0.0,
                "hm30_latency_ms": latency if self.hm30_connected else 0.0,
                "hm30_up_mbps": 0.4 if self.hm30_connected else 0.0,
                "hm30_down_mbps": 8.7 if self.hm30_connected else 0.0,

                "recording": self.recording,
                "zoom_level": round(self.zoom_level, 1),
                "gimbal_yaw_deg": round(self.gimbal_yaw_deg, 1),
                "gimbal_pitch_deg": round(self.gimbal_pitch_deg, 1),

                "ai_enabled": self.ai_enabled,
                "sahi_enabled": self.sahi_enabled,
                "depth_enabled": self.depth_enabled,
                "tracking": self.tracking,
                "tracking_mode_enabled": self.tracking_mode_enabled,
                "auto_gimbal": self.auto_gimbal,
                "ai_model": self.ai_model,
                "performance_profile": self.performance_profile,
                "mission_profile": self.mission_profile,
                "stream_mode": self.stream_mode,
                "video_profile": self.video_profile,
                "ai_classes": list(self.ai_classes),
                "confidence": round(self.confidence, 2),
                "ai_fps_target": self.ai_fps_target,
                "ai_resolution": self.ai_resolution,
                "sahi_slice": self.sahi_slice,
                "ai_fps": round(ai_fps, 1),
                "video_fps": 30.0,
                "video_bitrate_mbps": 8.2,

                "target_valid": self.target_valid,
                "target_id": self.target_id,
                "target_class": self.target_class,
                "target_confidence": round(self.target_confidence, 2),
                "target_distance_m": round(self.target_distance_m, 1),
                "target_bearing_deg": round(self.target_bearing_deg, 1),
                "target_latitude": self.target_latitude,
                "target_longitude": self.target_longitude,
            }

    def broadcast(self, message):
        if self.is_frozen():
            return

        data = (json.dumps(message, separators=(",", ":")) + "\n").encode("utf-8")

        with self.lock:
            clients = list(self.clients)

        for client in clients:
            try:
                client.sendall(data)
            except OSError:
                self.remove_client(client)

    def broadcast_status(self):
        self.broadcast({"type": "status", **self.snapshot()})

    def event(self, message, level="info"):
        print(f"[EVENT] {level.upper()}: {message}", flush=True)
        self.broadcast(
            {
                "type": "event",
                "level": level,
                "message": message,
                "ts_ms": int(time.time() * 1000),
                **self.snapshot(),
            }
        )

    def set_status(self, name, value):
        with self.lock:
            if name == "hm30":
                self.hm30_connected = value
            elif name == "a8":
                self.a8_connected = value
            else:
                return

        self.event(f"{name.upper()} {'connected' if value else 'disconnected'}")

    def set_all(self, value):
        with self.lock:
            self.hm30_connected = value
            self.a8_connected = value

        self.event(f"Payload links {'online' if value else 'offline'}")


STATE = AtlasState()


def send_json(sock, message):
    sock.sendall((json.dumps(message, separators=(",", ":")) + "\n").encode("utf-8"))


def clamp(value, minimum, maximum):
    return max(minimum, min(maximum, value))


class AtlasRequestHandler(socketserver.StreamRequestHandler):
    def handle(self):
        client_ip, client_port = self.client_address

        print(f"[MOCK] Cliente ligado: {client_ip}:{client_port}", flush=True)

        self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        STATE.add_client(self.request)

        if not STATE.is_frozen():
            send_json(
                self.request,
                {
                    "type": "hello",
                    "server": "ATLAS-Jetson-Mock",
                    "protocol": 2,
                    **STATE.snapshot(),
                },
            )

        try:
            while True:
                line = self.rfile.readline()
                if not line:
                    break

                try:
                    message = json.loads(line.decode("utf-8"))
                except json.JSONDecodeError as error:
                    print("[MOCK] JSON inválido:", error, flush=True)
                    continue

                if STATE.is_frozen():
                    continue

                message_type = message.get("type", "unknown")

                if message_type == "ping":
                    send_json(
                        self.request,
                        {
                            "type": "pong",
                            "ping_seq": message.get("seq"),
                            "ping_ts_ms": message.get("ts_ms"),
                            **STATE.snapshot(),
                        },
                    )

                elif message_type == "hello":
                    print(f"[MOCK] HELLO: {message}", flush=True)

                elif message_type == "gimbal_rate":
                    yaw = float(message.get("yaw", 0.0))
                    pitch = float(message.get("pitch", 0.0))

                    with STATE.lock:
                        STATE.gimbal_yaw_deg = clamp(
                            STATE.gimbal_yaw_deg + yaw * 0.012,
                            -180.0,
                            180.0,
                        )
                        STATE.gimbal_pitch_deg = clamp(
                            STATE.gimbal_pitch_deg + pitch * 0.010,
                            -90.0,
                            30.0,
                        )

                    print(f"[GIMBAL] yaw={yaw:6.1f} pitch={pitch:6.1f}", flush=True)

                elif message_type == "gimbal_stop":
                    print("[GIMBAL] STOP", flush=True)

                elif message_type == "gimbal_center":
                    with STATE.lock:
                        STATE.gimbal_yaw_deg = 0.0
                        STATE.gimbal_pitch_deg = 0.0
                    STATE.event("Gimbal recentered")

                elif message_type == "gimbal_preset":
                    preset = str(message.get("preset", "FORWARD")).upper()
                    presets = {
                        "FORWARD": (0.0, 0.0),
                        "DOWN": (0.0, -90.0),
                        "LEFT": (-90.0, 0.0),
                        "RIGHT": (90.0, 0.0),
                    }

                    with STATE.lock:
                        if preset in presets:
                            STATE.gimbal_yaw_deg, STATE.gimbal_pitch_deg = presets[preset]
                        elif preset == "TARGET" and STATE.target_valid:
                            STATE.gimbal_yaw_deg = STATE.target_bearing_deg
                            STATE.gimbal_pitch_deg = -20.0
                    STATE.event(f"Gimbal preset {preset}")

                elif message_type == "zoom":
                    direction = int(message.get("value", 0))
                    with STATE.lock:
                        if direction != 0:
                            STATE.zoom_level = clamp(
                                STATE.zoom_level + direction * 0.3,
                                1.0,
                                30.0,
                            )
                    if direction != 0:
                        print(f"[CAMERA] ZOOM {direction}", flush=True)

                elif message_type == "photo":
                    STATE.event("Photo captured")

                elif message_type == "record":
                    enabled = bool(message.get("enabled", False))
                    with STATE.lock:
                        STATE.recording = enabled
                        STATE.recording_started_ms = int(time.time() * 1000) if enabled else 0
                    STATE.event("Recording started" if enabled else "Recording stopped")

                elif message_type == "ai_enable":
                    enabled = bool(message.get("enabled", False))
                    with STATE.lock:
                        STATE.ai_enabled = enabled
                    STATE.event(f"AI {'enabled' if enabled else 'disabled'}")

                elif message_type == "sahi_enable":
                    enabled = bool(message.get("enabled", False))
                    with STATE.lock:
                        STATE.sahi_enabled = enabled
                    STATE.event(f"SAHI {'enabled' if enabled else 'disabled'}")

                elif message_type == "depth_enable":
                    enabled = bool(message.get("enabled", False))
                    with STATE.lock:
                        STATE.depth_enabled = enabled
                    STATE.event(f"Depth {'enabled' if enabled else 'disabled'}")

                elif message_type == "ai_model":
                    model = str(message.get("model", "YOLO11s"))
                    with STATE.lock:
                        STATE.ai_model = model
                    STATE.event(f"AI model: {model}")

                elif message_type == "performance_profile":
                    profile = str(message.get("profile", "BALANCED")).upper()
                    with STATE.lock:
                        STATE.performance_profile = profile
                    STATE.event(f"Performance profile: {profile}")

                elif message_type == "mission_profile":
                    profile = str(message.get("profile", "SEARCH")).upper()
                    with STATE.lock:
                        STATE.mission_profile = profile
                        STATE.ai_enabled = True

                        if profile == "SEARCH":
                            STATE.sahi_enabled = True
                            STATE.depth_enabled = False
                            STATE.auto_gimbal = False
                            STATE.performance_profile = "BALANCED"
                            STATE.ai_fps_target = 10
                        elif profile == "TRACK":
                            STATE.sahi_enabled = False
                            STATE.depth_enabled = True
                            STATE.auto_gimbal = True
                            STATE.performance_profile = "FAST"
                            STATE.ai_fps_target = 20
                        elif profile == "IDENTIFY":
                            STATE.sahi_enabled = False
                            STATE.depth_enabled = True
                            STATE.auto_gimbal = True
                            STATE.performance_profile = "ACCURATE"
                            STATE.ai_resolution = 1280
                            STATE.ai_fps_target = 10

                    STATE.event(f"Mission profile: {profile}")

                elif message_type == "stream_mode":
                    mode = str(message.get("mode", "AI")).upper()
                    with STATE.lock:
                        STATE.stream_mode = mode
                    STATE.event(f"Video mode: {mode}")

                elif message_type == "video_profile":
                    profile = str(message.get("profile", "LOW")).upper()
                    with STATE.lock:
                        STATE.video_profile = profile
                    STATE.event(f"Video profile: {profile}")

                elif message_type == "ai_classes":
                    classes = message.get("classes", [])
                    if isinstance(classes, list):
                        clean = [str(item).upper() for item in classes if str(item).strip()]
                        with STATE.lock:
                            STATE.ai_classes = clean
                        STATE.event("AI classes: " + ", ".join(clean))

                elif message_type == "confidence":
                    value = clamp(float(message.get("value", 0.45)), 0.05, 0.99)
                    with STATE.lock:
                        STATE.confidence = value
                    STATE.broadcast_status()

                elif message_type == "ai_fps_target":
                    value = int(clamp(int(message.get("value", 18)), 1, 60))
                    with STATE.lock:
                        STATE.ai_fps_target = value
                    STATE.broadcast_status()

                elif message_type == "ai_resolution":
                    value = int(clamp(int(message.get("value", 640)), 320, 1920))
                    with STATE.lock:
                        STATE.ai_resolution = value
                    STATE.event(f"AI resolution: {value}")

                elif message_type == "sahi_slice":
                    value = int(clamp(int(message.get("value", 640)), 128, 1280))
                    with STATE.lock:
                        STATE.sahi_slice = value
                    STATE.event(f"SAHI slice: {value}")

                elif message_type == "tracking_mode":
                    enabled = bool(message.get("enabled", False))
                    with STATE.lock:
                        STATE.tracking_mode_enabled = enabled
                        if not enabled:
                            STATE.tracking = False
                            STATE.auto_gimbal = False
                            STATE.target_valid = False
                            STATE.target_id = -1
                            STATE.target_class = ""
                            STATE.target_confidence = 0.0
                            STATE.target_distance_m = 0.0
                            STATE.target_bearing_deg = 0.0
                            STATE.target_latitude = 0.0
                            STATE.target_longitude = 0.0
                    STATE.event(f"Tracking mode {'enabled' if enabled else 'disabled'}")

                elif message_type == "track_point":
                    with STATE.lock:
                        tracking_enabled = STATE.tracking_mode_enabled
                    if not tracking_enabled:
                        STATE.event("Tracking point ignored: tracking mode is OFF", "warning")
                        continue

                    x = float(message.get("x", 0.5))
                    y = float(message.get("y", 0.5))
                    with STATE.lock:
                        STATE.target_valid = True
                        STATE.tracking = True
                        STATE.target_id = 3
                        STATE.target_class = "PERSON"
                        STATE.target_confidence = 0.91
                        STATE.target_distance_m = 37.4
                        STATE.target_bearing_deg = (x - 0.5) * 70.0
                        STATE.target_latitude = 38.702341 + (0.5 - y) * 0.0002
                        STATE.target_longitude = -9.420133 + (x - 0.5) * 0.0002
                    STATE.event("Target #3 selected (PERSON, 37.4 m)")

                elif message_type == "track_roi":
                    with STATE.lock:
                        tracking_enabled = STATE.tracking_mode_enabled
                    if not tracking_enabled:
                        STATE.event("ROI ignored: tracking mode is OFF", "warning")
                        continue

                    with STATE.lock:
                        STATE.target_valid = True
                        STATE.tracking = True
                        STATE.target_id = 3
                        STATE.target_class = "PERSON"
                        STATE.target_confidence = 0.89
                        STATE.target_distance_m = 39.1
                        STATE.target_bearing_deg = 4.0
                        STATE.target_latitude = 38.702341
                        STATE.target_longitude = -9.420133
                    STATE.event("ROI tracking started")

                elif message_type == "auto_gimbal":
                    enabled = bool(message.get("enabled", False))
                    with STATE.lock:
                        STATE.auto_gimbal = enabled and STATE.tracking_mode_enabled and STATE.target_valid
                        applied = STATE.auto_gimbal
                    STATE.event(f"Auto-gimbal {'enabled' if applied else 'disabled'}")

                elif message_type == "tracking_clear":
                    with STATE.lock:
                        STATE.tracking = False
                        STATE.auto_gimbal = False
                        STATE.target_valid = False
                        STATE.target_id = -1
                        STATE.target_class = ""
                        STATE.target_confidence = 0.0
                        STATE.target_distance_m = 0.0
                        STATE.target_bearing_deg = 0.0
                        STATE.target_latitude = 0.0
                        STATE.target_longitude = 0.0
                    STATE.event("Tracking cleared")

                else:
                    print(f"[MOCK] RX: {message}", flush=True)

        finally:
            STATE.remove_client(self.request)
            print(f"[MOCK] Cliente desligado: {client_ip}:{client_port}", flush=True)


class AtlasTcpServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def console_loop():
    print()
    print("Comandos do mock:")
    print("  hm30 on | hm30 off")
    print("  a8 on   | a8 off")
    print("  all on  | all off")
    print("  tracking on | tracking off")
    print("  target on | target off")
    print("  hot on | hot off")
    print("  link bad | link good")
    print("  freeze on | freeze off")
    print("  demo start | demo stop")
    print("  status")
    print()

    while True:
        line = sys.stdin.readline()
        if not line:
            return

        command = line.strip().lower()

        if command == "hm30 on":
            STATE.set_status("hm30", True)
        elif command == "hm30 off":
            STATE.set_status("hm30", False)
        elif command == "a8 on":
            STATE.set_status("a8", True)
        elif command == "a8 off":
            STATE.set_status("a8", False)
        elif command == "all on":
            STATE.set_all(True)
        elif command == "all off":
            STATE.set_all(False)
        elif command == "freeze on":
            STATE.set_frozen(True)
        elif command == "freeze off":
            STATE.set_frozen(False)
        elif command == "hot on":
            with STATE.lock:
                STATE.force_hot = True
            STATE.event("JETSON HOT", "warning")
        elif command == "hot off":
            with STATE.lock:
                STATE.force_hot = False
            STATE.event("Jetson temperature normal")
        elif command == "link bad":
            with STATE.lock:
                STATE.link_bad = True
            STATE.event("HM30 LINK DEGRADED", "warning")
        elif command == "link good":
            with STATE.lock:
                STATE.link_bad = False
            STATE.event("HM30 link recovered")
        elif command == "tracking on":
            with STATE.lock:
                STATE.tracking_mode_enabled = True
            STATE.event("Tracking mode enabled")
        elif command == "tracking off":
            with STATE.lock:
                STATE.tracking_mode_enabled = False
                STATE.tracking = False
                STATE.auto_gimbal = False
                STATE.target_valid = False
                STATE.target_id = -1
                STATE.target_class = ""
            STATE.event("Tracking mode disabled")
        elif command == "target on":
            with STATE.lock:
                STATE.tracking_mode_enabled = True
                STATE.target_valid = True
                STATE.tracking = True
                STATE.target_id = 3
                STATE.target_class = "PERSON"
                STATE.target_confidence = 0.91
                STATE.target_distance_m = 37.4
                STATE.target_bearing_deg = 42.0
                STATE.target_latitude = 38.702341
                STATE.target_longitude = -9.420133
            STATE.event("Target #3 acquired")
        elif command == "target off":
            with STATE.lock:
                STATE.target_valid = False
                STATE.tracking = False
                STATE.auto_gimbal = False
            STATE.event("TARGET LOST", "warning")
        elif command == "demo start":
            with STATE.lock:
                STATE.demo_mode = True
                STATE.hm30_connected = True
                STATE.a8_connected = True
                STATE.recording = True
                STATE.recording_started_ms = int(time.time() * 1000)
                STATE.ai_enabled = True
                STATE.sahi_enabled = True
                STATE.depth_enabled = True
                STATE.tracking_mode_enabled = True
                STATE.tracking = True
                STATE.auto_gimbal = True
                STATE.ai_model = "YOLO11s"
                STATE.performance_profile = "BALANCED"
                STATE.mission_profile = "TRACK"
                STATE.stream_mode = "AI"
                STATE.video_profile = "BALANCED"
                STATE.ai_classes = ["PERSON", "VESSEL", "VEHICLE"]
                STATE.confidence = 0.45
                STATE.ai_fps_target = 18
                STATE.ai_resolution = 640
                STATE.sahi_slice = 640
                STATE.target_valid = True
                STATE.target_id = 3
                STATE.target_class = "PERSON"
                STATE.target_confidence = 0.91
                STATE.target_distance_m = 37.4
                STATE.target_bearing_deg = 42.0
                STATE.target_latitude = 38.702341
                STATE.target_longitude = -9.420133
            STATE.event("SAHI enabled")
            STATE.event("Person detected (0.91, 37.4 m)")
            STATE.event("Auto-gimbal tracking started")
            STATE.event("Target #3 selected (PERSON, 37.4 m)")
            STATE.event("Recording started")
        elif command == "demo stop":
            with STATE.lock:
                STATE.demo_mode = False
                STATE.recording = False
                STATE.tracking_mode_enabled = False
                STATE.tracking = False
                STATE.auto_gimbal = False
                STATE.target_valid = False
            STATE.event("Demo mission stopped")
        elif command == "status":
            print("[MOCK STATUS]", STATE.snapshot(), flush=True)
        elif command:
            print("[MOCK] Comando desconhecido:", command, flush=True)


def demo_loop():
    """Move the simulated target/gimbal while the demo profile is active."""
    while True:
        time.sleep(0.25)

        with STATE.lock:
            if not STATE.demo_mode or not STATE.tracking_mode_enabled:
                continue

            now = time.time()
            phase = now * 0.55

            STATE.target_valid = True
            STATE.tracking = True
            STATE.target_id = 3
            STATE.target_class = "PERSON"
            STATE.target_confidence = 0.91 + 0.02 * math.sin(phase * 0.7)
            STATE.target_distance_m = 37.4 + 2.2 * math.sin(phase)
            STATE.target_bearing_deg = 42.0 + 6.0 * math.sin(phase * 0.8)
            STATE.target_latitude = 38.702341 + 0.000030 * math.sin(phase * 0.55)
            STATE.target_longitude = -9.420133 + 0.000045 * math.cos(phase * 0.55)

            if STATE.auto_gimbal:
                STATE.gimbal_yaw_deg = STATE.target_bearing_deg
                STATE.gimbal_pitch_deg = -8.7 + 1.8 * math.sin(phase * 0.9)


def main():
    parser = argparse.ArgumentParser(description="ATLAS Jetson Payload Mode mock server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=45900)
    args = parser.parse_args()

    with AtlasTcpServer((args.host, args.port), AtlasRequestHandler) as server:
        print(f"[MOCK] ATLAS Jetson server em {args.host}:{args.port}", flush=True)

        thread = threading.Thread(target=console_loop, daemon=True)
        thread.start()

        demo_thread = threading.Thread(target=demo_loop, daemon=True)
        demo_thread.start()

        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print()
            print("[MOCK] Servidor terminado.", flush=True)


if __name__ == "__main__":
    main()
