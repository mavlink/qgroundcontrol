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

## Video Statistics

While a video stream plays, the [Video indicator](fly_view_toolbar.md#video) in the Fly View toolbar shows its frame
rate and bitrate. The indicator turns orange while a stream loses packets, drops frames, or shows less than three
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
  the decoder was replaced after an error or a stall.
- **Display path**: how decoded frames reach the screen: `CPU`, or the zero-copy GPU path (for example `DMABuf`, `GL`
  or `D3D11`). **GPU fallbacks** counts frames the GPU path couldn't import, which took the CPU path instead.
- **Latency**: the delay the video pipeline adds on top of the network (mostly the RTP jitter buffer).
- **Packet loss** and **Jitter** (RTP streams only: UDP h.264/h.265 and RTSP): the share of packets lost over the last
  second (with the number lost since the stream started), and the variation in packet arrival times.
  They are not shown for MPEG-TS streams, nor for UDP streams in **Low Latency Mode**, which has no jitter buffer to
  measure them.
- **Late / duplicate packets** (RTP streams only): packets that arrived after their playout time, and packets that
  arrived twice.
- **Retransmissions** (RTSP): lost packets QGroundControl asked the camera to send again, and how many arrived in time.
- **Uptime**: how long the stream has played since it last connected. **Reconnects** counts the times it came back
  after a loss.
- **Recording**: while recording, how long and how much has been written.

For a MAVLink camera, its advertised frame rate, bitrate and resolution follow the measured ones in brackets.
When a camera also sends a thermal stream, its statistics appear below.

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
of a network stream (under **Video Stream**), the **Decoder Priority** and **Force CPU Video Path** (under
**Decoding**; changing either restarts the video) and the **Grid Lines** and **Display Fit** of the video (under
**Video Display**). **Restart Video** reconnects the streams, and **All Video Settings** opens
[Application Settings > Video](../settings_view/video.md). For a USB camera, only the **Video Display** settings and
**All Video Settings** apply.

## Record Video

If supported by the camera and vehicle, _QGroundControl_ can start and stop video recording on the camera itself. _QGroundControl_ can also record the video stream and save it locally.

:::tip
Video stored on the camera may be of much higher quality, but it is likely that your ground station will have a much larger recording capacity.
:::

### Record Video Stream (on GCS)

Video stream recording is controlled on the [video stream instrument page](hud.md#video_instrument_page).
Press the red circle to start recording a new video (a new video file is created each time the circle is pressed); the circle will change into a red square while recording is in progress.
If the stream drops out while recording, the recording continues once the stream is back, in a new file named after the first with `_part2`, `_part3` and so on added (for example `2026-10-24_10.15.00_part2.mp4`).

A USB camera is recorded the same way, with its telemetry subtitles. QGroundControl encodes it with Qt: as H.264
where the platform's encoder accepts the camera's frames, otherwise with the next codec that does (H.265, AV1, VP9,
MPEG-4 Part 2 or MPEG-2). On Linux, Qt encodes H.264 only on the GPU (VA-API or NVENC), so without one a USB camera
recording is MPEG-4 Part 2. If Qt can't write the recording format's container, another container is used and the file
is named for it. A recording ends when the USB camera, or its format, changes.

Video stream recording is configured in the [Application Settings > General tab](../settings_view/general.md):

- [Video Recording](../settings_view/general.md#video-recording) - specifies the recording file format and storage limits.

  ::: info
  Videos are saved in Matroska format (.mkv) by default.
  This format is relatively robust against corruption in case of errors.
  :::

- [Miscellaneous](../settings_view/general.md#miscellaneous) - Streamed video is saved under the **Application Load/Save Path**.

::: tip
The stored video includes just the video stream itself.
To record video with QGroundControl application elements displayed, you should use separate screen recording software.
:::

### Record Video on Camera

Start/stop video recording _on the camera itself_ using the [camera instrument page](hud.md#camera_instrument_page).
First toggle to video mode, then select the red button to start recording.
