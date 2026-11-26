# Video

Configure video streaming and recording settings.

## Video Source

- **Source** — Video Stream Disabled / RTSP Video Stream / UDP h.264 / UDP h.265 / TCP-MPEG2 / MPEG-TS, or an attached
  USB camera by its name, such as Integrated Camera. The list follows the cameras as they are plugged in and unplugged.
  A selected camera stays selected while it is unplugged, listed as **(not connected)**, and plays again once it is
  plugged back in. On Android the list holds the cameras Android offers to apps: the device's own, as **Rear Camera**
  and **Front Camera**, and a USB camera, as **External Camera**, only on devices whose Android supports external
  cameras.
- **USB Camera Tile** (while the source is a stream, unless **Video Tiles** is 0) — **None**, or an attached USB camera
  to show in a [tile](../fly_view/video.md#video-tiles) beside the stream. It lists the cameras as **Source** does, and
  a selected camera likewise stays selected while it is unplugged. A camera chosen here starts in the tile, also when
  the one before it was made the main picture with a tap on its tile.
- **USB Camera Format** (USB cameras only) — the resolution, frame rate and pixel format the camera runs in, from the
  ones it offers, best first. **Auto** lets Qt choose: the highest resolution at 30 fps or more, often an MJPEG format,
  which costs CPU time to decode. YUYV and NV12 formats need no decoding but more USB bandwidth. A format chosen for
  another camera stays selected, listed as **(not offered)**, and Qt chooses for this camera.
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

The camera takes these settings each time it starts. The USB camera settings show while a USB camera plays, as the
main picture or in a tile.

When a MAVLink camera advertises its video stream, QGC plays that stream (and any thermal stream it advertises)
instead of the configured source. Your source and connection settings are left unchanged and apply again once no
camera advertises a stream. The camera's other streams, and those of the vehicle's other cameras, play in
[tiles](../fly_view/video.md#video-tiles).

A stream its camera reports as rotated (by 90, 180 or 270 degrees) is shown turned clockwise by that angle, and so are
the photos taken of it; recordings keep the stream as it arrives. The rotation adds to any orientation the stream itself
carries, so a camera should report only one of the two.

GStreamer shows interlaced video (576i or 1080i, as some analog video encoders send) as it is decoded, without
deinterlacing it, so moving edges look like a comb. Set such an encoder to progressive video where it can send that.

## Connection

Connection settings vary by source type:

- **RTSP URL** — full RTSP stream address. An `rtsp://` stream arrives over UDP, or over TCP when no UDP data arrives;
  enter `rtspt://` instead to receive it over TCP from the start, as through a firewall that blocks UDP.
- **TCP URL** — TCP stream address
- **UDP URL** — the local address and port to listen on for a UDP stream (default: `0.0.0.0:5600`). `:5600` alone
  listens on every IPv4 address. With GStreamer, the stream doesn't start while another program is listening on
  the same port.

## Settings

- **Video backend** — plays network streams with **GStreamer** (default) or **Qt Multimedia**'s player (FFmpeg).
  Qt Multimedia plays RTSP, MPEG-TS over UDP or TCP, and H.264 or H.265 RTP over UDP with payload type 96. It records
  a stream by encoding the video on display anew, as it does a USB camera, so only while the video is shown, and with
  the [Recording Codec and Recording Quality](#local-video-storage) settings; it doesn't
  measure a stream's bitrate or packet loss, and the decoder, CPU video path and RTP jitter latency settings don't
  apply to it, so they are hidden. Changing it restarts the video stream. Builds without GStreamer always
  use Qt Multimedia and don't show the setting. When GStreamer fails to start, QGroundControl says so in a message and
  plays the streams with Qt Multimedia, whatever this setting.
- **Aspect Ratio** — the shape the video is shown with: **Auto** (default), 16:9, 4:3, 5:4, 3:2 or 1:1. Auto shows
  the video with the ratio of its frames. Choose a ratio for a camera whose video looks squeezed or stretched, as that
  of an analog camera behind a digital encoder may: the video is then stretched to that ratio, in the main view and
  in its [tile](../fly_view/video.md#video-tiles), and so are the photos taken of it; recordings keep the stream as
  it arrives. It applies to the video source set on this page, not to the streams a vehicle's camera announces, to
  a thermal stream or to a USB camera, and is hidden while a camera's stream plays. Changing between Auto and a
  ratio restarts the video stream.
- **Video Tiles** — how many other video sources are shown in [tiles](../fly_view/video.md#video-tiles) beside the main
  video: 0 to 4 (default: 3). 0 shows none. A video too narrow for that many shows as many as fit. Each tile takes a
  network connection and a video decoder, of which tablets have few, and its stream takes bandwidth on the vehicle's
  link.
- **Disable Video Stream When Disarmed** — keep the video stream off, and stop any recording, while the active vehicle
  is disarmed: also for a vehicle that connects disarmed, and when this setting is turned on while it is disarmed. The
  vehicle's camera is asked to stop sending its video meanwhile, so the stream takes none of the link's bandwidth.
  The stream starts again when the vehicle arms, when this setting is turned off, or when video is started again with
  **Restart Video**, which lasts until the vehicle next arms or disarms.
- **Low Latency Mode** — minimize video decode latency at the cost of some smoothness
- **RTP jitter latency** — how long RTSP and UDP h.264/h.265 streams hold packets before showing them (default: 80 ms).
  A lower value shortens the video delay; a higher one leaves more time to reorder packets and to recover lost ones by
  retransmission, which values below about 40 ms prevent. An MPEG-TS stream over UDP is delayed by this value too,
  and by 100 ms at least. Not used, and hidden, in **Low Latency Mode**.
- **Auto-reconnect on stream loss** — reconnect a stream that times out or fails, waiting 1 s before the first attempt
  and up to 30 s between later ones (default: on). It applies to every stream, whatever its transport (RTSP, UDP, TCP or
  MPEG-TS) and whether it is the configured source or one a camera advertises, in a tile or not; USB cameras don't use
  it. A stream
  waiting to reconnect does so at once when a vehicle connects or its link returns, or when the ground station's own
  network connection comes back. When off, the stream stops after the
  first failure. An RTSP stream whose camera refuses the user name or password in its URL stops with a message whatever
  this setting, and is tried again once the URL changes or the video is restarted.
  A UDP source that has received nothing yet hasn't failed: with either setting QGC keeps listening until
  the stream starts. A stream times out when nothing arrives for 3 s, or for 8 s if it is an RTSP stream. Only the RTSP
  time can be changed, with the `rtspTimeout` setting (in seconds, at least 1), which has no control on this page: a
  [settings override file](../../qgc-dev-guide/file_formats/settings_override.md) or a custom build sets it.
- **Force CPU video path** — disable hardware-accelerated rendering (use if video displays incorrectly). Changing it
  restarts the video stream.
- **Force video decoder priority** — override the automatic decoder selection: Default / Software / Hardware / NVIDIA / VA-API / Direct3D / VideoToolbox / Intel. Only the families of the installed decoders are listed. Changing it restarts the video stream.
- **Software decoder threads** — how many frames a software decoder decodes at once, each on a CPU core of its own:
  1 to 4 (default: 1). A software decoder decodes one frame at a time, on one core unless the camera sends its frames
  in slices. Raise it when the video stutters because the computer is too slow or too busy to decode the stream on one
  core: two threads decode about 1.7 times as fast, four about three times. Each thread delays the video by a frame:
  by 67 ms with 2 threads at 30 frames per second, and by one frame less in **Low Latency Mode**. Hardware decoders
  don't use this setting. Changing it restarts the video stream.
- `videoConversionElement` — the GStreamer element that converts and scales decoded frames before they are shown. Like
  `rtspTimeout` it has no control on this page: a
  [settings override file](../../qgc-dev-guide/file_formats/settings_override.md) or a custom build sets it. Left
  empty (the default), QGroundControl uses the board's own converter where one is installed (`imxvideoconvert_g2d` on
  NXP i.MX, `nvvidconv` on NVIDIA Jetson) and GStreamer's software converter (`videoconvertscale`) elsewhere. Name
  another element, such as `videoconvertscale`, when the board's converter shows the video wrongly; a name that isn't
  installed is ignored. It applies to the GStreamer backend only, and changing it restarts the video stream.

## Video Display

How the video is shown in the Fly View. The [Video indicator](../fly_view/fly_view_toolbar.md#video)'s expanded page
has the same settings.

- **Video Display Fit** — Fit Width / Fit Height / Fill / No Crop (default: Fit Height)
- **Video Grid Lines** — draw a grid over the video
- **Telemetry Overlay** — show the telemetry bar's values along the bottom of the video
  (see [Live Overlay](../fly_view/video_overlay.md#live-overlay))

## Local Video Storage

Recordings are saved in the `Video` folder of the [Application Load/Save Path](general.md#load_save_path). A recording
needs 256 MB free on that storage: with less it isn't started, and one that is running is stopped, with a message.
Free space that can't be read keeps nothing from being recorded.

- **Record File Format** — mp4 / mov / mkv
- **Recording Codec** (USB cameras, and streams with the Qt Multimedia [video backend](#settings)) — the codec a
  recording that QGroundControl encodes tries first, from the ones Qt can encode on this computer. **Auto** records with
  the first of H.264, H.265, AV1, VP9, MPEG-4 Part 2 and MPEG-2 that encodes, which also follow a chosen codec the
  encoder turns down, and a chosen codec Qt can't encode on this computer, listed as **(not available)**. A codec the
  recording format's container can't hold goes into another container. GStreamer saves
  a stream as it arrives, without encoding it again, so this setting and the next don't apply to its recordings and are
  hidden.
- **Recording Quality** (the same recordings) — Very Low / Low / Normal / High / Very High (default: High). A lower
  quality makes a smaller file. On Android these recordings are several times larger than the quality asks for, a defect
  in Qt 6.12: about 340 MB a minute for 1080p at High and 45 MB at Very Low on the tablet tested.
- **Auto-Delete Saved Recordings** — automatically delete the oldest recordings when the storage limit is reached. The
  limit is checked when a recording starts and every minute while one runs. At a start it makes room first: the free
  space a recording needs is looked at once the oldest recordings are deleted. Files being recorded are kept, so a long
  recording can end up with only its latest [parts](../fly_view/video.md#record-video-stream-on-gcs).
- **Max Storage Usage** — maximum disk space for saved recordings (default: 10240 MB)
