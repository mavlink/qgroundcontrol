# FFmpeg receiver (experimental)

A third network video receiver, `FFmpegVideoReceiver`, written directly on the FFmpeg libraries that the Qt kit
ships, to try in the application whether FFmpeg could replace GStreamer, and to measure it against the GStreamer and
Qt Multimedia receivers. It is a prototype, built for desktop Linux and Android only, and GStreamer stays the backend
the application uses until the FFmpeg one is chosen.

## Build and try it

`QGC_ENABLE_FFMPEG_VIDEO` is on by default, and builds the receiver where it can be built: on desktop Linux and on
Android, with a Qt kit that ships its own FFmpeg libraries and FFmpeg plugin implementation, as the kits of Qt's
installer do, with `make` and `sh` on a host that isn't Windows, and in no cross build but Android's. Any other build
leaves it out, and configure says so (`FFmpeg video backend not built: ...`). A build directory configured before the
option was on keeps it off:

```sh
cmake --preset default -DQGC_ENABLE_FFMPEG_VIDEO=ON
just build
```

The Qt kit has FFmpeg's libraries but not its headers. `CMakeLists.txt` here downloads the source of the FFmpeg
release the kit's libraries are (`QGC_FFMPEG_TAG`, `n9.0.1` for Qt 6.12.0, pinned by `QGC_FFMPEG_SOURCE_SHA256`)
into the CPM cache, and generates the headers into `build/ffmpeg-<tag>` with FFmpeg's `configure` and
`make install-headers`, which compile nothing: configure needs the network once, `sh`, `make` and a C compiler. For
Android, FFmpeg's `configure` is told the target and the NDK's compiler, so it runs nothing of what it compiles.
Configure fails when the headers' major versions differ from those of the kit's libraries: of their sonames, or on
Android, where the libraries' names carry no version, of the version libavcodec and libavformat hold as a string.

Those headers go ahead of every other include directory, and `FFmpegPointers.cc` fails to compile with the headers of
another FFmpeg release: GStreamer's Android SDK has those of its own FFmpeg, whose structs are laid out differently.

On Android, Qt's static library defines an exported `JNI_OnLoad()` in an object file that everything the receiver uses
of the library brings along, and QGroundControl has its own (`src/Android/AndroidInit.cc`). The build therefore links
a copy of the library in which Qt's `JNI_OnLoad()` is a local symbol (`objcopy --localize-symbol`), made in the build
directory. Nothing calls it then: the receiver gives FFmpeg the Java VM itself, and the native methods it would
register are those of Qt's plugin, which registers its own.

In the application, with a stream as the video source, choose **FFmpeg (experimental)** as the Video Backend on the
Video settings page; the streams restart with the new receiver. The page shows the decoder priority, the software
decoder threads and the RTP jitter latency for it as for GStreamer, in a build without GStreamer as well
(`VideoSettings::backendTakesDecoderSettings()`).

## Files

- `FFmpegVideoReceiver`: the `VideoReceiver`. GUI thread: states and reconnects (`StreamSupervisor`), the watchdog,
  the statistics, the recording's `RecordingPlan`, and the hand-off of each frame to the output's `QVideoSink`.
- `FFmpegStreamSession`: one connection, on a thread of its own: opens the stream with libavformat, reads it, decodes
  it with libavcodec while an output displays it, and hands over each frame as it is decoded. A stop interrupts a
  blocking open or read through FFmpeg's interrupt callback.
- `FFmpegStreamRecorder`: a recording, on a thread of its own: the packets the sessions hand it, copied into
  Matroska, MP4 or MOV.
- `FFmpegQtInternals`: the only file that uses Qt's FFmpeg plugin implementation (below).
- `FFmpegPointers`: owners of the FFmpeg objects.

## What it does

