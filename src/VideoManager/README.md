# Video

Network video streams and USB cameras: their display, recording and statistics. Three receivers play video behind one
interface, `VideoReceiver`: GStreamer's (`GstVideoReceiver`), Qt Multimedia's (`QtMultimediaVideoReceiver`) and the USB
camera's (`UVCCameraController`). A desktop Linux or Android build has a fourth, experimental one on FFmpeg
(`FFmpegVideoReceiver`, [FFmpeg/README.md](FFmpeg/README.md)).

This page is a map of the folders and the rules of the application layer. [GStreamer/README.md](GStreamer/README.md) is
the guide to the GStreamer backend.

## Where to start reading

1. `VideoManager.h`: its class comment says what the video sources are.
2. `Streams/VideoStreamController.h`: one source.
3. `Receiver/VideoReceiver.h`: what a source asks of its receiver, whichever backend plays it.
4. One receiver. For GStreamer: `GStreamer/GStreamerVideoBackend.h`, then `GStreamer/README.md`.

## Folders

- This folder: `VideoManager`, the QML facade of the sources (the primary stream, the thermal stream, the USB camera
  and the streams in tiles).
- `Streams/`: `VideoStreamController`, one for each source; `VideoStreamResolver`, which turns the video settings
  and a camera's advertised stream into a `VideoReceiver::Config`; and `VehicleVideoStreams`, which reads the streams
  the active vehicle's cameras advertise and tells the cameras which of them play.
- `Recording/`: what is recorded and where. `VideoRecordingService` names the files, starts and stops the sources'
  recordings and keeps the storage limit; `SubtitleWriter` writes the telemetry subtitles beside a recording.
- `Telemetry/`: `TelemetryBarValues`, the values the Fly View's telemetry bar shows, which the subtitles show too,
  and `VideoTelemetryOverlay`, the telemetry text shown over the video.
- `Receiver/`: the receiver contract and what its implementations share: `VideoReceiver`, `VideoBackend` and the
  run of its initialization (`VideoBackendInit`), the reconnect policy (`StreamSupervisor`), the watchdog that tells
  when a stream has stalled (`StreamWatchdog`), the files of a recording (`RecordingPlan`), screenshots, stream
  statistics, and the URI schemes with what FFmpeg opens for each (`VideoStreamUri`). No receiver that plays
  anything is here: those are in the next four folders.
