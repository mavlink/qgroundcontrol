# Video

Network video streams and USB cameras: their display, recording and statistics. Three receivers play video behind one
interface, `VideoReceiver`: GStreamer's (`GstVideoReceiver`), Qt Multimedia's (`QtMultimediaVideoReceiver`) and the USB
camera's (`UVCCameraController`).

This page is a map of the folders. [GStreamer/README.md](GStreamer/README.md) is the guide to the GStreamer backend,
and also says how the sources and tiles of `VideoManager` behave.

## Where to start reading

1. `VideoManager.h`: its class comment says what the video sources are.
2. `Streams/VideoStreamController.h`: one source.
3. `Receiver/VideoReceiver.h`: what a source asks of its receiver, whichever backend plays it.
4. One receiver. For GStreamer: `GStreamer/GStreamerVideoBackend.h`, then `GStreamer/README.md`.

## Folders

- This folder: `VideoManager`, the QML facade of the sources (the primary stream, the thermal stream, the USB camera
  and the streams in tiles), and `VideoTelemetryOverlay`, the telemetry text shown over the video.
- `Streams/`: `VideoStreamController`, one for each source, and `VideoStreamResolver`, which turns the video settings
  and a camera's advertised stream into a `VideoReceiver::Config`.
- `Recording/`: what is recorded and where. `VideoRecordingService` names the files, starts and stops the sources'
  recordings and keeps the storage limit; `SubtitleWriter` writes the telemetry subtitles beside a recording.
- `Receiver/`: the receiver contract and what its implementations share: `VideoReceiver`, `VideoBackend`, the
  reconnect policy (`StreamSupervisor`), the files of a recording (`RecordingPlan`), screenshots, stream statistics
  and URI schemes. No receiver that plays anything is here: those are in the next three folders.
- `GStreamer/`: the GStreamer backend, listed below.
- `QtMultimedia/`: `QtMultimediaVideoReceiver`, a stream played by `QMediaPlayer`: in builds without GStreamer, by
  choice (`VideoSettings::videoBackend`), or once GStreamer failed to start. Also `QtVideoRecorder`, the
  `QMediaRecorder` that it and the USB camera record with, and `SharedVideoFrame`.
- `UVC/`: the USB camera: `UVCCameraController` (a `QCamera` as a receiver) and `UVCDevices`.
- `Qml/`: the `QGroundControl.VideoManager` QML module: `VideoView`, the tiles, the video outputs, and the statistics
  and settings groups. `src/FlyView`, `src/FlightMap/Widgets`, `src/Toolbar` and the video settings page import it.

Inside `GStreamer/`:

- `GStreamerVideoBackend`: the `VideoBackend`, the only way the app reaches GStreamer.
- `Runtime/`: what must be up before a pipeline runs (initialization, environment, logging, decoder ranks), and the
  helpers the other folders use.
- `Source/`: the source bins, from a stream's URI to its parsed stream (`GstSourceFactory`).
- `Pipeline/`: the receiver, its worker, and a `PipelineSession` with a `DecodingBranch` and a `RecordingBranch`.
- `Sink/`: the display sink: `QGCQVideoSinkController` and the `gstqgc` GStreamer plugin (`Sink/gstqgc/`).
- `HwBuffers/`: how a decoded frame becomes a `QVideoFrame` without a copy: the `HwBuffers` facade, a folder for each
  GPU path, and `common/`, which the paths share.
- `SelfTest/`: the boot self-test and the encoded samples it decodes and records (`Media/`).
- `Platform/`: two C files for Android and iOS.

