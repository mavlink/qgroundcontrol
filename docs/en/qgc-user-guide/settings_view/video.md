# Video

Configure video streaming and recording settings.

## Video Source

- **Source** — Video Stream Disabled / RTSP Video Stream / UDP h.264 / UDP h.265 / TCP-MPEG2 / MPEG-TS / Integrated Camera

When a MAVLink camera advertises its video stream, QGC plays that stream (and any thermal stream it advertises)
instead of the configured source. Your source and connection settings are left unchanged and apply again once no
camera advertises a stream.

## Connection

Connection settings vary by source type:

- **RTSP URL** — full RTSP stream address
- **TCP URL** — TCP stream address
- **UDP URL** — UDP stream address and port (default: `0.0.0.0:5600`)

## Settings

- **Aspect Ratio** — aspect ratio for scaling video in the display widget (default: 16:9; set to 0.0 to disable scaling)
- **Disable Video Stream When Disarmed** — stop the video stream, and any recording, when the vehicle disarms. The stream
  starts again when the vehicle arms, when this setting is turned off, when the active vehicle changes, or when video is
  started again.
- **Low Latency Mode** — minimize video decode latency at the cost of some smoothness
- **RTP jitter latency** — how long RTSP and UDP h.264/h.265 streams hold packets before showing them (default: 80 ms).
  A lower value shortens the video delay; a higher one leaves more time to reorder packets and to recover lost ones by
  retransmission, which values below about 40 ms prevent. Not used, and hidden, in **Low Latency Mode**.
- **Auto-reconnect on stream loss** — reconnect a stream that times out or fails, waiting 1 s before the first attempt
  and up to 30 s between later ones (default: on). When off, the stream stops after the first failure.
- **Force CPU video path** — disable hardware-accelerated rendering (use if video displays incorrectly). Changing it
  restarts the video stream.
- **Force video decoder priority** — override the automatic decoder selection: Default / Software / Hardware / NVIDIA / VA-API / DirectX 3D11 / VideoToolbox / Intel / Vulkan. Changing it restarts the video stream.

## Local Video Storage

- **Record File Format** — mp4 / mov / mkv
- **Auto-Delete Saved Recordings** — automatically delete old recordings when storage limit is reached
- **Max Storage Usage** — maximum disk space for saved recordings (default: 10240 MB)
