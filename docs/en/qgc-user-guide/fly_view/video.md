# Video

When video streaming is enabled (Application Settings - Video), _QGroundControl_ will display the video stream for the currently selected vehicle in the "video switcher window" at the bottom left of the map.
You can press the switcher anywhere to toggle _Video_ and _Map_ to foreground.

::: info
Video streaming is configured/enabled in [Application Settings > General tab > Video](../settings_view/general.md#video).
:::

You can further configure video display using controls on the switcher:

- Resize the switcher by dragging the icon in the top right corner.
- Hide the switcher by pressing the toggle icon in the lower left.
- Detach the video switcher window by pressing on the icon in its top left corner
  (once detached, you can move and resize the window just like any other in your OS).
  If you close the detached window the switcher will re-lock to the QGC Fly view.

When a stream is lost, its last picture stays on screen, dimmed, under **VIDEO LOST - RECONNECTING** until the video
returns. A stream you stop or change is cleared instead.

A video view without a picture says why: **WAITING FOR VIDEO** until the stream arrives, **VIDEO DISABLED WHILE
DISARMED** while [Disable Video Stream When Disarmed](../settings_view/video.md#settings) keeps it off for a disarmed
vehicle, and **VIDEO STOPPED** once the video was stopped.

If a video decoder or a recording stops responding, _QGroundControl_ tells you after about a minute, and a picture
that froze is taken off the screen. That video works again only once _QGroundControl_ is restarted.

When a stream arrives but can't be decoded, as on a ground station that has no decoder for its codec, _QGroundControl_
says why, once. The stream is still received and [can be recorded](#record-video-stream-on-gcs), and its picture
appears as soon as a decoder works: one is tried again after a second, and then at intervals that grow to 30 seconds.

On Android and iOS the video is not decoded while _QGroundControl_ is in the background, which saves battery. Back in
the app, the view says **WAITING FOR VIDEO** until the stream's next keyframe arrives. A recording goes on for as long
as the stream arrives.

On Android, _QGroundControl_ shows a notification while a vehicle is connected or a video is being recorded. With it,
Android keeps the app's network open in the background, where it would otherwise cut it after a few seconds: the stream
stays connected and a recording goes on in the same file. Tap the notification to return to the app. Android 13 and
later ask once whether _QGroundControl_ may show notifications; the stream and the recording go on in the background
without it too. Telemetry is not handled while the app is in the background: the vehicle's messages are handled when
the app returns, and a recording's telemetry subtitles have a gap for that time.

## Thermal Video

A thermal stream that the vehicle's cameras advertise is shown over the video, in the thermal view mode of the
current camera: blended with the video, in its place, or as a small picture in the top left corner.

The thermal stream is the current camera's own. When that camera has none, it is the thermal stream of the first of
the vehicle's other cameras that has one: camera software that presents every stream as a camera of its own, such as
mavlink-camera-manager, sends its thermal stream that way.

When the current camera advertises a thermal stream and no other, that stream is the main picture, in place of the
video source of the settings, and no thermal stream is shown over it.

The vehicle is asked to send the thermal stream only while it is shown or recorded.

## Video Tiles

While the video is in the foreground, _QGroundControl_ shows the other video it can play in small tiles along the top
of the video, each under its name:

1. The USB camera chosen as **USB Camera Tile** in [Application Settings > Video](../settings_view/video.md#video-source),
   while the video source is a stream.
1. The other video streams of the vehicle's current camera: those that are neither on display already nor a thermal
   stream. A thermal stream gets no tile: it is [shown with the video](#thermal-video).
1. The video streams of the vehicle's other MAVLink cameras, in the order the camera tools list the cameras.

At most three tiles are shown. [Video Tiles](../settings_view/video.md#settings) sets how many, up to four, and 0 turns
them off. The tiles are filled in that order: a source there is no tile left for isn't shown, connected to or
decoded. A camera that advertises several streams at one address (the same URL or UDP port) sends one of them at a
time, so only one of those streams gets a picture.

The tiles share the width of the video, and are never smaller than half their full size. Where the video is too narrow
for all of them at that size, as on a phone held upright, it shows as many as fit, in the same order, and the sources
left without a tile aren't connected to. A USB camera that was made the main picture stays the main picture where no
tile fits.

Tap a tile to make it the main picture:

- A camera's stream becomes that camera's current stream, and its camera the current camera, as when you select them in
  the [camera tools](camera_tools.md)' settings. The camera controls, the thermal view, recordings and photos are then
  that camera's and that stream's. The stream that was the main picture takes the tile. Both streams keep playing: they
  aren't connected to again.
- The USB camera becomes the main picture, and the stream takes the camera's tile. Tap the stream's tile to change them
  back. This choice is saved: the camera is the main picture again the next time _QGroundControl_ starts, and each time
  it is plugged back in, until you tap the stream's tile or choose another **USB Camera Tile**.

A tile says **WAITING FOR VIDEO** until its video arrives and **VIDEO LOST - RECONNECTING** while its stream is lost, as
the main picture does. The tiles are not shown in the video switcher window. While the video is there, their streams
aren't decoded, and once it has been there for 15 seconds they are disconnected, so that tiles nobody sees take none
of the link's bandwidth. Back in the foreground within that time, the video finds its tiles playing. After it, and
when _QGroundControl_ starts with the video in the switcher window, the tiles connect once the video is in the
foreground. A USB camera in a tile is turned off and on the same way.

The vehicle is asked to send each stream that has a tile, and to stop it once it has none, so every tile takes some of
the link's bandwidth, as well as a video decoder on the ground station.

Recordings and photos are of the main picture, with its thermal stream: the tiles aren't recorded. A stream that moves
to a tile while it is being recorded goes on being recorded until you stop the recording.

## Video Statistics

While a video stream plays, the [Video indicator](fly_view_toolbar.md#video) in the Fly View toolbar shows its frame
rate and bitrate (its resolution with the Qt Multimedia video backend, which can't measure the bitrate). The indicator turns orange while a stream loses packets, drops frames, or shows less than three
quarters of the frame rate its camera advertises. To check the health of the video link, click the indicator: its
dropdown shows, about once a second:

- **Source** and **URL**: how the stream arrives (RTSP, RTP/UDP, MPEG-TS over UDP or TCP) and its address, without
  user name, password or query values.
- **Camera stream**: the name of the stream a MAVLink camera advertises.
- **Frame rate**: frames shown per second, averaged over the last three seconds.
- **Dropped**: frames dropped since the stream started because display couldn't keep up.
- **Bitrate**: compressed video received, averaged over the last three seconds.
- **Video**: resolution and video format, such as H.264.
- **Keyframe interval**: the time between the last two keyframes. A long interval makes the picture take longer to
  recover from packet loss.
- **Decoder**: the video decoder, and whether it decodes in hardware or software. **Decoder restarts** counts the times
  the decoder was replaced after an error or a stall. A hardware decoder that shows nothing, though the stream gave
  it a keyframe to start from or it had replaced a decoder that failed, is left out from then on, and the next decoder
  takes over. It gets its turn again when the decoder setting changes or QGroundControl restarts.
- **Display path**: how decoded frames reach the screen: `CPU`, or the zero-copy GPU path (for example `DMABuf`, `GL`
  or `D3D11`). **GPU fallbacks** counts frames the GPU path couldn't import, which took the CPU path instead.
- **Latency**: the delay the video pipeline adds on top of the network (mostly the RTP jitter buffer, and the frames
  that [Software decoder threads](../settings_view/video.md#settings) above 1 hold).
- **Packet loss** and **Jitter** (RTP streams only: UDP h.264/h.265 and RTSP): the share of packets lost over the last
  second (with the number lost since the stream started), and the variation in packet arrival times.
  Lost are the packets that never arrived, and those dropped on arrival because more came at once than the
  **RTP jitter latency** holds, as after a link stalls.
  They are not shown for MPEG-TS streams. A UDP stream in **Low Latency Mode** has no jitter buffer to wait for packets:
  one that arrives out of order counts as lost until it arrives.
- **Late / duplicate packets** (RTP streams only): packets that arrived after their playout time, and packets that
  arrived twice. They are not shown for UDP streams in **Low Latency Mode**, which has no jitter buffer to tell them.
- **Retransmissions** (RTSP): lost packets QGroundControl asked the camera to send again, and how many arrived in time.
- **Uptime**: how long the stream has played since it last connected. **Reconnects** counts the times it came back
  after a loss.
- **Recording**: while recording, how long and how much has been written.

For a MAVLink camera, its advertised frame rate, bitrate and resolution follow the measured ones in brackets.
The indicator's figures are the main picture's. The statistics of each stream in a [tile](#video-tiles) appear below
those of the main picture, under the stream's name.
When a [thermal stream](#thermal-video) goes with the video, its statistics appear below. The thermal stream is
connected to only while the thermal view is on or the stream is being recorded: 15 seconds after the view was turned
off it is disconnected, the vehicle is asked to stop sending it, and its statistics go. Turning the view on connects
it again, and shows it from its next keyframe.
GStreamer still records the thermal stream with the video while its view is off (the Qt Multimedia backend records
only video on display): the stream connects for the recording, so its file can start a moment after the video's.

For a USB camera, the indicator shows the frame rate and the resolution (for example `1080p`), and its dropdown shows
under **USB Camera**:

- **Camera**: the camera's name.
- **Frame rate**: frames shown per second, averaged over the last three seconds, followed by the frame rate the camera
  was set up for. Many USB cameras lower their frame rate in dim light to expose longer.
- **Video**: resolution and the format the camera sends, such as MJPEG or YUYV.
- **Display path**: `CPU` for frames in memory, `GPU` for frames that reach the screen as textures.
- **Uptime**: how long the camera has shown video.

QGroundControl tells you when the USB camera reports an error, and tries it again a few times, two seconds apart. It
also tells you when the camera is unplugged, and shows its video again once it is plugged back in.

Expand the dropdown (">") to change, without leaving the Fly View, the **Low Latency Mode** and **RTP jitter latency**
of a network stream (under **Video Stream**), the **Video Backend**, **Decoder Priority** and **Force CPU Video Path**
(under **Decoding**; changing any restarts the video) and the **Grid Lines**,
[Telemetry Overlay](video_overlay.md#live-overlay) and **Display Fit** of the video (under **Video Display**).
**Restart Video** reconnects the streams, and **All Video Settings** opens
[Application Settings > Video](../settings_view/video.md). For a USB camera, only the **Video Display** settings and
**All Video Settings** apply.

## Record Video

If supported by the camera and vehicle, _QGroundControl_ can start and stop video recording on the camera itself. _QGroundControl_ can also record the video stream and save it locally.

:::tip
Video stored on the camera may be of much higher quality, but it is likely that your ground station will have a much larger recording capacity.
:::

### Record Video Stream (on GCS)

Video stream recording is controlled on the [video stream instrument page](camera_tools.md#video_instrument_page).
Press the red circle to start recording a new video (a new video file is created each time the circle is pressed); the circle will change into a red square while recording is in progress.
The file starts with the stream's next keyframe that comes with the stream's parameter sets, as the keyframes of most cameras do.
If the camera sends keyframes but none that a file can start with, QGroundControl reports five seconds later that the recording failed, rather than record nothing.
If the storage fills up, the recording ends and QGroundControl reports that the storage is full; the video stays on display.
If the stream drops out while recording, the recording continues once the stream is back, in a new file named after the first with `_part2`, `_part3` and so on added (for example `2026-10-24_10.15.00_part2.mp4`).
A recording that runs for longer than an hour continues in a new part the same way, without losing a frame between the parts.
An MP4 or MOV part ends sooner when the space the file sets aside for its index is about to run out, which a very high frame rate can cause.
A new part also starts when the camera changes its resolution or encoding profile during a recording, so that each file holds video of one kind.
A stream whose codec the recording format can't hold (MP4 holds neither MJPEG, VP8 nor MPEG-2 video, and MOV no MPEG-2) is recorded as Matroska instead, in files that end in `.mkv`.
With the Qt Multimedia [video backend](../settings_view/video.md#settings) a stream is recorded as a USB camera is (below),
from the video on display: it continues in the same file once the stream is back.
Recordings carry the time they started and, as their comment, the _QGroundControl_ version that recorded them.

A USB camera is recorded the same way, with its telemetry subtitles. QGroundControl encodes it with Qt: as H.264
where the platform's encoder accepts the camera's frames, otherwise with the next codec that does (H.265, AV1, VP9,
MPEG-4 Part 2 or MPEG-2). On Linux, Qt encodes H.264 only on the GPU (VA-API or NVENC), so without one a USB camera
recording is MPEG-4 Part 2. **Recording Codec** in
[Application Settings > Video](../settings_view/video.md#local-video-storage) picks a codec to try first, and
**Recording Quality** how large the file gets; both apply to a stream recorded with the Qt Multimedia video backend as
well. If Qt can't write the recording format's container, another container is used and the file is named for it. A
recording ends when the USB camera, or its format, changes.

Video stream recording is configured in the [Application Settings > General tab](../settings_view/general.md):

- [Video Recording](../settings_view/general.md#video-recording) - specifies the recording file format and storage limits.

  ::: info
  Videos are saved in MP4 format (.mp4) by default; MOV and Matroska (.mkv) can be chosen instead.
  :::

- [Miscellaneous](../settings_view/general.md#miscellaneous) - Streamed video is saved under the **Application Load/Save Path**.

::: tip
The stored video includes just the video stream itself.
To record video with QGroundControl application elements displayed, you should use separate screen recording software.
:::

### Record Video on Camera

Start/stop video recording _on the camera itself_ using the [camera instrument page](hud.md#camera_instrument_page).
First toggle to video mode, then select the red button to start recording.
