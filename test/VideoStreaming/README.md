# Video Streaming Test Servers

Test servers for validating HTTP MJPEG and WebSocket video streaming in QGroundControl.

## Setup

```bash
pip install -r requirements.txt
```

## HTTP MJPEG Server

```bash
python http_mjpeg_server.py --port 5077 --fps 30
```

In QGC: Select "HTTP MJPEG Video Stream", set URL to `http://127.0.0.1:5077/video_feed`

## WebSocket Server

```bash
python websocket_video_server.py --port 5078 --fps 30
```

In QGC: Select "WebSocket Video Stream", set URL to `ws://127.0.0.1:5078/ws/video_feed`

## Protocol

### HTTP MJPEG
Standard `multipart/x-mixed-replace` with `boundary=frame`. Each part contains a JPEG image with `Content-Type: image/jpeg`.

### WebSocket
1. Server sends JSON text: `{"type":"frame","size":N,"quality":Q}`
2. Server sends binary JPEG data
3. Client can send `ping` for keepalive

Both servers generate a test pattern with a moving circle, crosshair, timestamp, and frame counter.
