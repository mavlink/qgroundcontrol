#!/usr/bin/env python3
"""Minimal SIYI A8 UDP mock for ATLAS Direct A8 development.

Implements the command subset used by AtlasSiyiClient. It is deliberately
small and deterministic: enough to validate framing/CRC, gimbal, zoom,
recording state and liveness without A8 hardware.
"""

from __future__ import annotations

import argparse
import math
import socket
import struct
import time
from dataclasses import dataclass

STX = b"\x55\x66"


def crc16_xmodem(data: bytes) -> int:
    crc = 0x0000
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc


def build_frame(command: int, payload: bytes = b"", seq: int = 0, control: int = 0x00) -> bytes:
    head = STX + bytes([control]) + struct.pack("<H", len(payload)) + struct.pack("<H", seq & 0xFFFF) + bytes([command & 0xFF])
    body = head + payload
    return body + struct.pack("<H", crc16_xmodem(body))


def parse_frame(data: bytes) -> tuple[int, int, int, bytes]:
    if len(data) < 10 or data[:2] != STX:
        raise ValueError("invalid SIYI frame header")
    control = data[2]
    length = struct.unpack_from("<H", data, 3)[0]
    seq = struct.unpack_from("<H", data, 5)[0]
    frame_size = 8 + length + 2
    if len(data) != frame_size:
        raise ValueError(f"invalid SIYI frame length: got {len(data)}, expected {frame_size}")
    expected = struct.unpack_from("<H", data, frame_size - 2)[0]
    actual = crc16_xmodem(data[:-2])
    if actual != expected:
        raise ValueError(f"invalid SIYI CRC: got 0x{expected:04x}, expected 0x{actual:04x}")
    return control, seq, data[7], data[8:-2]


@dataclass
class CameraState:
    zoom: float = 1.0
    yaw: float = 0.0
    pitch: float = 0.0
    roll: float = 0.0
    yaw_rate: float = 0.0
    pitch_rate: float = 0.0
    recording: bool = False
    main_codec: int = 2
    main_width: int = 1920
    main_height: int = 1080
    main_bitrate_kbps: int = 6000
    main_fps: int = 25
    record_codec: int = 2
    record_width: int = 1920
    record_height: int = 1080
    record_bitrate_kbps: int = 15000
    record_fps: int = 25
    gimbal_mode: str = "FOLLOW"
    video_output: str = "ETHERNET"
    last_update: float = 0.0

    def update_motion(self) -> None:
        now = time.monotonic()
        if self.last_update == 0.0:
            self.last_update = now
            return
        dt = min(0.5, max(0.0, now - self.last_update))
        self.last_update = now
        # Scale command -100..100 to a plausible mock rate of +/- 90 deg/s.
        self.yaw = max(-135.0, min(135.0, self.yaw + self.yaw_rate * 0.9 * dt))
        self.pitch = max(-90.0, min(25.0, self.pitch + self.pitch_rate * 0.9 * dt))


def i16_tenths(value: float) -> int:
    return int(round(value * 10.0))


def attitude_payload(state: CameraState) -> bytes:
    return struct.pack(
        "<hhhhhh",
        i16_tenths(state.yaw),
        i16_tenths(state.pitch),
        i16_tenths(state.roll),
        i16_tenths(state.yaw_rate * 0.9),
        i16_tenths(state.pitch_rate * 0.9),
        0,
    )


def config_payload(state: CameraState) -> bytes:
    # AtlasSiyiClient only requires byte index 3: record state 0/1.
    return bytes([0, 0, 0, 1 if state.recording else 0, 0, 1, 0])


def zoom_query_payload(state: CameraState) -> bytes:
    whole = int(math.floor(state.zoom))
    decimal = int(round((state.zoom - whole) * 10.0))
    if decimal >= 10:
        whole += 1
        decimal = 0
    return bytes([max(0, min(255, whole)), max(0, min(9, decimal))])


def zoom_ack_payload(state: CameraState) -> bytes:
    return struct.pack("<H", int(round(state.zoom * 10.0)))



def codec_payload(state: CameraState, stream_type: int) -> bytes:
    if stream_type == 0:
        return struct.pack("<BBHHHB", 0, state.record_codec, state.record_width, state.record_height, state.record_bitrate_kbps, state.record_fps)
    return struct.pack("<BBHHHB", 1, state.main_codec, state.main_width, state.main_height, state.main_bitrate_kbps, state.main_fps)

