#!/usr/bin/env python3
"""
WebSocket Video Streaming Test Server for QGroundControl

This script creates a simple WebSocket server that streams JPEG video frames,
allowing developers to test QGC's WebSocket video streaming capabilities
without requiring physical cameras or external hardware.

Usage:
    python websocket_video_server.py [--host HOST] [--port PORT] [--fps FPS]

Example:
    python websocket_video_server.py --host 127.0.0.1 --port 5077 --fps 30

Default URL: ws://127.0.0.1:5077/ws/video_feed
"""

import argparse
import asyncio
import json
import time
from datetime import datetime

import cv2
import numpy as np
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
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
            "QGroundControl WebSocket Test Stream",
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (255, 0, 255),
            2,
            cv2.LINE_AA
        )

        self.frame_count += 1
        return frame


class WebSocketStreamer:
    """Handles WebSocket video streaming."""

    def __init__(self, pattern: VideoTestPattern, quality: int = 85):
        self.pattern = pattern
        self.quality = quality
        self.clients = 0

    async def handle_client_messages(self, websocket: WebSocket, client_id: int):
        """Handle incoming messages from QGC client."""
        try:
            while True:
                message = await websocket.receive_text()
                try:
                    data = json.loads(message)
                    msg_type = data.get("type")

                    if msg_type == "ping":
                        # Respond to heartbeat (QGC expects "pong")
                        await websocket.send_text(json.dumps({"type": "pong"}))
                        timestamp = data.get("timestamp", "")
                        print(f"[Client {client_id}] Heartbeat (ping timestamp: {timestamp})")

                    elif msg_type == "quality":
                        # Handle quality change request (QGC sends "quality", not "setQuality")
                        new_quality = data.get("quality", self.quality)
                        if 1 <= new_quality <= 100:
                            self.quality = new_quality
                            print(f"[Client {client_id}] Quality changed to {new_quality}")
                        else:
                            print(f"[Client {client_id}] Invalid quality: {new_quality}")

                    else:
                        print(f"[Client {client_id}] Unknown message type: {msg_type}")

                except json.JSONDecodeError:
                    print(f"[Client {client_id}] Invalid JSON: {message}")

        except WebSocketDisconnect:
            pass
        except Exception as e:
            print(f"[Client {client_id}] Message handler error: {e}")

    async def stream_frames(self, websocket: WebSocket, client_id: int):
        """Stream video frames to client."""
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

                frame_bytes = buffer.tobytes()

                # Send frame metadata first (QGC/PixEagle protocol)
                metadata = {
                    "type": "frame",
                    "size": len(frame_bytes),
                    "quality": self.quality
                }
                await websocket.send_text(json.dumps(metadata))

                # Then send the actual JPEG frame as binary data
                await websocket.send_bytes(frame_bytes)

                # Maintain frame rate
                elapsed = time.time() - start_time
                sleep_time = max(0, frame_duration - elapsed)
                await asyncio.sleep(sleep_time)

        except WebSocketDisconnect:
            pass
        except Exception as e:
            print(f"[Client {client_id}] Frame sender error: {e}")

    async def stream_to_client(self, websocket: WebSocket):
        """Stream video frames to a WebSocket client with bidirectional communication."""
        self.clients += 1
        client_id = self.clients
        print(f"[Client {client_id}] Connected to WebSocket stream")

        try:
            # Run frame streaming and message handling concurrently
            await asyncio.gather(
                self.stream_frames(websocket, client_id),
                self.handle_client_messages(websocket, client_id)
            )

        except WebSocketDisconnect:
            print(f"[Client {client_id}] Disconnected normally")
        except Exception as e:
            print(f"[Client {client_id}] Disconnected: {e}")
        finally:
            print(f"[Client {client_id}] Stream ended")


