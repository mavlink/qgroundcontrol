"""
WebSocket Video Test Server for QGroundControl

Generates a test pattern and streams JPEG frames over WebSocket.
Protocol: JSON metadata text message followed by binary JPEG frame.

Usage:
    pip install -r requirements.txt
    python websocket_video_server.py [--port PORT] [--fps FPS]

Default: ws://0.0.0.0:5078/ws/video_feed
"""

import argparse
import asyncio
import json
import time
import math

import cv2
import numpy as np
import websockets


def generate_test_frame(width, height, frame_number, fps):
    """Generate a test pattern frame with timestamp and moving circle."""
    frame = np.zeros((height, width, 3), dtype=np.uint8)

    # Grid pattern
    for x in range(0, width, 40):
        cv2.line(frame, (x, 0), (x, height), (40, 40, 40), 1)
    for y in range(0, height, 40):
        cv2.line(frame, (0, y), (width, y), (40, 40, 40), 1)

    # Moving circle (different pattern from HTTP server)
    t = frame_number / fps
    cx = int(width / 2 + (width / 4) * math.sin(t * 1.5))
    cy = int(height / 2 + (height / 4) * math.cos(t))
    cv2.circle(frame, (cx, cy), 25, (255, 165, 0), -1)

    # Crosshair at center
    cv2.line(frame, (width // 2 - 20, height // 2), (width // 2 + 20, height // 2), (0, 0, 255), 2)
    cv2.line(frame, (width // 2, height // 2 - 20), (width // 2, height // 2 + 20), (0, 0, 255), 2)

    # Text overlay
    timestamp = time.strftime("%H:%M:%S")
    cv2.putText(frame, f"QGC WebSocket Test - {timestamp}", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    cv2.putText(frame, f"Frame: {frame_number} | FPS: {fps}", (10, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

    return frame


async def video_handler(websocket):
    """Handle a single WebSocket video client."""
    print(f"Client connected: {websocket.remote_address}")
    frame_number = 0
    fps = 30
    quality = 85
    frame_interval = 1.0 / fps

    try:
        while True:
            start = time.monotonic()

            frame = generate_test_frame(640, 480, frame_number, fps)
            _, jpeg = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, quality])
            jpeg_bytes = jpeg.tobytes()

            # Send JSON metadata first
            metadata = json.dumps({
                "type": "frame",
                "size": len(jpeg_bytes),
                "quality": quality,
                "frame": frame_number,
                "timestamp": time.time()
            })
            await websocket.send(metadata)

            # Send binary JPEG frame
            await websocket.send(jpeg_bytes)

            frame_number += 1

            elapsed = time.monotonic() - start
            remaining = frame_interval - elapsed
            if remaining > 0:
                await asyncio.sleep(remaining)

    except websockets.exceptions.ConnectionClosed:
        print(f"Client disconnected: {websocket.remote_address}")


async def main(port):
    print(f"Starting WebSocket video server on ws://0.0.0.0:{port}/ws/video_feed")
    async with websockets.serve(video_handler, "0.0.0.0", port):
        await asyncio.Future()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='WebSocket Video Test Server')
    parser.add_argument('--port', type=int, default=5078, help='Server port (default: 5078)')
    parser.add_argument('--fps', type=int, default=30, help='Frames per second (default: 30)')
    args = parser.parse_args()

    asyncio.run(main(args.port))
