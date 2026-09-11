# Video

Configure video streaming and recording settings.

## Video Source

- **Source** — Video Stream Disabled / RTSP Video Stream / HTTP MJPEG Video Stream / WebSocket JPEG Video Stream / UDP h.264 / UDP h.265 / TCP-MPEG2 / MPEG-TS / Integrated Camera

## Connection

Connection settings vary by source type:

- **RTSP URL** — full RTSP stream address
- **HTTP MJPEG URL** — full `http://` or `https://` address of a multipart MJPEG stream
- **WebSocket JPEG URL** — full `ws://` or `wss://` address of a JPEG message stream
- **TCP URL** — TCP stream address
- **UDP URL** — UDP stream address and port (default: `0.0.0.0:5600`)

HTTP MJPEG expects a `multipart/x-mixed-replace` response containing JPEG frames.
It does not accept a web page, a single JPEG URL, or an arbitrary HTTP video file.
HTTPS certificate validation remains enabled in GStreamer, redirects are not followed, and URL user information
(`user:password@host`) is rejected.

WebSocket JPEG expects exactly one complete JPEG image in each binary message.
Text messages are ignored. Messages larger than 16 MiB, images over 8,192
pixels in either dimension, and images exceeding the 64 MiB decoded-surface
budget are rejected. The receiver drops older queued images when decoding
falls behind. `wss://` certificates are verified using QGroundControl's
configured trust store.

This source supports unauthenticated endpoints only. Authorization headers are
not supported, credentials embedded in the URL are rejected, and query values
are stored as ordinary settings. Use unencrypted `ws://` only on a trusted
network.

## Settings

- **Aspect Ratio** — aspect ratio for scaling video in the display widget (default: 16:9; set to 0.0 to disable scaling)
- **Stop recording when disarmed** — automatically stop recording when the vehicle disarms
- **Low Latency Mode** — minimize video decode latency at the cost of some smoothness
- **Network Video Timeout** — time before a stalled RTSP, HTTP MJPEG, or WebSocket JPEG source is considered unavailable
- **Auto-reconnect on stream loss** — restart a timed-out or failed stream with exponential backoff
- **Force CPU video path** — disable hardware-accelerated rendering (use if video displays incorrectly)
- **Force video decoder priority** — override the automatic decoder selection: Default / Software / Hardware / NVIDIA / VA-API / DirectX 3D11 / VideoToolbox / Intel / Vulkan

## Local Video Storage

- **Record File Format** — mp4 / mov / mkv
- **Auto-Delete Saved Recordings** — automatically delete old recordings when storage limit is reached
- **Max Storage Usage** — maximum disk space for saved recordings (default: 10240 MB)