def create_app(pattern: VideoTestPattern, quality: int = 85) -> FastAPI:
    """Create FastAPI application."""
    app = FastAPI(
        title="QGroundControl WebSocket Video Test Server",
        description="Streams test video pattern via WebSocket for QGC testing"
    )

    streamer = WebSocketStreamer(pattern, quality)

    @app.get("/")
    async def root():
        """Information endpoint."""
        return {
            "service": "QGroundControl WebSocket Video Test Server",
            "websocket_endpoint": "/ws/video_feed",
            "resolution": f"{pattern.width}x{pattern.height}",
            "fps": pattern.fps,
            "quality": quality,
            "usage": f"ws://{app.state.host}:{app.state.port}/ws/video_feed"
        }

    @app.get("/test", response_class=HTMLResponse)
    async def test_page():
        """Simple HTML test page to view the WebSocket stream in browser."""
        return f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>QGC WebSocket Video Test</title>
            <style>
                body {{
                    font-family: Arial, sans-serif;
                    max-width: 800px;
                    margin: 50px auto;
                    padding: 20px;
                    background-color: #f0f0f0;
                }}
                h1 {{
                    color: #333;
                }}
                #video {{
                    border: 2px solid #333;
                    max-width: 100%;
                }}
                #status {{
                    margin: 10px 0;
                    padding: 10px;
                    background-color: #fff;
                    border-radius: 5px;
                }}
            </style>
        </head>
        <body>
            <h1>QGroundControl WebSocket Video Test</h1>
            <div id="status">Status: Connecting...</div>
            <canvas id="video"></canvas>
            <script>
                const canvas = document.getElementById('video');
                const ctx = canvas.getContext('2d');
                const status = document.getElementById('status');

                const ws = new WebSocket('ws://{app.state.host}:{app.state.port}/ws/video_feed');
                ws.binaryType = 'arraybuffer';

                ws.onopen = () => {{
                    status.textContent = 'Status: Connected';
                    status.style.backgroundColor = '#d4edda';
                }};

                ws.onmessage = (event) => {{
                    const blob = new Blob([event.data], {{type: 'image/jpeg'}});
                    const url = URL.createObjectURL(blob);
                    const img = new Image();
                    img.onload = () => {{
                        canvas.width = img.width;
                        canvas.height = img.height;
                        ctx.drawImage(img, 0, 0);
                        URL.revokeObjectURL(url);
                    }};
                    img.src = url;
                }};

                ws.onerror = (error) => {{
                    status.textContent = 'Status: Error - ' + error;
                    status.style.backgroundColor = '#f8d7da';
                }};

                ws.onclose = () => {{
                    status.textContent = 'Status: Disconnected';
                    status.style.backgroundColor = '#fff3cd';
                }};
            </script>
        </body>
        </html>
        """

    @app.websocket("/ws/video_feed")
    async def websocket_endpoint(websocket: WebSocket):
        """WebSocket video stream endpoint."""
        client_info = f"{websocket.client.host}:{websocket.client.port}" if websocket.client else "unknown"
        print(f"\n{'='*70}")
        print(f"WebSocket connection attempt from: {client_info}")
        print(f"Endpoint: /ws/video_feed")
        print(f"{'='*70}\n")

        await websocket.accept()
        print(f"✓ WebSocket connection accepted from {client_info}\n")

        await streamer.stream_to_client(websocket)

    return app


def main():
    parser = argparse.ArgumentParser(
        description="WebSocket Video Streaming Test Server for QGroundControl"
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
    print("QGroundControl WebSocket Video Test Server")
    print("=" * 70)
    print(f"WebSocket URL: ws://{args.host}:{args.port}/ws/video_feed")
    print(f"Info URL:      http://{args.host}:{args.port}/")
    print(f"Test Page:     http://{args.host}:{args.port}/test")
    print(f"Resolution:    {args.width}x{args.height}")
    print(f"Frame Rate:    {args.fps} FPS")
    print(f"Quality:       {args.quality}%")
    print("=" * 70)
    print("\nConfiguring QGroundControl:")
    print("  1. Open Settings → Video")
    print("  2. Set 'Video Source' to 'WebSocket Video Stream'")
    print(f"  3. Set 'URL' to: ws://{args.host}:{args.port}/ws/video_feed")
    print("  4. Click 'Apply' and view video in the main display")
    print("\nYou can also test in browser:")
    print(f"  Open: http://{args.host}:{args.port}/test")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 70)

    # Run server
    uvicorn.run(app, host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()