def handle_command(state: CameraState, command: int, payload: bytes) -> tuple[bytes | None, str]:
    state.update_motion()

    if command == 0x01:  # firmware version
        return struct.pack("<III", 0x01020304, 0x05060708, 0x090A0B0C), "firmware query"

    if command == 0x05:  # manual zoom
        direction = struct.unpack("<b", payload[:1])[0] if payload else 0
        if direction > 0:
            state.zoom = min(30.0, state.zoom + 0.1)
        elif direction < 0:
            state.zoom = max(1.0, state.zoom - 0.1)
        return zoom_ack_payload(state), f"zoom {'+' if direction > 0 else '-' if direction < 0 else 'stop'} -> {state.zoom:.1f}x"

    if command == 0x07:  # gimbal rotation speed
        if len(payload) >= 2:
            state.yaw_rate, state.pitch_rate = struct.unpack("<bb", payload[:2])
        return b"\x01", f"gimbal rate yaw={state.yaw_rate:.0f} pitch={state.pitch_rate:.0f}"

    if command == 0x08:  # center
        state.yaw = state.pitch = state.roll = 0.0
        state.yaw_rate = state.pitch_rate = 0.0
        return b"\x01", "gimbal center"

    if command == 0x0A:  # configuration information
        return config_payload(state), f"config recording={state.recording}"

    if command == 0x0C:  # photo / record
        action = payload[0] if payload else 0
        if action == 0:
            return None, "PHOTO"
        if action == 2:
            state.recording = not state.recording
            return None, f"REC {'START' if state.recording else 'STOP'}"
        if action in (3, 4, 5):
            state.gimbal_mode = {3: "LOCK", 4: "FOLLOW", 5: "FPV"}[action]
            return None, f"gimbal mode {state.gimbal_mode}"
        if action in (6, 7, 8):
            state.video_output = {6: "HDMI", 7: "CVBS", 8: "ETHERNET"}[action]
            return None, f"video output {state.video_output}"
        return None, f"camera action {action}"

    if command == 0x0D:  # attitude
        return attitude_payload(state), f"attitude yaw={state.yaw:.1f} pitch={state.pitch:.1f}"

    if command == 0x0E:  # absolute angle control
        if len(payload) >= 4:
            yaw10, pitch10 = struct.unpack_from("<hh", payload, 0)
            state.yaw = max(-135.0, min(135.0, yaw10 / 10.0))
            state.pitch = max(-90.0, min(25.0, pitch10 / 10.0))
            state.yaw_rate = state.pitch_rate = 0.0
        # SDK ACK is yaw/pitch/roll in 0.1 degree units.
        ack = struct.pack("<hhh", i16_tenths(state.yaw), i16_tenths(state.pitch), i16_tenths(state.roll))
        return ack, f"absolute yaw={state.yaw:.1f} pitch={state.pitch:.1f}"

    if command == 0x18:  # current zoom
        return zoom_query_payload(state), f"zoom query {state.zoom:.1f}x"

    if command == 0x20:  # codec specs request
        stream_type = payload[0] if payload else 0
        if stream_type not in (0, 1):
            stream_type = 0
        return codec_payload(state, stream_type), f"codec query stream={stream_type}"

    if command == 0x21:  # codec specs set
        if len(payload) < 9:
            return bytes([0, 0]), "codec set invalid payload"
        stream_type, codec, width, height, bitrate, _reserved = struct.unpack("<BBHHHB", payload[:9])
        ok = codec in (1, 2) and (width, height) in ((1920, 1080), (1280, 720)) and 500 <= bitrate <= 30000
        if ok and stream_type == 0:
            # A8 SDK V0.1.1 says the recording-stream codec is not writable.
            # Accept only if the packet preserves the codec already reported by 0x20.
            ok = codec == state.record_codec
            if ok:
                state.record_width, state.record_height, state.record_bitrate_kbps = width, height, bitrate
        elif ok and stream_type == 1:
            state.main_codec, state.main_width, state.main_height, state.main_bitrate_kbps = codec, width, height, bitrate
        else:
            ok = False
        return bytes([stream_type & 0xFF, 1 if ok else 0]), f"codec set stream={stream_type} {width}x{height} {bitrate}kbps {'OK' if ok else 'REJECT'}"

    return b"", f"unknown cmd=0x{command:02X}"


def serve(host: str, port: int) -> None:
    state = CameraState(last_update=time.monotonic())
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host, port))
    print(f"[MOCK SIYI] listening on udp://{host}:{port}", flush=True)
    print("[MOCK SIYI] Ctrl-C to stop", flush=True)

    while True:
        data, peer = sock.recvfrom(4096)
        try:
            control, seq, command, payload = parse_frame(data)
            ack, message = handle_command(state, command, payload)
            print(f"[SIYI] {peer[0]}:{peer[1]} seq={seq} cmd=0x{command:02X} {message}", flush=True)
            if ack is not None:
                # Responses do not request another ACK.
                sock.sendto(build_frame(command, ack, seq=seq, control=0x02), peer)
        except Exception as exc:
            print(f"[MOCK SIYI] rejected datagram from {peer}: {exc}", flush=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="ATLAS mock SIYI A8 UDP camera")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=37260)
    args = parser.parse_args()
    serve(args.host, args.port)


if __name__ == "__main__":
    main()
