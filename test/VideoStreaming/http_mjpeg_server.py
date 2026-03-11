"""
HTTP MJPEG Test Server for QGroundControl

Generates a test pattern with a moving circle and streams it as
multipart/x-mixed-replace MJPEG over HTTP.

Usage:
    pip install -r requirements.txt
    python http_mjpeg_server.py [--port PORT] [--fps FPS]

Default: http://0.0.0.0:5077/video_feed
"""

import argparse
import time
import math

import cv2
import numpy as np
from flask import Flask, Response

app = Flask(__name__)


def generate_test_frame(width, height, frame_number, fps):
    """Generate a test pattern frame with timestamp and moving circle."""
    frame = np.zeros((height, width, 3), dtype=np.uint8)

    # Grid pattern
    for x in range(0, width, 40):
        cv2.line(frame, (x, 0), (x, height), (40, 40, 40), 1)
    for y in range(0, height, 40):
        cv2.line(frame, (0, y), (width, y), (40, 40, 40), 1)

    # Moving circle
    t = frame_number / fps
    cx = int(width / 2 + (width / 3) * math.cos(t))
    cy = int(height / 2 + (height / 3) * math.sin(t * 0.7))
    cv2.circle(frame, (cx, cy), 30, (0, 255, 0), -1)

    # Crosshair at center
    cv2.line(frame, (width // 2 - 20, height // 2), (width // 2 + 20, height // 2), (0, 0, 255), 2)
    cv2.line(frame, (width // 2, height // 2 - 20), (width // 2, height // 2 + 20), (0, 0, 255), 2)

    # Text overlay
    timestamp = time.strftime("%H:%M:%S")
    cv2.putText(frame, f"QGC MJPEG Test - {timestamp}", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    cv2.putText(frame, f"Frame: {frame_number} | FPS: {fps}", (10, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

    return frame


def generate_mjpeg_stream(fps, width, height):
    """Generator that yields MJPEG frames."""
    frame_number = 0
    frame_interval = 1.0 / fps

    while True:
        start = time.monotonic()

        frame = generate_test_frame(width, height, frame_number, fps)
        _, jpeg = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n'
               b'Content-Length: ' + str(len(jpeg)).encode() + b'\r\n'
               b'\r\n' + jpeg.tobytes() + b'\r\n')

        frame_number += 1

        elapsed = time.monotonic() - start
        remaining = frame_interval - elapsed
        if remaining > 0:
            time.sleep(remaining)


@app.route('/video_feed')
def video_feed():
    return Response(
        generate_mjpeg_stream(app.config['FPS'], app.config['WIDTH'], app.config['HEIGHT']),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )


@app.route('/')
def index():
    return '<h1>QGC MJPEG Test Server</h1><p>Stream: <a href="/video_feed">/video_feed</a></p>'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='HTTP MJPEG Test Server')
    parser.add_argument('--port', type=int, default=5077, help='Server port (default: 5077)')
    parser.add_argument('--fps', type=int, default=30, help='Frames per second (default: 30)')
    parser.add_argument('--width', type=int, default=640, help='Frame width (default: 640)')
    parser.add_argument('--height', type=int, default=480, help='Frame height (default: 480)')
    args = parser.parse_args()

    app.config['FPS'] = args.fps
    app.config['WIDTH'] = args.width
    app.config['HEIGHT'] = args.height

    print(f"Starting MJPEG server on http://0.0.0.0:{args.port}/video_feed")
    print(f"Resolution: {args.width}x{args.height} @ {args.fps} FPS")
    app.run(host='0.0.0.0', port=args.port, threaded=True)
