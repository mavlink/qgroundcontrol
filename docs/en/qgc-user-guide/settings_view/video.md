# Video

Configure video streaming and recording settings.

## Video Source

- **Source** — Video Stream Disabled / RTSP Video Stream / UDP h.264 / UDP h.265 / TCP-MPEG2 / MPEG-TS, or an attached
  USB camera by its name, such as Integrated Camera. The list follows the cameras as they are plugged in and unplugged.
  A selected camera stays selected while it is unplugged, listed as **(not connected)**, and plays again once it is
  plugged back in.
- **USB Camera Tile** (while the source is a stream, unless **Video Tiles** is 0) — **None**, or an attached USB camera
  to show in a [tile](../fly_view/video.md#video-tiles) beside the stream. It lists the cameras as **Source** does, and
  a selected camera likewise stays selected while it is unplugged.
- **USB Camera Format** (USB cameras only) — the resolution, frame rate and pixel format the camera runs in, from the
  ones it offers, best first. **Auto** lets Qt choose: the highest resolution at 30 fps or more, often an MJPEG format,
  which costs CPU time to decode. YUYV and NV12 formats need no decoding but more USB bandwidth.
- **USB Camera Rotation** and **Mirror USB Camera** (USB cameras only) — turn the picture clockwise, for a camera mounted
  on its side or upside down, and mirror it left to right after that. Photos follow; recordings keep the camera's own
  picture.
- **USB Camera White Balance** (USB cameras that can set it) — **Auto**, or set for the light: **Sunlight**, **Cloudy**,
  **Shade**, **Tungsten**, **Fluorescent**, **Flash** or **Sunset**.
- **USB Camera Zoom** (USB cameras that zoom) — from 1x up to the most the camera offers. On Linux Qt scales every
  camera's zoom range to 1x to 2x.
- **USB Camera Exposure Compensation** and **USB Camera Focus** (USB cameras that can set them, not on Linux) — brighten
  or darken the automatic exposure, in EV, and focus automatically, at the hyperfocal distance or at infinity. Qt's
  Linux camera support can't set these.

The camera takes these settings each time it starts. The USB camera settings show while a USB camera is the main
picture: to change them for the camera in a tile, tap its tile first.

When a MAVLink camera advertises its video stream, QGC plays that stream (and any thermal stream it advertises)
instead of the configured source. Your source and connection settings are left unchanged and apply again once no
camera advertises a stream. The camera's other streams, and those of the vehicle's other cameras, play in
[tiles](../fly_view/video.md#video-tiles).

A stream its camera reports as rotated (by 90, 180 or 270 degrees) is shown turned clockwise by that angle, and so are
the photos taken of it; recordings keep the stream as it arrives. The rotation adds to any orientation the stream itself
carries, so a camera should report only one of the two.

## Connection

Connection settings vary by source type:

- **RTSP URL** — full RTSP stream address. An `rtsp://` stream arrives over UDP, or over TCP when no UDP data arrives;
  enter `rtspt://` instead to receive it over TCP from the start, as through a firewall that blocks UDP.
- **TCP URL** — TCP stream address
- **UDP URL** — UDP stream address and port (default: `0.0.0.0:5600`)

## Settings

- **Video backend** — plays network streams with **GStreamer** (default) or **Qt Multimedia**'s player (FFmpeg).
  Qt Multimedia plays RTSP, MPEG-TS over UDP or TCP, and H.264 or H.265 RTP over UDP with payload type 96. It records
  a stream by encoding the video on display anew, as it does a USB camera, so only while the video is shown; it doesn't
  measure a stream's bitrate or packet loss, and the decoder, CPU video path and RTP jitter latency settings don't
  apply to it, so they are hidden. Changing it restarts the video stream. Builds without GStreamer always
  use Qt Multimedia and don't show the setting. When GStreamer fails to start, QGroundControl says so in a message and
  plays the streams with Qt Multimedia, whatever this setting.
- **Aspect Ratio** — aspect ratio for scaling video in the display widget (default: 16:9; set to 0.0 to disable scaling)
- **Video Tiles** — how many other video sources are shown in [tiles](../fly_view/video.md#video-tiles) beside the main
  video: 0 to 4 (default: 3). 0 shows none. A video too narrow for that many shows as many as fit. Each tile takes a
  network connection and a video decoder, of which tablets have few, and its stream takes bandwidth on the vehicle's
  link.
- **Disable Video Stream When Disarmed** — stop the video stream, and any recording, when the vehicle disarms. The stream
  starts again when the vehicle arms, when this setting is turned off, when the active vehicle changes, or when video is
  started again.
- **Low Latency Mode** — minimize video decode latency at the cost of some smoothness
- **RTP jitter latency** — how long RTSP and UDP h.264/h.265 streams hold packets before showing them (default: 80 ms).
  A lower value shortens the video delay; a higher one leaves more time to reorder packets and to recover lost ones by
  retransmission, which values below about 40 ms prevent. Not used, and hidden, in **Low Latency Mode**.
- **Auto-reconnect on stream loss** — reconnect a stream that times out or fails, waiting 1 s before the first attempt
  and up to 30 s between later ones (default: on). It applies to every stream, whatever its transport (RTSP, UDP, TCP or
  MPEG-TS) and whether it is the configured source or one a camera advertises, in a tile or not; USB cameras don't use
  it. A stream
  waiting to reconnect does so at once when a vehicle connects or its link returns. When off, the stream stops after the
  first failure. A UDP source that has received nothing yet hasn't failed: with either setting QGC keeps listening until
  the stream starts. A stream times out when nothing arrives for 3 s, or for 8 s if it is an RTSP stream. Only the RTSP
  time can be changed, with the `rtspTimeout` setting (in seconds, at least 1), which has no control on this page: a
  [settings override file](../../qgc-dev-guide/file_formats/settings_override.md) or a custom build sets it.
- **Force CPU video path** — disable hardware-accelerated rendering (use if video displays incorrectly). Changing it
  restarts the video stream.
- **Force video decoder priority** — override the automatic decoder selection: Default / Software / Hardware / NVIDIA / VA-API / Direct3D / VideoToolbox / Intel / Vulkan. Only the families of the installed decoders are listed, and Vulkan only in builds with the Vulkan GPU path (`QGC_GST_ENABLE_VULKAN`). Changing it restarts the video stream.

## Local Video Storage

- **Record File Format** — mp4 / mov / mkv
- **USB Camera Recording Codec** (USB cameras only) — the codec a USB camera recording tries first, from the ones Qt
  can encode on this computer. **Auto** records with the first of H.264, H.265, AV1, VP9, MPEG-4 Part 2 and MPEG-2 that
  encodes, which also follow a chosen codec the encoder turns down. A codec the recording format's container can't hold
  goes into another container.
- **USB Camera Recording Quality** (USB cameras only) — Very Low / Low / Normal / High / Very High (default: High). A
  lower quality makes a smaller file.
- **Auto-Delete Saved Recordings** — automatically delete the oldest recordings when the storage limit is reached. The
  limit is checked when a recording starts and every minute while one runs. Files being recorded are kept, so a long
  recording can end up with only its latest [parts](../fly_view/video.md#record-video-stream-on-gcs).
- **Max Storage Usage** — maximum disk space for saved recordings (default: 10240 MB)