- Plays the five stream types of the other receivers: RTSP (`rtsp://`, `rtsps://`; `rtspt://` over TCP, through
  FFmpeg's `rtsp_transport`), H.264 (`udp://`) and H.265 (`udp265://`) RTP over UDP with payload type 96 through an
  SDP description, and MPEG-TS over UDP (`mpegts://`) and TCP (`tcp://`).
- Low delay: no input buffering (`fflags nobuffer`), no stream probing (`avformat_find_stream_info()` is not called:
  the decoder takes the picture's parameters from the stream), slice threads, and each frame shown as soon as it is
  decoded, with no clock; a frame the GUI thread hasn't shown when the next one arrives is dropped. RTP packets out
  of order are waited for the RTP jitter latency setting (FFmpeg's `max_delay`), not at all in Low Latency Mode, which
  also sets the decoder's `low_delay` flag. More than one software decoder thread decodes on frame threads.
- Decodes on VA-API, then NVDEC, and on Android on MediaCodec, where the decoder and the device open, and in software
  otherwise. A forced software decoder or CPU video path (`DisplayConfig::gpuZeroCopy` off) decodes in software. Frames
  in GPU memory stay there: Qt's texture converter shows them, or maps them into memory where it has none for the
  platform.
- MediaCodec: FFmpeg's `h264_mediacodec` and `hevc_mediacodec` decoders, which render to a Surface that Qt's
  `QFFmpeg::getFormat()` gives them, and whose frames Qt shows as an external texture. They are opened with the
  picture's size and parameter sets, taken from the keyframe decoding starts at
  (`FFmpegStreamRecorder::completeParameters()`), and give each frame some milliseconds after its packet: the session
  waits for it for up to half the time between packets (`FFmpegStreamSession::_awaitFrame()`), and hands it over with
  the next packet otherwise. A stream MediaCodec refuses, such as 10-bit 4:4:4 H.264, is decoded in software.
- Reconnects as the Qt Multimedia receiver does: a stream that fails, or sends nothing, or shows no picture, for the
  config's timeout. A UDP port nobody sends to yet is listened on for as long as it takes.
- Records the stream as it arrives, without re-encoding (`encodesRecordings()` is false), from the next keyframe, also
  with no output. MP4 and MOV are fragmented at each keyframe
  (`movflags +frag_keyframe+empty_moov+default_base_moof`), so a crash loses the last fragment only. A reconnect, and
  each hour, continue the recording in the next part.
- Statistics: frame rate, bitrate from the packets, frames dropped, codec, decoder, whether frames are in GPU memory,
  keyframe interval, uptime and reconnects, recording time and size.

## What it doesn't

- RTP statistics: FFmpeg counts lost packets and jitter only in its private RTP state, so `packetsLost`, `lossPercent`
  and `jitterMs` stay -1. No RTP retransmission (RTCP NACK), no jitter buffer beyond FFmpeg's reordering wait.
- The decoder priority's hardware choices: only a forced software decoder is followed.
- `videoConversionElement`, GPU path fallback counters, decoder restarts after a stall: GStreamer's only.
- Platforms other than Linux and Android; NVDEC is untried. Audio.
- Fast software decoding on Android: Qt builds its FFmpeg for Android without assembler code.

## Qt-internal

`FFmpegQtInternals.cc` links `Qt6::FFmpegMediaPluginImplPrivate`, the static library of Qt's FFmpeg media plugin,
whose API is internal to Qt and may change in any release, and `Qt6::MultimediaPrivate`. It uses
`QFFmpegVideoBuffer`, which turns a decoded `AVFrame`, a hardware one included, into a `QVideoFrame`;
`QFFmpeg::getFormat()`, the decoder's format choice that Qt's texture converters need; and
`QVideoFramePrivate::createFrame()`. On Android it also gives MediaCodec's frames a hardware frames context as Qt's
player does, with FFmpeg's own functions. It compiles with Qt 6.12.0 only (`static_assert`). The application then
holds a second copy of the plugin's code beside the plugin itself; its symbols are not exported.

A new Qt version therefore needs the receiver looked at again before it builds: `QGC_FFMPEG_TAG` and its hash follow
the FFmpeg release of the kit (configure fails when they don't match its libraries), and the `static_assert` follows
once the three Qt-internal calls are checked against the new Qt. Until then, set `QGC_ENABLE_FFMPEG_VIDEO` to `OFF`.

## Tests

`test/VideoManager/FFmpeg/FFmpegVideoReceiverStreamTest` plays the streams of `MockVideoStreamServer`, which GStreamer
sends: built with `QGC_ENABLE_FFMPEG_VIDEO` and GStreamer only. So is `VideoReceiverDelayTest`, beside it, which
measures the delay of one stream through the GStreamer, the Qt Multimedia and this receiver; its class comment says
what it reports.
