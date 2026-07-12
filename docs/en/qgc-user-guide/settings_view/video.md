# Video

Configure video streaming and recording settings.

## Video Source

- **Source** — Video Stream Disabled / RTSP Video Stream / UDP h.264 / UDP h.265 / TCP-MPEG2 / MPEG-TS / HTTP MJPEG / WebSocket JPEG / Integrated Camera

## Connection

Connection settings vary by source type:

- **RTSP URL** — full RTSP stream address
- **TCP URL** — TCP stream address
- **UDP URL** — UDP stream address and port (default: `0.0.0.0:5600`)
- **HTTP MJPEG URL** — full `http://` or `https://` URL for a multipart MJPEG stream
- **WebSocket JPEG URL** — full `ws://` or `wss://` URL for a source that sends each complete JPEG frame as one binary WebSocket message

## Network Video Security

HTTP MJPEG and WebSocket JPEG sources can be anonymous for local labs and trusted test networks. If you select Basic or Bearer authentication, use `https://` or `wss://`; QGC rejects credentials on plaintext HTTP or WS.

- **Authentication** — None, Basic, or Bearer token
- **Username** — Basic authentication username
- **Session credential** — password or token retained only in memory until QGC exits or you clear it
- **Credential file** — optional owner-only file containing one password or token for unattended Unix-like desktop use; other platforms use the in-memory session credential
- **Origin** — optional HTTP/WebSocket Origin header for servers that require one
- **CA certificate file** — optional PEM trust file for HTTPS or WSS validation

Do not put passwords, bearer tokens, or API keys in the video URL. QGC rejects URL user-info and common token query parameters and removes user-info, query, and fragment data from video URL logs.
Automatic redirects are disabled whenever authentication, Origin, or a custom CA is configured so credentials and trust policy cannot be silently forwarded to another endpoint.

## Test Sources

Synthetic HTTP MJPEG and WebSocket JPEG servers are available in `test/VideoStreaming/`.
They generate moving JPEG frames and do not require a camera.

```bash
cd test/VideoStreaming
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
python http_mjpeg_server.py --host 127.0.0.1 --port 5077
```

Use `http://127.0.0.1:5077/video_feed` with the HTTP MJPEG source. For WebSocket JPEG, run `python websocket_jpeg_server.py --host 127.0.0.1 --port 5078` and use `ws://127.0.0.1:5078/ws/video_feed`.

## Settings

- **Aspect Ratio** — aspect ratio for scaling video in the display widget (default: 16:9; set to 0.0 to disable scaling)
- **Stop recording when disarmed** — automatically stop recording when the vehicle disarms
- **Low Latency Mode** — minimize video decode latency at the cost of some smoothness
- **Force CPU video path** — disable hardware-accelerated rendering (use if video displays incorrectly)
- **Force video decoder priority** — override the automatic decoder selection: Default / Software / Hardware / NVIDIA / VA-API / DirectX 3D11 / VideoToolbox / Intel / Vulkan

## Local Video Storage

- **Record File Format** — mp4 / mov / mkv
- **Auto-Delete Saved Recordings** — automatically delete old recordings when storage limit is reached
- **Max Storage Usage** — maximum disk space for saved recordings (default: 10240 MB)