- `GStreamer/`: the GStreamer backend, whose folders [its README](GStreamer/README.md#source-code-architecture)
  lists.
- `QtMultimedia/`: `QtMultimediaVideoReceiver`, a stream played by `QMediaPlayer`: in builds without GStreamer, by
  choice (`VideoSettings::videoBackend`), or once GStreamer failed to start. Also `QtVideoRecorder`, the
  `QMediaRecorder` that it and the USB camera record with, and `SharedVideoFrame`.
- `FFmpeg/`: `FFmpegVideoReceiver`, experimental, on desktop Linux and Android (`QGC_ENABLE_FFMPEG_VIDEO`): a stream
  received directly with the FFmpeg libraries of the Qt kit, by choice (`VideoSettings::videoBackend`). Its
  `FFmpegStreamSession` reads and decodes, its `FFmpegStreamRecorder` records, and `FFmpegQtInternals` is its use of
  Qt's FFmpeg plugin implementation, which is Qt-internal.
- `UVC/`: the USB camera: `UVCCameraController` (a `QCamera` as a receiver) and `UVCDevices`.
- `Qml/`: the `QGroundControl.VideoManager` QML module: `VideoView`, the tiles, the video outputs, the camera tracking
  controller (`OnScreenCameraTrackingController`), and the statistics and settings groups. `src/FlyView`,
  `src/FlightMap/Widgets`, `src/Toolbar` and the video settings page import it.

Video code outside this folder: `src/Settings/VideoSettings`, `src/Camera/QGCVideoStreamInfo` (a stream a camera
advertises), `src/AnalyzeView/VideoRecordings` (the page that plays recordings) and
`src/Comms/MockLink/MockVideoStreamServer` (MockLink's test stream).

## The rules the folders keep

- `VideoSettings` is read by `VideoManager.cc`, `Streams/`, `Recording/VideoRecordingService.cc` and
  `Receiver/VideoBackend.cc` (the backend setting) only, and the vehicle and its cameras are known to `Telemetry/`
  and `Streams/` only. A receiver gets a `VideoReceiver::Config` and a `VideoReceiver::DisplayConfig`.
- `Receiver/` names the backends in one file, `VideoBackend.cc`: `VideoBackend::instance()` is GStreamer's backend
  where the build has it, and `VideoBackend::createReceiver()` chooses a stream's receiver: GStreamer's where the build
  has it, it hasn't failed to start and the backend setting asks for it, FFmpeg's where the build has it and the
  setting asks for it, else Qt Multimedia's. `VideoManager` reaches it through `QGCCorePlugin::createVideoReceiver()`,
  which a custom build overrides to supply its own.
- The backends include each other in one place: `UVC/` records with `QtMultimedia/QtVideoRecorder`.
- With `QGC_ENABLE_GST_VIDEOSTREAMING` off, nothing of `GStreamer/` is built, and streams play through `QtMultimedia/`.
- With `QGC_ENABLE_FFMPEG_VIDEO` off, and in every build but a desktop Linux or Android one, nothing of `FFmpeg/` is
  built.
- Inside `GStreamer/`, [its README](GStreamer/README.md#source-code-architecture) says which way the includes point.

## Sources and tiles

`VideoManager` is the QML facade of the video sources, each a `VideoStreamController` with a receiver of its own: the
primary stream, the thermal stream, the USB camera, and one for each other stream shown in a tile beside the main
picture (`Qml/VideoTiles.qml`, `Qml/VideoTile.qml`). Its class comment says which streams take the tiles.

- There are no tiles, and so none of their sources, until a row of tiles that is shown tells its room
  (`VideoManager::tileRoom`): the Fly View shows the row only while the video is its main view, not in the
  picture-in-picture. A row that is no longer shown tells that it has no room once it has been out of view for 15 s
  (`VideoStreamController::kIdleGrace`), so a video that changes places with the map and back within that time finds
  the tiles' streams playing.
- A stream at an address (its URI, or the UDP port it is listened for on) that a stream on display already has gets no
  source: a camera sends such streams one at a time.
- When the vehicle's current stream becomes one that plays in a tile, as after `VideoManager::selectSource()`, that
  tile's source becomes the primary stream and the former primary stream takes the tile: both keep their receivers,
  and neither stream restarts. A stream that plays in no tile is played by the primary stream, whose receiver connects
  to it.
- A QML `VideoOutput` registers with the source it shows (`Qml/VideoStreamOutput.qml`). One that registers as another
  one leaves takes the source over, so a source moves between the main picture and a tile without its receiver being
  without an output, which would take the decoder out of its pipeline. A tile's source has an output only while the
  tile is shown: hidden, its stream is received but not decoded, until its row tells that it has no room.
- The thermal stream is the active vehicle's: its current camera's (`thermalStreamInstance()`) or, when that camera
  has none, that of the first of its other cameras that has one. That is how mavlink-camera-manager sends it, which
  makes a camera component of each stream, with stream id 1, and flags the thermal one `THERMAL` and not `RUNNING`. A
  current camera that advertises thermal streams only has no current stream: the primary stream then plays its
  thermal stream, in place of the video source of the settings, and there is no thermal stream
  (`VehicleVideoStreams::_readStreams()`).
- The thermal stream plays only while it is in use: while its view is on, which gives it an output, and from
  `startRecording()` until its recording has ended. Out of use for `kIdleGrace` its receiver is deactivated, and
  activated again with the next use: its picture then waits for the stream to connect and send a keyframe.
- On Android and iOS no source has an output while the app is in the background (`Qt::ApplicationHidden` or
  `Qt::ApplicationSuspended`): nothing is decoded there. The streams are still received and recorded, for as long as
  the system lets them arrive: on Android, a foreground service keeps the app's network up while a recording runs
  (`AndroidInterface::setBackgroundWork()`).
- The vehicle's cameras are told which of their streams play besides their current one
  (`MavlinkCameraControlInterface::setExtraStreams()`): those in tiles, and a camera's thermal stream while the source
  that plays it wants it (`VideoStreamController::wantsStream()`), which the thermal stream out of use doesn't.
  `VehicleCameraControl` asks the camera to transmit them (`MAV_CMD_VIDEO_START_STREAMING`) and to stop those that left
  (`MAV_CMD_VIDEO_STOP_STREAMING`), and neither stops nor starts a stream that stays on display when the current stream
  changes.
- While the video is off, stopped (`VideoManager::stopVideo()`) or disabled for a disarmed vehicle
  (`VideoSettings::disableWhenDisarmed`), the vehicle's current camera is told to stop transmitting its stream
  (`stopStream()`), and none of its streams is named as an extra one: a stream pushed to a UDP port would otherwise
  cross the vehicle's link with nobody receiving it. The camera is told to resume (`resumeStream()`) once the video
  plays again.
- Recordings and photos are of the main picture and its thermal stream, not of the tiles.

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
   GPU path of `GStreamer/HwBuffers/`, or on the CPU, mapped in place.
7. `GStreamer/Sink/gstqgc/GstQgcFrameDelivery` gives the latest frame to the `QVideoSink` that
   `GStreamer/Sink/QGCQVideoSinkController` bound.
8. `Qml/VideoStreamOutput.qml` shows it, inside `VideoView.qml` or `VideoTile.qml`.

The other receivers take fewer steps. `QtMultimedia/QtMultimediaVideoReceiver` has a `QMediaPlayer` play into the
`QVideoSink`. `FFmpeg/FFmpegStreamSession` reads the stream with libavformat and decodes it with libavcodec on a thread
of its own, Qt's `QFFmpegVideoBuffer` makes a `QVideoFrame` of each frame (`FFmpeg/FFmpegQtInternals`), and
`FFmpeg/FFmpegVideoReceiver` sets the newest on the `QVideoSink` on the GUI thread. `UVC/UVCCameraController` puts a
`QCamera` in a capture session, which plays into it. Every source's output is a `Qml/VideoStreamOutput.qml`, registered
with the source's `VideoStreamController`.

Each layer counts for itself (`GStreamer/Pipeline/PipelineStatsProbe`, `GStreamer/Source/GstRtpPacketProbe`,
`GStreamer/Sink/gstqgc/GstQgcSinkStats`, `GStreamer/HwBuffers/common/GstHwPathTelemetry`, and the other receivers
themselves), and `Receiver/VideoStreamStatsMeter` and `Receiver/VideoStreamStatsSampler` make the `VideoStreamStats` of
the counters that `Qml/VideoStreamStatsGroup.qml` shows. `Receiver/KeyframeIntervalMeter` is the keyframe interval of
the GStreamer and FFmpeg receivers.

## A recording

1. `VideoManager::startRecording()` calls `Recording/VideoRecordingService`, which picks the file name, applies the
   storage limit and calls `VideoStreamController::startRecording()` for the main picture and the thermal stream,
   which connects for it if its view is off, so that its file starts a moment after the main picture's. While a
   recording runs the service refuses another start, with a warning logged. It refuses one as well, and stops a
   recording that runs, while less than `VideoRecordingService::kFreeSpaceFloor` (256 MiB) is free on the volume of the
   video save directory: it looks at a start and every 10 s while recording, and a start short of space first waits
   for the storage limit's pruning, which runs on the thread pool. Free space that isn't known stops nothing.
2. GStreamer: `GstReceiverWorker` follows a `Receiver/RecordingPlan`, which names the parts a recording continues in,
   and `GStreamer/Pipeline/RecordingBranch` writes each file (parser, muxer, file sink): in Matroska, as `<base>.mkv`,
   when the format asked for can't hold the stream's codec.
3. Qt Multimedia and the USB camera: `QtMultimedia/QtVideoRecorder` encodes the stream's decoded frames, or the
   camera's capture session. A start they refuse, their recorder being in use, comes back as `recordingFinished()`
   with the reason.
4. FFmpeg: `FFmpeg/FFmpegVideoReceiver` follows a `Receiver/RecordingPlan` too, and `FFmpeg/FFmpegStreamRecorder`
   copies the stream's packets into each file with libavformat's muxers, on a thread of its own.
5. `recordingStarted(file)` starts `Recording/SubtitleWriter` for that file, and `recordingFinished()` stops it.
6. `src/AnalyzeView/VideoRecordings` plays the recordings with their subtitles.

## Where settings come in

`VideoManager::_connectSettings()` connects the video settings to the sources, and its comments list every setting:

- The video source, its URLs, the latency and reconnect settings, and which tiles there are: the sources resolve their
  config again, and a receiver restarts for a changed config.
- `videoConversionElement`, `forceCpuVideoPath`: the sources restart, which rebuilds the display sink.
- `forcedAspectRatio`: each `VideoStreamController` follows it. The manually configured stream's outputs
  (`Qml/VideoStreamOutput.qml`) stretch its picture to the ratio, and a screenshot with it; its display sink is then
  built without the scaling to square pixels, so the stream restarts when the setting changes between Auto and a
  ratio.
- `forceVideoDecoder`: the decoder ranks are applied again, and the sources restart.
- `videoBackend`: each stream gets a new receiver.
- The USB camera's settings (`uvcCameraFormat`, `uvcZoom` and the like) go to `UVCCameraController`. The recording
  settings are read when a recording starts, the storage limit also each minute while one runs.

## What is platform-specific

- `GStreamer/HwBuffers/`: `d3d/` is for Windows, `apple/` for macOS and iOS, `dmabuf/` for Linux, `gl/` for Linux and
  Android; `android/` is built only on request. `GStreamer/HwBuffers/CMakeLists.txt` has the table of the paths with
  their conditions.
- `GStreamer/Platform/`, which `cmake/GStreamer/platform/Android.cmake` and `cmake/platform/Apple.cmake` compile.
- `cmake/GStreamer/`: how each platform's GStreamer SDK is found, downloaded and installed.
- `#if` blocks in `GStreamer/Runtime/GStreamerEnvironment.cc` (the layout of each platform's install) and
  `UVC/UVCCameraController.cc`.
- `FFmpeg/`: built for Linux and Android only; its hardware decoders are theirs: VA-API and NVDEC, and MediaCodec.

## Tests

`test/VideoManager` has the same folders: the tests of `Receiver/` are in `test/VideoManager/Receiver/`, those of
`GStreamer/Source/` in `test/VideoManager/GStreamer/Source/`. Those of `Qml/` are in `test/VideoManager/Qml/`:
`VideoQmlTest` shows the views, the tiles, the overlays and the Video indicator's groups with a `VideoManager`, and
`VideoStreamOutputRenderTest` reads back what Qt Quick draws of a `VideoStreamOutput`. `VideoManagerTest` tests the
manager and its sources; it and `VideoQmlTest` share their fixture, `VideoManagerTestBase`.
[GStreamer/README.md](GStreamer/README.md#automated-tests) lists the GStreamer classes and says how to run them.

`VideoReceiverContractTest` (`test/VideoManager/Receiver/`) holds the receivers to what they have in common: each of
its cases runs against the GStreamer and the Qt Multimedia receiver, and the experimental FFmpeg receiver where it is
built, with the same H.264 RTP stream. What a backend does
on its own stays in its tests. The Qt Multimedia receiver's are `QtMultimediaVideoReceiverTest`, which needs neither a
stream nor GStreamer, and `QtMultimediaVideoReceiverStreamTest`, which plays the streams of `MockVideoStreamServer`.
That class and the contract test are built only with GStreamer, which sends their streams. So are the classes in
`test/VideoManager/FFmpeg/`, which are built with `QGC_ENABLE_FFMPEG_VIDEO` as well
([FFmpeg/README.md](FFmpeg/README.md#tests)).