Video code outside this folder: `src/Settings/VideoSettings`, `src/Camera/QGCVideoStreamInfo` (a stream a camera
advertises), `src/AnalyzeView/VideoRecordings` (the page that plays recordings) and
`src/Comms/MockLink/MockVideoStreamServer` (MockLink's test stream).

## The rules the folders keep

- `VideoSettings` is read by `VideoManager.cc`, `Streams/` and `Recording/VideoRecordingService.cc` only, and the
  vehicle and its cameras are known to this folder, `Streams/` and `Recording/TelemetryBarValues.cc` only. A receiver
  gets a `VideoReceiver::Config` and a `VideoReceiver::DisplayConfig`.
- `Receiver/` names a backend in one place, `VideoBackend::instance()`.
- The backends include each other in two places: `GStreamerVideoBackend` makes `QtMultimediaVideoReceiver`s once
  GStreamer failed to start, and `UVC/` records with `QtMultimedia/QtVideoRecorder`.
- Inside `GStreamer/` the includes point one way: `GStreamerVideoBackend` and `SelfTest/`, then `Pipeline/`, then
  `Source/`, then `Runtime/`.
- `HwBuffers/` and `Sink/gstqgc/` are the static library `QGCGStreamer`, which is not a unity build. Of the folders
  above they include one header, `Runtime/GstScoped.h`. Everything else is compiled into the application, which CI
  builds as unity units: a file-local name must not repeat in another `.cc` of the application.
- With `QGC_ENABLE_GST_VIDEOSTREAMING` off, nothing of `GStreamer/` is built, and streams play through `QtMultimedia/`.

## A frame, from the network to the screen

1. `Streams/VideoStreamResolver` makes a `VideoReceiver::Config` from the video settings, or from the stream a camera
   of the vehicle advertises.
2. `Streams/VideoStreamController` hands it to its receiver: `setConfig()`, `setActive()`, `setVideoOutput()`.
3. `GStreamer/Pipeline/GstVideoReceiver`, on the GUI thread, passes each request to its `GstReceiverWorker`, which
   runs one `PipelineSession` on a thread of its own.
4. `GStreamer/Source/GstSourceFactory` builds the source bin for the URI. It feeds the session's tee.
5. `GStreamer/Pipeline/DecodingBranch` decodes with `decodebin3`, which plugs the decoder that
   `GStreamer/Runtime/GStreamerDecoderRanks` ranked first.
6. `GStreamer/Sink/gstqgc/gstqgcqvideosink` makes a `QVideoFrame` of the decoded frame (`GstQgcFrameMap`): through a
   GPU path of `GStreamer/HwBuffers/`, or on the CPU (`HwBuffers/common/CpuVideoFramePool`).
7. `GStreamer/Sink/gstqgc/GstQgcFrameDelivery` gives the latest frame to the `QVideoSink` that
   `GStreamer/Sink/QGCQVideoSinkController` bound.
8. `Qml/VideoStreamOutput.qml` shows it, inside `VideoView.qml` or `VideoTile.qml`.

The other receivers take fewer steps. `QtMultimedia/QtMultimediaVideoReceiver` has a `QMediaPlayer` play into the
`QVideoSink`. `UVC/UVCCameraController` puts a `QCamera` in a capture session, which `Qml/UVCVideoOutput.qml` shows.

Each layer counts for itself (`GStreamer/Pipeline/PipelineStatsProbe`, `GStreamer/Source/GstRtpPacketProbe`,
`GStreamer/Sink/gstqgc/GstQgcSinkCounters`, `GStreamer/HwBuffers/common/GstHwPathTelemetry`,
`UVC/UVCCameraController`),
and `Receiver/VideoStreamStatsMeter` and `Receiver/VideoStreamStatsSampler` make the `VideoStreamStats` of the
counters that `Qml/VideoStreamStatsGroup.qml` shows.

## A recording

1. `VideoManager::startRecording()` calls `Recording/VideoRecordingService`, which picks the file name, applies the
   storage limit and calls `VideoStreamController::startRecording()` for the main picture and the thermal stream,
   which connects for it if its view is off. While a recording runs the service refuses another start.
2. GStreamer: `GstReceiverWorker` follows a `Receiver/RecordingPlan`, which names the parts a recording continues in,
   and `GStreamer/Pipeline/RecordingBranch` writes each file (parser, muxer, file sink): in Matroska, as `<base>.mkv`,
   when the format asked for can't hold the stream's codec.
3. Qt Multimedia and the USB camera: `QtMultimedia/QtVideoRecorder` encodes the stream's decoded frames, or the
   camera's capture session.
4. `recordingStarted(file)` starts `Recording/SubtitleWriter` for that file, and `recordingFinished()` stops it.
5. `src/AnalyzeView/VideoRecordings` plays the recordings with their subtitles.

## Where settings come in

`VideoManager::_connectSettings()` connects the video settings to the sources, and its comments list every setting:

- The video source, its URLs, the latency and reconnect settings, and which tiles there are: the sources resolve their
  config again, and a receiver restarts for a changed config.
- `videoConversionElement`, `disablePixelAspectRatio`, `forceCpuVideoPath`: the sources restart, which rebuilds the
  display sink.
- `forceVideoDecoder`: the decoder ranks are applied again, and the sources restart.
- `videoBackend`: each stream gets a new receiver.
- The USB camera's settings (`uvcCameraFormat`, `uvcZoom` and the like) go to `UVCCameraController`. The recording
  format and the storage limit are read when a recording starts.

## What is platform-specific

- `GStreamer/HwBuffers/`: `d3d/` is for Windows, `apple/` for macOS and iOS, `dmabuf/` for Linux, `gl/` for Linux and
  Android; `cuda/` (Linux), `android/` and `vulkan/` are built only on request. `GStreamer/HwBuffers/CMakeLists.txt`
  has the table of the paths with their conditions.
- `GStreamer/Platform/`, which `cmake/GStreamer/platform/Android.cmake` and `cmake/platform/Apple.cmake` compile.
- `cmake/GStreamer/`: how each platform's GStreamer SDK is found, downloaded and installed.
- `#if` blocks in `GStreamer/Runtime/GStreamerEnvironment.cc` (the layout of each platform's install) and
  `UVC/UVCCameraController.cc`.

## Tests

`test/VideoManager` has the same folders: the tests of `Receiver/` are in `test/VideoManager/Receiver/`, those of
`GStreamer/Source/` in `test/VideoManager/GStreamer/Source/`. `Qml/` has no folder there: `VideoManagerTest` tests the
QML module. [GStreamer/README.md](GStreamer/README.md#automated-tests) lists the GStreamer classes and says how to
run them.

`VideoReceiverContractTest` (`test/VideoManager/Receiver/`) holds both receivers to what they have in common. Each of
its cases runs against the GStreamer and the Qt Multimedia receiver with the same H.264 RTP stream: a port nobody sends
to yet, a stream lost with and without auto-reconnect, a restart, and a recording ended by stopping the stream. What a
backend does on its own stays in its tests. The Qt Multimedia receiver's are `QtMultimediaVideoReceiverTest`, which
needs neither a stream nor GStreamer, and `QtMultimediaVideoReceiverStreamTest`, which plays the streams of
`MockVideoStreamServer`. That class and the contract test are built only with GStreamer, which sends their streams.
