#!/usr/bin/env python3

from __future__ import annotations

import argparse
import http.server
import socketserver
import time
from typing import cast

from pattern import jpeg_frame


class MjpegHandler(http.server.BaseHTTPRequestHandler):
    server_version = "QGCTestMjpeg/1.0"

    def do_GET(self) -> None:
        if self.path not in ("/", "/video_feed"):
            self.send_error(404)
            return

        self.send_response(200)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Pragma", "no-cache")
        self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=frame")
        self.end_headers()

        server = cast("ThreadedServer", self.server)
        frame_index = 0
        frame_interval = 1.0 / max(1, server.fps)
        while True:
            payload = jpeg_frame(frame_index, server.width, server.height, server.quality)
            try:
                self.wfile.write(b"--frame\r\n")
                self.wfile.write(b"Content-Type: image/jpeg\r\n")
                self.wfile.write(f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii"))
                self.wfile.write(payload)
                self.wfile.write(b"\r\n")
                self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                return
            frame_index += 1
            time.sleep(frame_interval)

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"{self.address_string()} - {fmt % args}")


class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    fps: int
    width: int
    height: int
    quality: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Synthetic multipart MJPEG source for QGC video testing."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5077)
    parser.add_argument("--fps", type=int, default=12)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=360)
    parser.add_argument("--quality", type=int, default=80)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    server = ThreadedServer((args.host, args.port), MjpegHandler)
    server.fps = args.fps
    server.width = args.width
    server.height = args.height
    server.quality = args.quality
    print(f"HTTP MJPEG: http://{args.host}:{args.port}/video_feed")
    server.serve_forever()


if __name__ == "__main__":
    main()
