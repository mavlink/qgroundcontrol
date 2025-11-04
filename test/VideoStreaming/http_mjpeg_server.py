#!/usr/bin/env python3
"""
HTTP MJPEG Video Streaming Test Server for QGroundControl

This script creates a simple HTTP server that streams MJPEG video,
allowing developers to test QGC's HTTP video streaming capabilities
without requiring physical cameras or external hardware.

Usage:
    python http_mjpeg_server.py [--host HOST] [--port PORT] [--fps FPS]

Example:
    python http_mjpeg_server.py --host 127.0.0.1 --port 5077 --fps 30

Default URL: http://127.0.0.1:5077/video_feed
"""

import argparse
import asyncio
import time
from datetime import datetime
from typing import Generator

import cv2
import numpy as np
from fastapi import FastAPI
from fastapi.responses import StreamingResponse
import uvicorn


class VideoTestPattern:
    """Generates test video frames with color bars and timestamp."""

    def __init__(self, width: int = 640, height: int = 480, fps: int = 30):
        self.width = width
        self.height = height
        self.fps = fps
        self.frame_count = 0

    def generate_frame(self) -> np.ndarray:
        """Generate a test pattern frame with color bars, moving circle, and timestamp."""
        # Create base frame with color bars
        frame = np.zeros((self.height, self.width, 3), dtype=np.uint8)

        # Define color bars (BGR format)
        colors = [
            (255, 255, 255),  # White
            (0, 255, 255),    # Yellow
            (255, 255, 0),    # Cyan
            (0, 255, 0),      # Green
            (255, 0, 255),    # Magenta
            (0, 0, 255),      # Red
            (255, 0, 0),      # Blue
            (0, 0, 0),        # Black
        ]

        # Draw color bars
        bar_width = self.width // len(colors)
        for i, color in enumerate(colors):
            x1 = i * bar_width
            x2 = (i + 1) * bar_width if i < len(colors) - 1 else self.width
            frame[:self.height // 2, x1:x2] = color

        # Draw moving circle in bottom half
        circle_y = self.height * 3 // 4
        circle_x = int((self.frame_count % (self.width - 40)) + 20)
        cv2.circle(frame, (circle_x, circle_y), 20, (0, 255, 0), -1)

        # Add timestamp
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        cv2.putText(
            frame,
            f"Frame: {self.frame_count} | {timestamp}",
            (10, self.height - 10),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.5,
            (255, 255, 255),
            1,
            cv2.LINE_AA
        )

        # Add QGC test info
        cv2.putText(
            frame,
            "QGroundControl HTTP MJPEG Test Stream",
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (0, 255, 255),
            2,
            cv2.LINE_AA
        )

        self.frame_count += 1
        return frame


class MJPEGStreamer:
    """Handles MJPEG stream generation."""

    def __init__(self, pattern: VideoTestPattern, quality: int = 85):
        self.pattern = pattern
        self.quality = quality
        self.clients = 0

    async def generate_frames(self) -> Generator[bytes, None, None]:
        """Generate MJPEG stream frames."""
        self.clients += 1
        client_id = self.clients
        print(f"[Client {client_id}] Connected to MJPEG stream")

        try:
            frame_duration = 1.0 / self.pattern.fps

            while True:
                start_time = time.time()

                # Generate frame
                frame = self.pattern.generate_frame()

                # Encode as JPEG
                _, buffer = cv2.imencode(
                    '.jpg',
                    frame,
                    [cv2.IMWRITE_JPEG_QUALITY, self.quality]
                )

                # Yield frame in multipart format
                yield (
                    b'--frame\r\n'
                    b'Content-Type: image/jpeg\r\n\r\n' +
                    buffer.tobytes() +
                    b'\r\n'
                )

                # Maintain frame rate
                elapsed = time.time() - start_time
                sleep_time = max(0, frame_duration - elapsed)
                await asyncio.sleep(sleep_time)

        except Exception as e:
            print(f"[Client {client_id}] Disconnected: {e}")
        finally:
            print(f"[Client {client_id}] Stream ended")


def create_app(pattern: VideoTestPattern, quality: int = 85) -> FastAPI:
    """Create FastAPI application."""
    app = FastAPI(
        title="QGroundControl HTTP MJPEG Test Server",
        description="Streams test video pattern via HTTP MJPEG for QGC testing"
    )

    streamer = MJPEGStreamer(pattern, quality)

    @app.get("/")
    async def root():
        return {
            "service": "QGroundControl HTTP MJPEG Test Server",
            "video_feed": "/video_feed",
            "resolution": f"{pattern.width}x{pattern.height}",
            "fps": pattern.fps,
            "quality": quality,
            "usage": f"http://{app.state.host}:{app.state.port}/video_feed"
        }

    @app.get("/video_feed")
    async def video_feed():
        """MJPEG video stream endpoint."""
        return StreamingResponse(
            streamer.generate_frames(),
            media_type="multipart/x-mixed-replace; boundary=frame"
        )

    return app


def main():
    parser = argparse.ArgumentParser(
        description="HTTP MJPEG Video Streaming Test Server for QGroundControl"
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="Host address to bind to (default: 127.0.0.1)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=5077,
        help="Port to bind to (default: 5077)"
    )
    parser.add_argument(
        "--width",
        type=int,
        default=640,
        help="Video width in pixels (default: 640)"
    )
    parser.add_argument(
        "--height",
        type=int,
        default=480,
        help="Video height in pixels (default: 480)"
    )
    parser.add_argument(
        "--fps",
        type=int,
        default=30,
        help="Frames per second (default: 30)"
    )
    parser.add_argument(
        "--quality",
        type=int,
        default=85,
        help="JPEG quality 0-100 (default: 85)"
    )

    args = parser.parse_args()

    # Create test pattern generator
    pattern = VideoTestPattern(args.width, args.height, args.fps)

    # Create FastAPI app
    app = create_app(pattern, args.quality)
    app.state.host = args.host
    app.state.port = args.port

    print("=" * 70)
    print("QGroundControl HTTP MJPEG Test Server")
    print("=" * 70)
    print(f"Video URL:  http://{args.host}:{args.port}/video_feed")
    print(f"Info URL:   http://{args.host}:{args.port}/")
    print(f"Resolution: {args.width}x{args.height}")
    print(f"Frame Rate: {args.fps} FPS")
    print(f"Quality:    {args.quality}%")
    print("=" * 70)
    print("\nConfiguring QGroundControl:")
    print("  1. Open Settings → Video")
    print("  2. Set 'Video Source' to 'HTTP / HTTPS Video Stream'")
    print(f"  3. Set 'URL' to: http://{args.host}:{args.port}/video_feed")
    print("  4. Click 'Apply' and view video in the main display")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 70)

    # Run server
    uvicorn.run(app, host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()
