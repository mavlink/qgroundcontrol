#!/usr/bin/env python3

from __future__ import annotations

import argparse
import asyncio

from pattern import jpeg_frame
from websockets.asyncio.server import ServerConnection, serve


async def stream(websocket: ServerConnection, args: argparse.Namespace) -> None:
    frame_index = 0
    frame_interval = 1.0 / max(1, args.fps)
    while True:
        await websocket.send(jpeg_frame(frame_index, args.width, args.height, args.quality))
        frame_index += 1
        await asyncio.sleep(frame_interval)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Synthetic WebSocket JPEG source for QGC video testing."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5078)
    parser.add_argument("--fps", type=int, default=12)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=360)
    parser.add_argument("--quality", type=int, default=80)
    return parser.parse_args()


async def main_async() -> None:
    args = parse_args()

    async def handler(websocket: ServerConnection) -> None:
        if websocket.request.path != "/ws/video_feed":
            await websocket.close(code=1008, reason="Use /ws/video_feed")
            return
        await stream(websocket, args)

    async with serve(handler, args.host, args.port, max_size=16 * 1024 * 1024):
        print(f"WebSocket JPEG: ws://{args.host}:{args.port}/ws/video_feed")
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main_async())
