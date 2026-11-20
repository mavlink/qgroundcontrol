# 视频

当视频流启用时(应用程序设置-视频)， _QGroundControl_ 将在地图底部的“视频切换窗口”中显示当前选中载具的视频流。
You can press the switcher anywhere to toggle _Video_ and _Map_ to foreground.

:::info
视频流已配置/启用在 [应用程序设置 > 常规 > 视频](../settings_view/general.md#video)。
:::

您可以在开关上使用控件来进一步配置视频显示：

- 通过拖动右上角的图标来调整切换器的大小。
- 点击左下方的切换图标来隐藏切换器。
- 按下视频切换器窗口左上角的图标，将其分离
  （分离后，你可以像操作操作系统中的其他窗口一样移动和调整该窗口的大小）。如果您关闭分离的窗口，切换器将重新锁定到 QGC 飞行视图。

When a stream is lost, its last picture stays on screen, dimmed, under **VIDEO LOST - RECONNECTING** until the video
returns. A stream you stop or change is cleared instead.

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
of a network stream (under **Video Stream**), the **Video Backend**, **Decoder Priority** and **Force CPU Video Path**
(under **Decoding**; changing any restarts the video) and the **Grid Lines**,
[Telemetry Overlay](video_overlay.md#live-overlay) and **Display Fit** of the video (under **Video Display**).
**Restart Video** reconnects the streams, and **All Video Settings** opens
[Application Settings > Video](../settings_view/video.md). For a USB camera, only the **Video Display** settings and
**All Video Settings** apply.

## 录像

如果相机和载具支持，_QGroundControl_ 可以在相机上开始和停止视频录制。 _QGroundControl_ 也可以录制视频流并在本地保存。

:::tip
相机上存储的视频质量可能更高，但很可能你的地面站会有大得多的录制容量。
:::

### 录制视频流 (GCS)

Video stream recording is controlled on the [video stream instrument page](camera_tools.md#video_instrument_page).
按下红色圆圈开始录制新视频(每次按下圆圈时创建新视频文件)； 正在录制时，圆圈将会变成红色正方形。
If the stream drops out while recording, the recording continues once the stream is back, in a new file named after the first with `_part2`, `_part3` and so on added (for example `2026-10-24_10.15.00_part2.mp4`).
With the Qt Multimedia [video backend](../settings_view/video.md#settings) a stream is recorded as a USB camera is (below),
from the video on display: it continues in the same file once the stream is back.
Recordings carry the time they started and, as their comment, the _QGroundControl_ version that recorded them.

A USB camera is recorded the same way, with its telemetry subtitles. QGroundControl encodes it with Qt: as H.264
where the platform's encoder accepts the camera's frames, otherwise with the next codec that does (H.265, AV1, VP9,
MPEG-4 Part 2 or MPEG-2). On Linux, Qt encodes H.264 only on the GPU (VA-API or NVENC), so without one a USB camera
recording is MPEG-4 Part 2. **USB Camera Recording Codec** in
[Application Settings > Video](../settings_view/video.md#local-video-storage) picks a codec to try first. If Qt can't
write the recording format's container, another container is used and the file is named for it. A recording ends when
the USB camera, or its format, changes.

视频流录制已配置在 [应用程序设置 > 常规](../settings_view/general.md)：

- [视频录制](../settings_view/general.md#video-recording) - 指定录制文件格式和存储限制。

  ::: 信息
  视频默认保存为 Matroska 格式 (.mkv) 。这种格式在出现错误的情况下，相对不容易损坏。
  :::

- [杂项](../settings_view/general.md#miscellaneous) - 流视频保存在**应用程序加载/保存路径**下。

:::tip
存储的视频仅包括视频流本身。要使用显示的 QGroundControl 应用程序元素录制视频，您应该使用单独的屏幕录制软件。
:::

### 在相机上录制视频

Start/stop video recording _on the camera itself_ using the [camera instrument page](hud.md#camera_instrument_page).
先切换到视频模式，然后选择红色按钮开始录制。
