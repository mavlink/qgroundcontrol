# Troubleshooting QGC Setup

This topic lists troubleshooting information related to _QGroundControl_ setup and installation on the host computer.

::: tip
Problems when **using** _QGroundControl_ to interact with a vehicle are covered in: [QGC Vehicle Interaction Problems](../troubleshooting/qgc_usage.md).
:::

## Windows: UI Rendering/Video Driver Issues {#opengl_troubleshooting}

If you experience UI rendering issues or video driver crashes on Windows, this may be caused by "flaky" OpenGL drivers. _QGroundControl_ provides 3 shortcuts that you can use to start _QGroundControl_ in "safer" video modes (try these in order):

- **QGroundControl:** QGC uses OpenGL graphics drivers directly.
- **GPU Compatibility Mode:** QGC uses ANGLE drivers, which implement OpenGL on top of DirectX.
- **GPU Safe Mode:** QGC uses a software rasterizer for the UI (this is very slow).
  Video is decoded in software too, unless a hardware decoder is selected in the video settings.

## Windows: Doesn't connect to Vehicle over WiFi {#waiting_for_connection}

If _QGroundControl_ sits forever (for example, _Waiting For Vehicle Connection_) when trying to connect to the vehicle over Wifi, a possible cause is that IP traffic is being blocked by firewall software (e.g. Windows Defender, Norton, etc.).

The solution is to allow the _QGroundControl_ app through the firewall.

::: info
It is possible to simply switch the network profile from Public to Private to allow connections, but this exposes your PC to the Network, so be careful
:::

If using _Windows Defender_:

- In the **Start** bar, enter/select: _Firewall & Network Protection_ (System Settings).
- Scroll to and select the option: _Allow an app through firewall_.
- Select _QGroundControl_ and change the _Access_ selector to **Allow**.

  ::: tip
  Programs are listed in alphabetical order by description (not filename).
  You'll find QGC under **O**: _Open source ground control app provided by QGroundControl dev team_
  :::

## Video Does Not Show {#video_troubleshooting}

The [Video indicator](../fly_view/fly_view_toolbar.md#video) in the Fly View toolbar tells whether a stream arrives:
its dropdown shows the bitrate, decoder, display path and packet loss of each stream
(see [Video Statistics](../fly_view/video.md#video-statistics)).

- **Nothing arrives** (the view says **WAITING FOR VIDEO**, and the dropdown shows no frame rate): check the
  **Source** and its address in [Application Settings > Video](../settings_view/video.md), that no other program
  listens on the same UDP port, and that no firewall blocks the stream (on Windows, allow _QGroundControl_ through the
  firewall as described [above](#waiting_for_connection)). To receive an RTSP stream over TCP, enter its address as
  `rtspt://`.
- **The stream arrives but has no picture**: _QGroundControl_ says in a message why it can't decode the stream. When
  the dropdown names a hardware **Decoder**, set **Force video decoder priority** to **Software**.
- **The picture is wrong** (wrong colours, or a green or torn picture): turn on **Force CPU video path**. If that
  doesn't help, set **Force video decoder priority** to **Software**.
- **_QGroundControl_ says at startup that GStreamer failed to initialize**: it plays the streams with Qt Multimedia
  instead, which the **Video backend** setting can also choose. The application log has the cause.

These settings are described in [Application Settings > Video](../settings_view/video.md#settings). When you report a
video problem, attach the [application log](console_logging.md), with the logging categories
`Video.GStreamer.GstVideoReceiver` and `Video.VideoManager` turned on.
