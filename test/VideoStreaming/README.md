# HTTP MJPEG and WebSocket JPEG Test Sources

These synthetic sources let reviewers test QGC network video without a camera.
They generate a moving JPEG test pattern and expose it through the same protocols
configured in **Application Settings > Video**.

Python 3.10 or newer is required. The bounded `websockets` version range keeps
the fixture on its current asyncio API instead of the removed legacy server API.

```bash
cd test/VideoStreaming
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
```

## HTTP MJPEG

```bash
python http_mjpeg_server.py --host 127.0.0.1 --port 5077
```

In QGC:

- Source: `HTTP MJPEG Video Stream`
- HTTP MJPEG URL: `http://127.0.0.1:5077/video_feed`
- Authentication: `None`

## WebSocket JPEG

```bash
python websocket_jpeg_server.py --host 127.0.0.1 --port 5078
```

In QGC:

- Source: `WebSocket JPEG Video Stream`
- WebSocket JPEG URL: `ws://127.0.0.1:5078/ws/video_feed`
- Authentication: `None`

Plain `http://` and `ws://` are intended for local lab testing. Use `https://`
or `wss://` before enabling Basic or Bearer authentication.
