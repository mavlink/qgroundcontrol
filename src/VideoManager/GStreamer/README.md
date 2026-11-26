# GStreamer Video Streaming

QGroundControl uses GStreamer for UDP RTP and RTSP video streaming in the Main Flight Display.

This guide covers the GStreamer receiver architecture, platform setup, and diagnostics. See
[the video module's README](../README.md) for a map of the folders of `src/VideoManager` and the path of a frame and
of a recording through them, [CODING_STYLE.md](../../../CODING_STYLE.md) for repository-wide source conventions,
[test/README.md](../../../test/README.md) for the test framework, and
[tools/README.md](../../../tools/README.md) for general build and tooling commands.

## Source Code Architecture

The backend is split into focused components, by folder:

- `GStreamerVideoBackend`, in this folder: the `VideoBackend`, the app's way into the folders below.
- `Runtime/`: initialization, environment, logging, decoder ranks, workarounds and helpers.
- `Source/`: the source bins, from a stream's URI to its parsed stream.
- `Pipeline/`: the receiver, its worker, sessions and branches.
- `Sink/`: `QGCQVideoSinkController` and the `gstqgc` plugin (`Sink/gstqgc/`).
- `HwBuffers/`: the zero-copy GPU paths: the `HwBuffers` facade the other folders call, a folder for each path, and
  `common/`, which the paths share.
- `SelfTest/`: the boot self-test and the encoded samples it decodes and records (`SelfTest/Media/`).
- `Platform/`: platform shims (`IOSPipe2Compat.c`) and the Android JNI state `GStreamer::mobile` keeps for GStreamer's Android plugins (`gst_android_jni.c`).

The backend-neutral `VideoReceiver`, `VideoBackend` and `VideoStreamStats` are in `../Receiver/`. `../QtMultimedia/`
holds the other `VideoReceiver`, `QtMultimediaVideoReceiver`, which plays streams through `QMediaPlayer` (FFmpeg): in
builds without GStreamer, where `VideoSettings::videoBackend` picks it, and when GStreamer failed to initialize.

Each receiver plays for one video source, a `VideoStreamController` (`../Streams/`), and `VideoManager` (`..`) is the
QML facade of the sources: the primary stream, the thermal stream, the USB camera (`../UVC/`), and one for each other
stream shown in a tile beside the main picture (`VideoTiles.qml` and `VideoTile.qml` in `../Qml/`). Those are the other
streams the active vehicle's cameras advertise, up to `VideoSettings::maxVideoTiles`, or to the tiles the row has
room for where those are fewer (`VideoManager::tileRoom`), less the tile of the USB camera that
`VideoSettings::uvcTileCamera` names. A stream at an address (its URI, or the UDP port it is listened for on)
that a stream on display already has gets no source: a camera sends such streams one at a time.
There are no tiles, and so none of their sources, until a row of tiles that is shown tells its room: the Fly View
shows the row only while the video is its main view, not in the picture-in-picture. A row that is no longer shown
tells that it has no room once it has been out of view for 15 s (`VideoStreamController::kIdleGrace`), so a video
that changes places with the map and back within that time finds the tiles' streams playing.

- `VideoManager::sources` and `primaryStream` notify. When the vehicle's current stream becomes one that plays in a
  tile, as after `VideoManager::selectSource()`, that tile's source becomes the primary stream and the former primary
  stream takes the tile: both keep their receivers, and neither stream restarts. A stream that plays in no tile is
  played by the primary stream, whose receiver connects to it.
- A QML `VideoOutput` registers with the source it shows (`VideoStreamOutput.qml`). One that registers as another one
  leaves takes the source over, so a source moves between the main picture and a tile without its receiver being
  without an output, which would take the decoder out of its pipeline. A tile's source has an output only while the
  tile is shown: hidden, its stream is received but not decoded, until its row tells that it has no room.
- The thermal stream is the active vehicle's: its current camera's (`thermalStreamInstance()`) or, when that camera
  has none, that of the first of its other cameras that has one. That is how mavlink-camera-manager sends it, which
  makes a camera component of each stream, with stream id 1, and flags the thermal one `THERMAL` and not `RUNNING`.
  A thermal stream is none of the streams a camera lists, so a current camera that advertises thermal streams only
  has no current stream: the primary stream then plays its thermal stream, in place of the video source of the
  settings, and there is no thermal stream (`VideoManager::_updateStreamInfo()`).
- The thermal stream plays only while it is in use: while its view is on, which gives it an output, and from
  `startRecording()` until its recording has ended. Out of use for `kIdleGrace` its receiver is deactivated, and
  activated again with the next use: its picture then waits for the stream to connect and send a keyframe.
- On Android and iOS no source has an output while the app is in the background (`Qt::ApplicationHidden` or
  `Qt::ApplicationSuspended`): nothing is decoded there. The streams are still received and recorded, for as long as
  the system lets them arrive: Android 15 cuts a background app's network after about five seconds, with which the
  stream times out and a recording ends, to continue in a new part once the app is back.
- The vehicle's cameras are told which of their streams play besides their current one
  (`MavlinkCameraControlInterface::setExtraStreams()`): those in tiles, and a camera's thermal stream while the
  source that plays it wants it (`VideoStreamController::wantsStream()`), which the thermal stream out of use
  doesn't. `VehicleCameraControl` asks the camera to transmit them
  (`MAV_CMD_VIDEO_START_STREAMING`) and to stop those that left (`MAV_CMD_VIDEO_STOP_STREAMING`), and neither stops
  nor starts a stream that stays on display when the current stream changes.
- While the video is off, stopped (`VideoManager::stopVideo()`) or disabled for a disarmed vehicle
  (`VideoSettings::disableWhenDisarmed`, which goes by the active vehicle's armed state), the vehicle's current
  camera is told to stop transmitting its stream (`stopStream()`), and none of its streams is named as an extra one:
  a stream pushed to a UDP port would otherwise cross the vehicle's link with nobody receiving it. The camera is
  told to resume (`resumeStream()`) once the video plays again.
- Recordings and photos are of the main picture and its thermal stream, not of the tiles. A recording connects a
  thermal stream that is out of use, so its file starts a moment after the main picture's. A recording asked for
  while one runs is refused by `VideoRecordingService`, with a warning logged, and the Qt Multimedia receiver and
  the USB camera report one they refuse, their recorder being in use, as `recordingFinished()` with the reason.

The components, in the order of their folders: `Pipeline/`, `Source/`, `Sink/`, `Runtime/`, `SelfTest/`, and
`GStreamerVideoBackend`. Each has a line here: its header says what it does and why, with its threads and its limits.

| Component | Role |
| --- | --- |
| `GstVideoReceiver` | The `VideoReceiver` the app uses, on the GUI thread: owns the display sink, hands each change to its worker as one `GstReceiverRequest`, and notices a worker that is stuck (see [Threading](#threading)). |
| `GstReceiverWorker` | On the receiver's worker thread: keeps one `PipelineSession` in line with the latest request, reconnects as its `StreamSupervisor` paces it, continues a recording in the parts its `RecordingPlan` names, and reports back through queued signals. |
| `PipelineSession` | One pipeline (source → tee → the branches below) and everything about it: elements, bus, probes. Recovers its branches from their errors and reports failures of the stream to the worker. |
| `PipelineStatsProbe` | What the session measures for the stream statistics apart from its branches: bytes, keyframe times, RTP counters, latency and codec. |
| `GstBusPump` | The session's bus: a sync handler for what can't wait, the other messages popped on the worker, and a detach after which nothing reaches the session. |
| `DecodingBranch` | Display branch behind the tee: inserts `decodebin3` once the source streams, replaces a decoder that fails or stalls, gives up one that keeps failing, and drops frames rather than hold up the tee. |
| `RecordingBranch` | Recording branch behind the tee: a `parser ! muxer ! filesink` bin for each file, started at a keyframe, continued in parts, synced to the disk, and ended rather than stall the display. |
| `TeeBranchHead` | The `tee ! queue ! valve` head both branches start with, which also turns a push that fails behind it into its branch's own `ERROR`. |
| `GstReceiverTimings` | The receiver's intervals and timeouts that a test can't wait out, with the app's values as defaults. |
| `BranchContext` | What `PipelineSession` lends its branches, and the branch marks that blame a bus message on the branch it came from. |
| `StreamWatchdog` | Notices stalled source or decoder output on a monotonic clock. |
| `PipelineDotDumper` | Writes the pipeline's `.dot` graphs; see [Pipeline Observability](#pipeline-observability). |
| `GstSourceFactory` | Builds the source bin for the transport the stream URI's scheme selects, with its RTP jitter-buffer policy. |
| `GstRtpPacketProbe` | Counts the RTP packets a pad pushes, for a source that has no jitter buffer to count them. |
| `RtpPacketCounter` | The packets received and lost in one RTP stream, as RFC 3550 counts them, and its interarrival jitter. No GStreamer types. |
| `GstRtspSource` | The `rtspsrc` the source factory builds: URI check, transports, timeouts, retransmission, video-only stream selection and TEARDOWN. |
| `GstH26xParameterSetGate` | H.264/H.265 setup for depayloaders and parsers: parameter sets repeated at every IDR, keyframe requests after loss, and output held until a full parameter set arrives. Declared in `GstSourceFactory.h`. |
| `QGCQVideoSinkController` | GUI-thread companion of the display sink; see [`gstqgc`](#gstqgc--custom-gstreamer-plugin). |
| `GStreamer` | The facade QGC calls, through `VideoBackend` where it must also build without GStreamer: initialization and the display sink. |
| `GStreamerEnvironment` | Process-wide environment setup run before `gst_init()`; see [Runtime Environment Setup](#runtime-environment-setup). |
| `GStreamerDecoderRanks` | Video decoder classification, and `DecoderRankApplier`, which applies the decoder setting's rank plan to the registry and takes a hardware decoder that keeps failing out of autoplug. |
| `GStreamerDecoderRankPlan` | `planRanks()`: the rank changes a `forceVideoDecoder` option makes to the installed decoders, with the platform rules as `RankTraits`, without the registry. |
| `GStreamerHelpers` | Small utilities: keyframe requests, events sent without blocking, holding a new pad's data until it is linked, and taking a child element out of a running bin. |
| `GStreamerLibavWorkaround` | Has `avdec_h265` decode on one thread with FFmpeg 7.1.0, whose slice threads can deadlock. |
| `GStreamerLogging` | Routes GStreamer's log output through QGC's logging, unless GStreamer's own output was requested, and applies the persisted debug level. See [GStreamer Logging](#gstreamer-logging). |
| `GStreamerUdpSrcWorkaround` | Serializes `udpsrc`'s `unlock()`/`unlock_stop()` on gst-plugins-good before 1.24.10, whose race corrupts the heap. |
| `GstScoped` | RAII owners for transfer-full GStreamer returns so refs can't leak on early return, `makeElement()` (never floating), `PadProbe` and `ScopedSignal`. |
| `GStreamerSelfTest` | `GStreamer::SelfTest::run()`, which the boot test runs to prove the packaged runtime decodes and records. See [Boot self-test](#boot-self-test). |
| `GStreamerSelfTestMedia` | The self-test's media pipelines: decoding the embedded samples and recording and reading back MP4 and MKV. |
| `GStreamerVideoBackend` | The `VideoBackend` implementation: the only way the app reaches GStreamer's process lifecycle (environment, initialization, self-test) and its tuning. |

MockLink's `MockVideoStreamServer` (`src/Comms/MockLink`, Debug builds only) serves a synthetic `videotestsrc` stream (RTP H.264 or H.265 over UDP, RTSP, MPEG-TS over UDP or TCP) that MockLink advertises, so the receiver runs without a vehicle; builds without GStreamer compile a null server instead. `MockConfiguration::CameraThermalStream` has MockLink's cameras advertise a thermal stream as well, as the second stream of a camera or as the only stream of a camera of its own, the way mavlink-camera-manager does; nothing is sent at its address, `udp://127.0.0.1:5601`.

### Threading

- **GUI thread**: `GstVideoReceiver`'s API and every signal it emits, the display sink's `QGCQVideoSinkController`,
  and the video size, read from each frame the output's `QVideoSink` shows: its viewport (decoder padding and crop left
  out), turned as Qt presents it.
- **Worker thread** (one per receiver, running its `GstReceiverWorker`): the reconnect logic, every change to the
  pipeline, and its bus. A sync handler only does what can't wait: answers `NEED_CONTEXT`, selects the video streams,
  and closes the valve in front of a branch that posted an `ERROR`, so the error stays out of the tee (a failed push
  without an `ERROR` gets one posted at the branch's entrance, `TeeBranchHead::absorbFlowErrors()`). Everything else
  is popped on the worker, which a `QSocketNotifier` (`QWinEventNotifier` on Windows) on the bus's poll fd wakes up,
  as Qt's `QGstBusObserver` does.
- **Streaming threads**: probes and element signals only note what happened and queue the rest on their
  `PipelineSession`, which drops it once the pipeline is gone.
- **Teardown** is bounded: an open recording drains through an EOS sent from GStreamer's thread pool, the worker waits
  for it at most 3 s and then flushes the recorder, and the receiver's destructor waits at most 5 s for the worker,
  leaving one stuck in the kernel (a hung network mount) to finish on its own. The worker reaches the receiver only
  through signals queued to it, which Qt drops once the receiver is gone.
- **A stuck worker**: stopping an element waits for its streaming thread, so a thread that never returns (a decoder
  deadlocked in its library, a recorder stuck in the kernel) holds the worker for good, and with it the pipeline's state
  lock. No request is served any more, and the pipeline can neither be stopped nor replaced: a second one would not get
  the UDP port, or the decoder, that the first still holds. The receiver's check of its worker, a single-shot `QTimer`
  on the GUI thread that each check starts again, notices it: the receiver tells the user once, through `errorOccurred`,
  to restart the application, and from then on reports the stream as stopped unless frames still reach the output. Its
  interval, 10 s, is longer than the worker's longest waits in a row (3 s for a recording to finalize, 1 s for an RTSP
  TEARDOWN, 3 s for a TCP connect), and it takes six checks in a row, each a full interval after the one before, at
  which the worker has run none of its tasks since the check before: the name lookup for a `tcp://` or `udp://` host
  runs on the worker too, and can take half a minute to fail. So a worker that is busy, or behind with a row of slow
  requests, isn't reported, nor one that a suspended machine or a stopped process gave no time. The timer stops once the
  worker of a receiver that wants no stream is through with what it was posted.

### `gstqgc` — custom GStreamer plugin

A QGC-owned plugin (`gstqgc/`) bridging the pipeline into Qt:

- **`qgcqvideosink`** — a `GstVideoSink` that pushes decoded frames into a Qt `QVideoSink`. Delivery
  (`GstQgcFrameDelivery`) keeps only the latest frame, so a stalled GUI thread never holds more than one undelivered
  frame (or the decoder buffer behind it).
  A frame is as large as its buffer's `GstVideoMeta`, which a decoder pads beyond the caps (1088 rows for 1080p) or
  leaves uncropped, and its viewport is the picture in it: the buffer's `GstVideoCropMeta`, else the caps' size at the
  frame's origin (`frameGeometry()` in `GstQgcFrameMap`). The CPU, DMABuf and GLMemory paths show the frame at that
  size. A `GstVideoAffineTransformationMeta` narrows the viewport and flips the frame: Android's amcvideodec attaches
  one with its SurfaceTexture's crop and orientation, which glcolorconvert hands on unapplied. Only scales,
  translations and flips are applied, and the sink asks no element for the meta. glcolorconvert squeezes the
  decoder's padded buffer into a texture of the caps' size, so that viewport is smaller than the picture (1072 rows
  for 1080p): the sink's `picture-width` and `picture-height` properties give the picture's own size, which the
  receiver reports as the video size.
  Frames carry the rotation and mirroring of the stream's `image-orientation` tag, as a camera mounted sideways sends
  it. The `VideoOutput` showing them adds the rotation a MAVLink camera
  reports for the stream (`VideoStreamController::rotation`) on top, as its `orientation`. Its construct-only `gpu-zerocopy` property (passed down by the bin) enables zero-copy import of GPU memory, and its
  `frame-stats` property reports that sink's own frame counts per delivery path, GPU fallbacks, dropped tiled frames
  (`GstQgc::formatSinkStats` in `GstQgcSinkStats.h` formats them) and delivered frames a newer one replaced before the `QVideoSink` took them
  (`superseded-frames`), all counted by its `GstQgcSinkCounters`; GstBaseSink's standard `stats` property is left as is.
  Its `GstQgcPathSession` keeps what holds for one caps epoch (the GPU path resolved, one demotion and one tiled-drop
  warning per epoch) and fails the stream after 120 frames in a row fail to map.
- **Sink caps** — `qgcqvideosink` answers caps queries itself (`GstQgc::buildSinkCaps`), with the formats Qt renders
  (`kVideoFormatTable`):
  - System memory always.
  - Every entry asks for `pixel-aspect-ratio=1/1` unless the bin's `disable-par` is set. The converter scales system
    memory to square pixels; nothing in the bin scales GPU memory (d3d12convert aside), so a decoder whose pixels
    aren't square negotiates system memory.
  - With `gpu-zerocopy`, ahead of it, only the GPU memory the renderer Qt Quick uses can import. Under OpenGL: DMA_DRM
    and DMABuf on EGL while the DMABuf import works (see
    [DMABuf zero-copy diagnostics](#dmabuf-zero-copy-diagnostics)), AHardwareBuffer on EGL (Android), and GLMemory,
    which reaches Qt through the GL share group, on EGL or GLX. Under Direct3D11: D3D11Memory, and ahead of it NV12
    and P010 D3D12Memory once Qt's device is known to open it (see D3D12 frames under Direct3D 11). Under
    Direct3D12, the d3d12convert variant (below). Under Vulkan: DMA_DRM in the layouts Qt's device imports (see
    [DMABuf under Vulkan](#dmabuf-under-vulkan)). Metal and OpenGL on WGL or CGL get system memory only. vtdec's
    IOSurface-backed frames arrive as system memory.
  - GLMemory always carries `texture-target=2D`, the only target the GL path samples. It also makes glcolorconvert
    convert amcvideodec's external-OES textures, which Qt would otherwise sample black.
  - The glupload variant (see [GPU bin selection](#gpu-bin-selection)) takes GLMemory only.
  - The d3d12convert variant, the bin Qt's Direct3D12 renderer always gets, takes RGBA or BGRA D3D12Memory only: its
    `d3d12upload ! d3d12convert` uploads software-decoded frames and converts the d3d12 decoders' NV12/P010 on the GPU,
    because Qt's D3D12 QRhi creates no per-plane views and can't sample a planar resource's chroma (the D3D12 import
    sends any multi-plane frame to the CPU path). A D3D12 device is a per-adapter singleton, so the device GStreamer
    opens on Qt's adapter is Qt's own, and the decoders are ranked to match the renderer.

  The renderer is the backend of the scene graph's `QRhi` (`QGCRhiCapture`), or, until the scene graph runs, the
  graphics API Qt Quick was asked for. When it changes mid-stream, or the DMABuf import starts or stops working, the
  sink asks upstream to renegotiate.
- **`qgcvideosinkbin`** — `converter ! qgcqvideosink`, with `glupload ! glcolorconvert` in between for the glupload
  variant and `d3d12upload ! d3d12convert` for the d3d12convert variant. The converter is the `conversion-element`
  override, else `imxvideoconvert_g2d` or `nvvidconv` when present, else `videoconvertscale`
  (`videoconvert ! videoscale` before GStreamer 1.22). It scales non-square pixels to square ones, converts formats the
  sink doesn't take (such as a software decoder's `Y444`), and passes GPU memory through. When Qt renders with
  Direct3D 11, an override that names a filter of GStreamer's `d3d11` plugin other than `d3d11upload` and
  `d3d11download` (such as `d3d11convert` or `d3d11deinterlace`) is ignored: a pipeline's d3d11 elements share the
  device Qt renders with, and such a filter draws on its immediate context, whose state Qt caches. Decoders and the
  two elements named only copy there.

  The bin doesn't deinterlace: an interlaced stream (576i or 1080i, as analogue-to-IP encoders send) is shown woven,
  both fields in each frame, so motion combs. Upstream's playsink plugs gst-plugins-good's `deinterlace` for that.
  Here it would reach only software-decoded frames, the bundles don't ship its plugin (the `gstreamer.plugins` lists
  of `.github/build-config.json`), and on an interlaced stream it holds back a field or more (20 ms at 50 fields a
  second with its default method) and doubles the frame rate. A stream in `interlace-mode=alternate` doesn't
  negotiate at all, as the sink doesn't offer the `format:Interlaced` caps feature; decoders that can send both
  send interleaved frames.
- **Allocation proposal** — the sink asks the decoder for at least five buffers (`kMaxPinnedFrames` + 1), covering the
  frames Qt may still hold, also of a producer that asks for no pool, and keeps the video and crop metas it reads. D3D11Memory and D3D12Memory get
  no crop meta: the decoders then crop a picture that starts off their texture's origin themselves, where the D3D
  paths would copy it from the origin. It asks for GL sync metas as upstream's Qt sink does, which has glcolorconvert end each
  frame with a sync point. The GL path leaves a frame's sync meta alone, since other consumers share the buffer: it
  sets a sync point of its own, in a buffer its texture bundles hand on, and waits for it in Qt's context.
  `QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS` overrides the count, as Qt's `QT_GSTREAMER_PROPOSE_ALLOCATION_MIN_BUFFERS`
  does, e.g. to save CMA memory with 4K decoders on i.MX or Raspberry Pi.
- **`QGCQVideoSinkController`** — GUI-thread companion that binds the sink to a `QVideoSink`, lets it deliver only
  while the output's window is visible, and clears the frame that `QVideoSink` displays when decoding stops.

### `HwBuffers` — GPU paths

Per-platform import of decoded GPU frames without a copy. Most paths wrap their native textures in
`GstHwFrameTexturesBase : QVideoFrameTextures`; the DMABuf path hands its planes to Qt Multimedia's DMABuf importer
(`dmabuf/GstQtDmaBufImport`, the only code using that private Qt API), or, when Qt Quick renders with Vulkan, imports
them as VkImages itself (`dmabuf/GstDmaBufVulkanImport`):

- **Platforms**: `dmabuf` (Linux/VA-API), `gl` (Linux, Android), `vulkan`, `d3d` (D3D11/D3D12), `cuda`, `apple` (IOSurface), `android` (AHardwareBuffer).
- **`HwPathDescriptor`**: each backend describes its path once (memory match, buffer construction, cache reset,
  pool proposal, context bridge). `GstHwPathRegistry` lists the compiled ones in a constant array. The factory selects
  the first path whose descriptor matches a frame's memory, and the facade loops over the list for bus messages,
  context queries, restarts and resets.
- **`GstContextBridgeBase`**: GL, D3D11 and D3D12 bridges hand Qt's GPU device to GStreamer elements through
  `NEED_CONTEXT` and context queries (the Vulkan bridge can't: see the opt-in paths below). Under a Direct3D 11
  renderer the D3D12 bridge hands out a device of GStreamer's own on Qt's adapter instead. Qt's is the main window's
  device: a VideoOutput popped out into its own window renders on that window's device, so on Direct3D11 its frames
  take the CPU path. So do Direct3D frames a decoder allocated on another device before the bridge was primed, and
  GLMemory frames from a GL context outside Qt's share group, which gst-gl creates for itself when no element was
  handed the GL bridge's. The GL bridge limits gst-gl to the OpenGL API of Qt's context, desktop OpenGL or OpenGL ES:
  gst-gl would otherwise create OpenGL ES contexts wherever EGL offers them, and EGL shares only within one API.
  A bridge sets the context it answers a `NEED_CONTEXT` with on the pipeline of the element that asked, and the
  pipeline hands it to the elements it is given later. An element that asked before Qt's device was known made a
  device of its own, which its pipeline would hand on instead without asking again: when the scene graph publishes
  Qt's device, the bridges give those pipelines their contexts. Elements built from then on use Qt's device; the one
  that made its own keeps it until it is rebuilt. So each bridge also posts a `qgc-graphics-device-served` element
  message on such a pipeline, and the session replaces a hardware decoder plugged by then (`DecodingBranch`): a
  Direct3D 11 decoder left on its own device would have every frame copied through the CPU for as long as the stream
  ran. When GStreamer reports the Direct3D device a bridge shares removed (its devices' `device-removed-reason`, from
  1.26: a driver reset or update), the bridges and the paths' caches are dropped as they are when Qt tears its scene
  graph down, and every pipeline a bridge gave its device gets a `qgc-graphics-device-lost` element message. Its
  receiver then restarts the stream with a new video sink, so that nothing holds the removed device: Direct3D 12
  creates no new device on an adapter while the removed one is referenced, which would keep Qt from recovering. The
  new pipeline gets the device Qt renders with by then, or the one it publishes later. Until a bridge has a device
  again, a decoder that fails is not taken out of autoplug: none works on a removed device.
- **D3D11 decoder padding**: decoders pad their textures to their alignment (16 rows for H.264, 128 for HEVC/AV1)
  without crop meta. The picture is copied out on the render thread, as a texture-array slice is, so frames keep the
  picture's size and a CPU layout for that fallback and for screenshots.
- **D3D12 texture arrays**: no slice of a texture array reaches the sink. gst-d3d12's pools hold single textures,
  and its decoders copy a frame out of a reference-only array before they hand it on. QRhi can't sample one slice, so
  the path takes none: such a frame would go to the CPU.
- **D3D12 frames under Direct3D 11**: a Direct3D 11 QRhi samples a D3D12Memory frame without GStreamer using Qt's
  device. The frame's resource opens as a texture of that device (`gst_d3d12_memory_get_d3d11_texture`, which asks
  for a shared heap and a render-target binding: the pool the sink proposes has both), the render thread copies the
  picture out of it as it does for a d3d11 decoder, and signals a fence the two devices share; the frame's buffer
  goes back to its pool once Direct3D 11 has read it. The sink offers that memory only where Qt's device reports the
  sharing gst-d3d11 asks of a device for its own interop (`D3D11_SHARED_RESOURCE_TIER_2`, extended resource sharing
  and shared NV12 textures). WARP reports it and then opens NV12 without its chroma plane; no decoder shares its
  adapter, so nothing sends it such a frame. The NVIDIA decoders take this path, as they output D3D12Memory when a
  sink takes it. So do the d3d12 decoders, which are the default under Direct3D 11 where Qt's device reports that
  sharing, as they are in GStreamer's own ranking: they keep GStreamer off the device Qt draws with, and a video
  popped out into its own window imports their frames on that window's device too. The d3d11 decoders stay ranked
  behind them, and are the default where Qt's device lacks the sharing; `GST_PLUGIN_FEATURE_RANK`
  (`d3d11h264dec:MAX`) selects one anyway. A decoder on another adapter than Qt's goes to the CPU path, as with the
  D3D11 path, and ranks below the software decoders once Qt's adapter is known.
- **D3D12 resource states**: the import hands every resource to QRhi as `D3D12_RESOURCE_STATE_COMMON`. Qt
  transitions a texture it samples to a shader-resource state and never back (`QD3D12ResourceBarrierGenerator`,
  Qt 6.12), while the converter's pool recycles its resources. The D3D12 debug layer reports no state error for it
  (Qt 6.12.0, GStreamer 1.28.8, Intel Arc, 875 frames). To repeat the run, set `GST_ENABLE_D3D12_DEBUG=1` along
  with `QSG_RHI_DEBUG_LAYER=1`: the layer can't be turned on for a device GStreamer already created, and Qt then
  falls back to WARP.
- **`GstHwPathTelemetry`** tracks process-wide render-time counters (map failures, reuse hits, sync waits) for fallback
  diagnostics. A receiver's `GPU path live` and `GPU path stats` debug lines pair its own sink's `frame-stats` with
  them, both counted from when its session took the sink. Reading never resets the counters: each reader reports the
  change since its own snapshot, so receivers don't take each other's counts.
- **`CpuVideoFramePool`** recycles CPU-backed `QVideoFrame` storage (WebRTC-style slab) for the software-fallback path, avoiding per-frame allocation.
- **Build options**: `HwBuffers/CMakeLists.txt` probes one table row per path; the `QGC_GST_ENABLE_<PATH>` switches
  that compile a path in or out are listed in [`cmake/README.md`](../../../cmake/README.md#gstreamer). Windows builds
  leave out the GLMemory path: Qt 6's OpenGL there is WGL, which gst-gl can't share. Linux builds with gst-gl and
  DMABuf compile both GPU bins and the one that picks between them per stream; see
  [GPU bin selection](#gpu-bin-selection).
- **Opt-in paths**: the CUDA, AHardwareBuffer and Vulkan paths are compiled only with `QGC_GST_ENABLE_<PATH>=ON`,
  because no stream reaches them: no bin advertises CUDAMemory or VulkanImage caps, and Android's glupload bin offers
  only GLMemory. Configure lists the compiled paths that the sink can't reach at runtime. CI compiles the Vulkan and
  CUDA ones, and runs neither (the `gpu-opt-in` job of `linux.yml`). None of them is a zero-copy path that works
  today:
  - **Vulkan** (`vulkan/`, gst-vulkan's VulkanImage memory): gst-vulkan creates its own `VkDevice` and has no way to
    wrap Qt's (`gst_vulkan_device_new()` takes a physical device; so in 1.24, 1.28 and on `main`), and QRhi can't
    sample an image of another device. `GstVulkanVideoBuffer` therefore imports no frame, and the Vulkan bridge only
    puts GStreamer's device on the physical device Qt renders on. A VulkanImage frame that did reach the sink would
    not be shown: gst-vulkan's pools allocate device-local memory, which it doesn't map on the CPU either. In a build
    with the path, `GstHwBuffersVulkanReadbackTest` checks the bridge's device and the refused import. gst-vulkan
    looks for a device on its pads only, so an element that isn't linked to the sink when it opens its device has its
    instance create one, and gets the bridge's through the instance's `create-device` signal. That signal has no
    parameter before gst-vulkan 1.26 and the device's index from then on, so the bridge connects the handler that fits
    it.
  - **CUDA** (`cuda/`, gst-cuda's CUDAMemory): never run on NVIDIA hardware, only compiled, against a stub `cuda.h`.
    The path wraps the fd of `gst_cuda_memory_export()` as a dma-buf for the DMABuf path. gst-cuda exports only memory
    of a `GST_CUDA_MEMORY_ALLOC_MMAP` pool, which nothing in QGC proposes to a decoder, so a decoder's frames aren't
    admitted and take the CPU path. And the fd is CUDA's shareable handle (`cuMemExportToShareableHandle`), which the
    desktop driver's EGL is [reported](https://forums.developer.nvidia.com/t/cuda-and-linux-dma-buf/194267) to refuse
    as a dma-buf: the path asks the kernel whether it is one, and turns itself off for the process when it isn't
    rather than fail every import. The route to try on NVIDIA hardware is `cudadownload` to GLMemory (a copy on the
    GPU, where gst-cuda is built with OpenGL support) into the glupload bin.
  - **AHardwareBuffer** (`android/`): never compiled for Android, let alone run. It is written against
    `<gst/android/gstandroid.h>` (`gst_ahardware_buffer_memory_get_buffer()`), which no GStreamer release has, so
    `QGC_GST_ENABLE_AHARDWAREBUFFER=ON` finds nothing to build against in the SDK QGC downloads. GStreamer 1.30 brings
    an AHardwareBuffer memory of its own, which this path can't take as it is: another header
    (`<gst/allocators/gstahardwarebuffer.h>`), another getter (`gst_ahardware_buffer_memory_peek_buffer()`) and caps
    with `format=AHARDWARE_BUFFER` and an `ahb-format` field, where the sink offers the path NV12-style formats. It
    also gives glupload an uploader for that memory, so the glupload bin may need no path of QGC's for it.
    Its source is only syntax-checked on Linux against a stub of that header. Its frames are external-OES textures
    with no CPU layout Qt could show instead, so the path takes a frame only while it can import it: not under a
    renderer other than OpenGL, nor after the EGL display lacked `EGL_ANDROID_image_native_buffer` or the driver
    refused frame after frame. Android's zero-copy path is GLMemory through the glupload bin.

### Stream statistics

`VideoReceiver::stats` (`VideoStreamStats`, a QML value type that `VideoStreamController` forwards) tells how healthy a
stream is. The Fly View toolbar's video indicator shows those of `VideoManager::measuredSource`, and turns orange while
`VideoManager::degraded` tells that a source falls short (`VideoStreamStats::degraded()`). While the stream is
`Streaming`, `GstReceiverWorker` samples
its `PipelineSession` on each watchdog tick (once a second) through `VideoStreamStatsSampler`
(`../Receiver/VideoStreamStatsSampler.h`, shared with the Qt Multimedia receiver): its `VideoStreamStatsMeter`, a pure
class with an injectable clock, turns the cumulative counters into rates, and it adds the session's fields below. The first sample is the rates'
baseline, so stats are valid from the second one on. The worker hands them to the receiver like its other reports, so
`statsChanged` is emitted on the GUI thread, and sends invalid stats when its session stops: on a stop, a reconnect or a
failure. The streaming threads only add to atomic counters, with no lock and no allocation per frame or packet;
everything else is read on the worker.

| Field | Measured from |
| --- | --- |
| `fps` | Frames the display sink delivered to the `QVideoSink` (its `frame-stats` counts per path) less those it superseded, per second over the last `VideoStreamStatsMeter::kRateWindowIntervals` samples. |
| `bitrateKbps` | The compressed elementary stream reaching the tee, after the depayloader or demuxer and the parser, counted by the tee's buffer probe, over the same window as `fps`. |
| `framesDropped`, `droppedFps` | Since the stream started, and per second over the last interval: buffers the display queue leaked (its `overrun`s, counted once the buffer after them comes out marked `DISCONT`, which tells that the queue did leak), delta frames its keyframe gate dropped, and superseded frames. A frame that arrives too late for its time is not among them: the display sink shows it at once (it is decoded and uploaded by then, so dropping it would not help a decoder that fell behind to catch up). |
| `packetsLost`, `lossPercent`, `jitterMs` | The `stats` of every `rtpjitterbuffer` in the source bin, `rtspsrc`'s internal ones included: `num-lost` since the stream started, `num-lost` against `num-pushed` over the last second, and `avg-jitter`. Lost are also the packets a jitter buffer drops on arrival to stay within its latency (`drop-on-latency`), as after a burst: they are in none of its `stats`, so the source factory has it post a `drop-msg` element message for each (`post-drop-messages`), whose `num-drop-on-latency` `PipelineStatsProbe` adds up from the bus. UDP RTP in low-latency mode has no jitter buffer: the source bin's `GstRtpPacketProbe` counts the packets `udpsrc` receives instead (below). -1 for MPEG-TS, which carries no RTP. |
| `latencyMs` | The minimum latency of a latency query on the pipeline, as logged when it reaches PLAYING. 0 in low-latency mode, whose display sink doesn't sync, so the frames a decoder's frame threads hold back are not in it. |
| `decoder`, `hardwareDecoder` | The decoder factory decodebin3 plugged last, and `GStreamer::isHardwareDecoderFactory()`. Empty and false while no decoder is inserted: without a video output, and while a decoder is being replaced. |
| `gpuPath` | The delivery path most frames took since the previous sample, as `GstHw::toString()` names it: `CPU`, `DMABuf`, `GL`, `D3D11`, `D3D12`, `IOSurface`, `AHWBuf` or `Vulkan`. Empty for a display sink other than `qgcvideosinkbin`. |
| `codec` | The caps on the tee: `H.264`, `H.265`, `AV1`, `VP8`, `VP9`, `MJPEG`, `MPEG-2`, ... |
| `packetsLate`, `packetsDuplicate`, `rtxRequested`, `rtxRecovered` | The same jitter buffers' `num-late`, `num-duplicates`, `rtx-count` and `rtx-success-count`, since the stream started. -1 without a jitter buffer, which alone tells them: MPEG-TS, and UDP RTP in low-latency mode. |
| `keyframeIntervalMs` | The tee's buffer probe (`PipelineStatsProbe`): the average time between the timestamps (PTS, else DTS) of the last five buffers without `DELTA_UNIT`, so the arrival times a stream without a jitter buffer is stamped with don't swing it. -1 until two keyframes arrived, so a stream without keyframes, such as one with intra refresh, never has one. |
| `gpuFallbacks` | The display sink's `gpu-fallbacks` count since the stream started, the sinks a decoder restart replaced included. |
| `decoderRestarts` | Decoders `DecodingBranch::restartDecoder()` replaced since the stream started. |
| `uptimeMs`, `reconnects` | The worker's `StreamSupervisor`: the time since the session reported its first frame, and the sessions that streamed since the stream was started or restarted, less one. |
| `recordingMs`, `recordingBytes` | The worker's `RecordingPlan`, while recording: the time since the first part got its first frame, and the sizes of the parts' files. -1 otherwise. |

Without a jitter buffer, `RtpPacketCounter` counts as RFC 3550 (appendix A.1) has a receiver do. Lost packets are the
sequence numbers missing between the first packet and the highest one, across the 16-bit wraparounds, so a loss shows
with the next packet that arrives, where a jitter buffer first waits out its latency. A packet that arrives late, fewer
than 1024 packets behind, is no longer lost, and a duplicate counts once. `PipelineStatsProbe` gives the meter a lost
count that only grows, since the meter takes a counter that shrank for one that restarted: a late packet that a sample
counted as lost stays in the total until the next loss makes up for it. A sender that starts over begins a new sequence:
with another SSRC, or with sequence numbers 1024 or more behind the highest one or too far ahead of it to be loss,
and either way once a second packet goes on from the first, so one stray packet changes nothing. Too far ahead is
what the sender sends in ten seconds at the packet rate measured from the RTP timestamps, as `rtpbin` scales its
`max-dropout-time`, and 3000 packets before a second of the stream has measured it. The totals go on, without the
packets lost in between. The jitter is the interarrival jitter of appendix A.8 from the RTP timestamps (90 kHz) and the arrival times
`udpsrc` stamps its buffers with, which is how `rtpjitterbuffer` measures its `avg-jitter`.

`VideoStreamStatsMeterTest` covers the rate math on a fake clock and the sampler's session fields,
`GstVideoReceiverComponentsTest` the packet counter and its probe, `GstVideoReceiverStatsTest` the stats of real streams.

### Qt defects worked around

The video code works around three defects and one missing option in Qt Multimedia, as found in Qt 6.12.0. Qt
locations are relative to `qtmultimedia/src/`, QGC locations to this directory. The comment at each workaround names
the defect.

| Qt problem | Qt location | QGC workaround | Remove when |
| --- | --- | --- | --- |
| A `VideoOutput` never emits `windowChanged` and its `Window.window` goes stale, because `QQuickVideoOutput::itemChange()` never calls `QQuickItem::itemChange()`. | `multimediaquick/qquickvideooutput.cpp` | `../Qml/VideoStreamOutput.qml` follows its parent's window and registers again after it moved to another one. `QGCQVideoSinkController::followWindowVisibility()` (`Sink/QGCQVideoSinkController.cc`) takes the output's window at each registration. | The minimum Qt's `VideoOutput` emits `windowChanged`. The controller then connects to it, and the QML needs no second registration. |
| The V4L2 camera reads the focus range into its exposure compensation range and never sets its focus range, so `QCamera::setFocusDistance()` and `setExposureCompensation()` set wrong values. | `QV4L2Camera::initV4L2Controls()` in `plugins/multimedia/ffmpeg/qv4l2camera.cpp` | `kExposureAndFocusControls` in `../UVC/UVCCameraController.cc` turns the exposure and focus controls off on Linux. | The minimum Qt has qtmultimedia commit `b7f6f39957` ("v4l2: QV4L2Camera fixes"). It is on the 6.12 branch after 6.12.0, so Qt 6.12.1 should have it. |
| The DMABuf fourcc table names `Format_BGRA8888`, `Format_BGRA8888_Premultiplied` and `Format_BGRX8888` as `ABGR8888`, which is the bytes R,G,B,A. Qt renders these formats without swapping channels, so red and blue are exchanged. Their bytes B,G,R,A are `ARGB8888`. | `dmaBufFourccFromPixelFormat()` in `multimedia/video/qmultimedia_drm_support_p.h` | `GstQtDmaBufImport::planeFourccs()` (`HwBuffers/dmabuf/GstQtDmaBufImport.cc`) names these three formats itself. | The minimum Qt's table gives them `ARGB8888`. `planeFourccs()` then only copies that table. |
| A `QMediaPlayer`'s RTSP transport can only be chosen with the `QT_FFMPEG_RTSP_TRANSPORT` environment variable, which is process-wide and read on a worker thread while the source loads. `QPlaybackOptions` has no transport. | `loadMedia()` in `plugins/multimedia/ffmpeg/playbackengine/qffmpegmediadataholder.cpp` | `setRtspTransport()` in `../QtMultimedia/QtMultimediaVideoReceiver.cc` sets the variable before an open that needs another transport than it names, so receivers that open at the same time can get each other's transport. | `QPlaybackOptions` carries the transport. The receiver then sets it there. |

This project has reported none of these to Qt. As of November 2026:

- The V4L2 defect is already fixed upstream, by the commit in the table (`fdcdec2d8c` on `dev`).
- QTBUG-117074 (open) asks for a way to choose the RTSP transport. QTBUG-147533 (closed) added the environment variable
  in Qt 6.12.
- A search of Qt's bug tracker found no report of the `VideoOutput` or the DMABuf defect.

## Build Configuration

- **Enable/disable**: Set the `QGC_ENABLE_GST_VIDEOSTREAMING` CMake option to `ON`/`OFF`. With `OFF`, streams play
  through `QtMultimediaVideoReceiver` and can't be recorded.
- **Versions, checksums and plugins**: `gstreamer.version.<platform>`, `gstreamer.checksums`, `gstreamer.plugins` and the iOS CA-bundle pin live in [`.github/build-config.json`](../../../.github/build-config.json), read through `qgc_config_read` ([`cmake/modules/BuildConfigReader.cmake`](../../../cmake/modules/BuildConfigReader.cmake)). The download URLs (upstream, with the QGC S3 mirror as fallback) are templated from the version in [`cmake/GStreamer/Packages.cmake`](../../../cmake/GStreamer/Packages.cmake); a package is verified against its checksum whichever host serves it. [`mirror-gstreamer.yml`](../../../.github/workflows/mirror-gstreamer.yml) fills the mirror whenever a master push changes `build-config.json`. Linux uses the system GStreamer and treats `gstreamer.version.minimum` as its floor; every other platform's official SDK must be at least `gstreamer.version.bundled_minimum`, which configure, `tools/validate_configs.py`, the S3 mirror and `GStreamerTest` all read.
- **Configure options**:
  - `GStreamer_ROOT_DIR`: the SDK to use (on Linux, a prefix whose `lib*/pkgconfig` or `lib/<multiarch>/pkgconfig` holds `gstreamer-1.0.pc`). Configure never writes it; the SDK it selected is recorded as `QGC_GST_RESOLVED_ROOT` in `CMakeCache.txt`. pkg-config prints paths unquoted, so an SDK path with spaces fails configure, except on iOS, which reads its SDK without pkg-config. An installed Windows SDK in one (`C:/Program Files/...`) is skipped with a warning in favor of a download. SDKs are downloaded to `CPM_SOURCE_CACHE`, which defaults to `.cache/CPM` in the source tree: when its path has spaces, a Windows host downloads through its 8.3 short path, and configure fails on other hosts, or where the volume has no short names, until `CPM_SOURCE_CACHE` is set to a path without spaces.
  - `QGC_GST_VERSION`: overrides the build-config version (the minimum on Linux, the SDK to download elsewhere), as `<major>.<minor>.<micro>`.
  - `QGC_GST_BUNDLE_PLUGINS` (Linux, default `ON`): installs and AppImages bundle the system's plugins, plugin scanner and GIO modules (see [Linux](#linux)), and the app points GStreamer at them. Distribution packages set it `OFF`: the install then carries nothing of GStreamer, the app uses the system's plugins as they are, and the package must depend on the distribution's plugin packages. The Arch package then adds `gst-plugins-good`, `gst-plugins-bad-libs` and `gst-libav`; the DEB and RPM dependency scans do not see plugins, so their packagers list them. `OFF` requires `QGC_CREATE_APPIMAGE=OFF`.
  - `QGC_GST_USE_HOMEBREW` (macOS, default `OFF`): builds against Homebrew's GStreamer instead of the official SDK; such builds cannot be installed or packaged (see [macOS / Windows / iOS](#macos--windows--ios)).
  - `QGC_GST_CUDA_STUB_DIR` (Linux): a directory with a `cuda.h`, such as gst-plugins-bad's `gst-libs/gst/cuda/stub`, for building the CUDA GPU path without the CUDA Toolkit.
  - `GStreamer_REQUIRE_CHECKSUM` and `QGC_GST_DOWNLOAD_TIMEOUT` (the SDK download's wall-clock cap, default 1200 s; its other limits are the shared `QGC_DOWNLOAD_*` ones) control downloads. `GStreamer_REQUIRE_CHECKSUM=OFF` lets an unpinned SDK version be checked against upstream's `.sha256sum` sidecar, or not at all; the Windows SDK installer still runs only when its checksum is pinned.
- **Plugin allowlist**: `gstreamer.plugins` (`common` plus the platform's key) decides what is registered and bundled. Android links the requested plugins that have a static archive into `libgstreamer_android.so`; iOS registers only the requested plugins found in its merged archive, so the archive's other plugins (some GPL) stay out of the app; desktop installs bundle the allowlisted plugins found in the SDK and check after install that those and the bundle-required ones arrived. The plugin policy is `gstreamer_plugin_policy` in `.github/build-config.json` (`bundle_required`, `optional_decoders`, `mock_server_debug`, `gpl_denylist`, `alternates`, each described by the file's schema), outside the `gstreamer` section that keys CI's SDK caches; `PluginPolicy.cmake` reads it. Debug mobile builds add its `mock_server_debug` plugins (`videotestsrc`, `mpegtsmux`) for MockLink's test stream, which `MockVideoStreamServer` encodes with `openh264enc` (H.265 only where a desktop GStreamer has `x265enc`).
- **GPL**: QGC ships LGPL GStreamer only. The policy's `gpl_denylist` names the GPL plugins and libraries of the official SDKs (`x264`, `x265`, `a52dec`, `dvdread`, ...); a desktop install that would ship one fails, and the macOS framework copy leaves them out. CI's [artifact report](../../../.github/ci-notes/reporting.md#artifact-report) also fails a package that ships one anywhere in it, or an Android or iOS binary with x264 or x265 linked in; the Linux AppImage only warns, a known issue: Ubuntu's `libavcodec`, which the bundled `libav` plugin loads, pulls in GPL `libx264` and `libx265`. A missing bundle-required plugin fails a mobile configure. Configure warns once about allowlist names the SDK or system does not ship as plugins (misspelled, not built for the platform, or not installed) and once about shipped ones a static build cannot link; missing hardware-specific or alternative decoders (the policy's `optional_decoders`, e.g. `qsv`, `va`, `libav`, `dav1d`) only appear in a status line, as do optional components (`RtspServer`, `Gl`, ...) that were not found.
- **License notice**: configure writes `LGPL-2.1.txt` and a `NOTICE.txt` naming the GStreamer version, its source code and, for an auto-downloaded SDK, the official packages and their SHA-256 (`Notice.cmake`). It also names the third-party libraries the shipped plugins bring (FFmpeg for `libav`, libvpx, dav1d, OpenH264 with a note on Cisco's patent license, and OpenSSL outside Linux) and, on iOS, which links GStreamer statically, how to relink the app with a modified GStreamer. They ship in `share/licenses/gstreamer/` of desktop installs (macOS: `QGroundControl.app/Contents/Resources/licenses/gstreamer/`) and under `licenses/gstreamer/` in the Android APK assets and the iOS app bundle. The dependency SBOM CI attaches to each package ([`generate_cpm_sbom.py`](../../../tools/ci/security/generate_cpm_sbom.py)) lists GStreamer from the same `QGC_GST_RESOLVED_*` cache entries.
- **CMake modules** ([`cmake/GStreamer/`](../../../cmake/GStreamer/)): `Orchestrator.cmake` is the entry point this directory's `CMakeLists.txt` includes; its header lists inputs and outputs. `platform/<platform>.cmake` discovers each platform's SDK, and `Acquire.cmake` downloads its packages (`Packages.cmake` names them and their checksums) and extracts them with the platform's extractor in `acquire/<platform>.cmake`. `Targets.cmake` creates the targets the way Qt's `FindGStreamer.cmake` does, one `pkg_check_modules(IMPORTED_TARGET)` per library: `GStreamer::Core`, `GStreamer::<Component>` (`Base`, `Video`, `Gl`, `RtspServer`, ...) and `GStreamer::GStreamer`, which QGC links. The other modules are the helpers it includes, each described in its header. `tests/` holds script tests and a fake-SDK configure fixture, run with `ctest -L GStreamer`.

## Runtime Environment Setup

QGC configures GStreamer runtime paths before `gst_init()`:

- **Desktop (Linux/macOS/Windows)**: `GStreamer::Environment::prepareEnvironment()` sets plugin paths, scanner/helper paths, and GIO module paths when bundled runtimes are detected (never in a Linux build with `QGC_GST_BUNDLE_PLUGINS=OFF`). A bundled runtime also gets its own plugin registry, `gstreamer-<runtime version>-<arch>-<install hash>.bin` in the app's cache directory (`QStandardPaths::CacheLocation`), unless `GST_REGISTRY` or `GST_REGISTRY_1_0` is set, so it does not share the default per-user registry with the system GStreamer. The install hash keeps two installs of one GStreamer version (such as a stable and a daily build) from rescanning over each other's registry. It names what stays put across launches: the bundled plugin directory, the AppImage file (`$APPIMAGE`) rather than its mount point, which changes on every launch, and the app's name rather than the random directory macOS translocates a quarantined app to (such launches still rescan the plugins, whose recorded paths have moved). Each launch touches its registry and removes the `gstreamer-*.bin` registries there that no install has used for 30 days, such as those of an earlier GStreamer version or a replaced AppImage.
- **Linux AppImage**: the same Linux bundle detection applies to the AppImage's `usr/lib`; it is skipped when the user already set a `GST_PLUGIN_PATH*` or `GST_PLUGIN_SYSTEM_PATH*` variable. [`deploy/linux/AppRun`](../../../deploy/linux/AppRun) only sets the library, GIO and Qt environment.
- **Plugin search path**: The bundled plugin directory is set as `GST_PLUGIN_SYSTEM_PATH*`, which replaces GStreamer's default system and user plugin directories. `GST_PLUGIN_PATH*` is cleared, because it would only rescan the same directory.
- **Plugin scanner**: On desktop, GStreamer loads each new or changed plugin in its `gst-plugin-scanner` helper process, as it does by default, so a plugin that crashes while loading (a GPU driver probe, say) is blacklisted instead of taking QGC down on every launch. One scanner process loads every plugin of a scan, and GStreamer starts a new one only after such a crash. QGC points `GST_PLUGIN_SCANNER*` at the bundled scanner, and on Windows puts the app directory on `PATH` so the scanner finds the GStreamer DLLs. A missing, non-executable or failing scanner only logs a warning: GStreamer then loads the plugins in-process (the [boot self-test](#boot-self-test) fails on it). Android and iOS set `GST_REGISTRY_FORK=no`: their plugins are static, they ship no scanner, and iOS forbids `fork()`.
- **TLS certificates**: GStreamer's TLS users (RTSP over TLS, HTTPS sources) verify against GIO's default TLS database. On macOS and Windows, the bundled `gioopenssl` fills it from the system trust store (the Keychain's trust settings, the Windows `ROOT` and `CA` certificate stores), so roots an administrator or user added keep working; QGC leaves it alone and installs no CA bundle there. On Android and iOS, `gioopenssl` only knows OpenSSL's compiled-in certificate paths, which don't exist on the device, so the static plugin init makes the bundled `ca-certificates.crt` (`CA_CERTIFICATES`) the default database. On Linux, `libgiognutls` reads GnuTLS's system trust store.
- **Decoder ranks**: QGC's decoder rank changes (the video decoder setting, the zero-copy preferences) skip the features named in `GST_PLUGIN_FEATURE_RANK`, which keep the user's rank.
- **Validation**: A macOS app that ships `GStreamer.framework` without its plugin directory fails initialization with a clear error.
- **Plugin checks**: After `gst_init()`, `GStreamer::completeInit()` checks the elements each feature needs, as Qt's `qGstErrorMessageIfElementsNotAvailable` does. Only the display core is required: the `qgc` sink elements, `decodebin3`, `tee`, `queue`, `valve`, `capsfilter`, a video converter, and the RTP/UDP source with its H.264/H.265 depayloaders and parsers. Without it, initialization fails: QGC says so in a message, and plays the streams through Qt Multimedia instead. A missing optional element logs one warning naming the feature it disables: RTSP (`rtspsrc`), MPEG-TS (`tsdemux`), TCP (`tcpclientsrc`), MKV recording (`matroskamux`) or MP4/MOV recording (`mp4mux`, `qtmux`). When any element is missing, a warning also names each plugin file GStreamer blacklisted, as one it could not load.

## Platform Setup

### Linux

Use `python3 -m tools.setup.install_dependencies`, or manually:

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev
```

Debug builds also use `libgstrtspserver-1.0-dev`, when present, for MockLink's RTSP test stream and the RTSP receiver tests.

Configure finds GStreamer with pkg-config (`PKG_CONFIG_PATH`, or the toolchain's sysroot setup in cross builds) and takes its layout from `gstreamer-1.0.pc`: the plugin directory from `pluginsdir`, the plugin scanner's from `pluginscannerdir`, and the GIO modules' from `gio-2.0.pc`'s `giomoduledir`.

Unless `QGC_GST_BUNDLE_PLUGINS` is `OFF`, installs and AppImages bundle the system's allowlisted plugins, the plugin scanner and only the GIO modules for TLS and proxies (`libgiognutls`, `libgiolibproxy`, `libdconfsettings`). libproxy reads GNOME's and KDE's proxy settings itself; glib-networking's GNOME resolver (`libgiognomeproxy`) stays out because it aborts QGC wherever the `org.gnome.system.proxy` GSettings schema isn't installed. The VFS modules stay out: the AppImage, and a bundle on GLib 2.76+, run GIO with `GIO_USE_VFS=local`.

`udpsrc` before gst-plugins-good 1.24.10 (Ubuntu 24.04 ships 1.24.2) corrupts the heap when a UDP stream stops right after it starts: its `unlock()` cancels a `GCancellable` that `unlock_stop()` frees on the streaming thread. `GStreamer::completeInit()` serializes the two for those versions, as the upstream fix ([gstreamer!7897](https://gitlab.freedesktop.org/gstreamer/gstreamer/-/merge_requests/7897)) does.

#### GPU bin selection

With `gpu-zerocopy`, `qgcvideosinkbin` picks one of five variants when it is built:

- DMABuf-or-glupload (the default on Linux with gst-gl and DMABuf): the converter feeds a `switchbin`, which routes
  each stream by the memory its decoder negotiates. DMABuf and DMA_DRM go straight to Qt's DMABuf importer, as in
  `dmabuf`; GLMemory and system memory go through glupload → glcolorconvert, as in `glupload`. So a VA decoder's
  frames are imported without a copy, a software decoder's are uploaded off Qt's render thread, and a decoder that
  hands over GL textures (NVDEC) keeps them. The sink offers only the DMABuf layouts its importer takes, so a decoder
  with another layout negotiates system memory, which glupload uploads. It needs `switchbin` (gst-plugins-bad) of
  GStreamer 1.24 or later, where it offers upstream what the sink takes; without it the bin is `glupload`.
- `glupload` (the default on Android): the converter feeds glupload → glcolorconvert → GLMemory. On EGL, gst-gl
  imports LINEAR and vendor-tiled DMABuf, at the cost of its own copy/convert; on GLX it uploads system memory. It
  imports a VA decoder's tiled DMABuf as an external texture, which the driver converts to RGB: BT.709 video came out
  converted with the BT.601 matrix there (Intel, Mesa 25.2, GStreamer 1.24.2), its reds about 9% darker.
- `dmabuf`: DMABuf and DMA_DRM go straight to Qt's DMABuf importer, skipping gst-gl. It takes LINEAR and the plain
  tilings Qt's EGL display reports (see [DMABuf zero-copy diagnostics](#dmabuf-zero-copy-diagnostics)). Qt's render
  thread uploads system memory, so software-decoded video costs more than through glupload.
- `d3d12convert`: the converter feeds d3d12upload → d3d12convert → RGBA/BGRA D3D12Memory, for Qt Quick on Direct3D12
  (`QSG_RHI_BACKEND=d3d12`), whatever `QGC_GST_GPU_BIN` says. It falls back to passthrough if the d3d12 plugin lacks
  those elements.
- passthrough: the converter feeds the sink, which takes the GPU memory the renderer imports (see
  [`gstqgc`](#gstqgc--custom-gstreamer-plugin)). Windows on Direct3D11 and Apple builds use it.

`glupload` hands GLMemory to Qt through the GL share group, so it needs Qt Quick on OpenGL, over EGL or GLX. That
holds for any OpenGL on Linux and Android, so the bin doesn't wait to learn which: it may be built before the scene
graph runs, and Android can't tell EGL until then. The same goes for DMABuf-or-glupload, whose sink offers DMABuf only
once Qt's EGL display is known and asks upstream to renegotiate then; under GLX every stream takes its glupload path.
`dmabuf` hands EGLImages to Qt's EGL display, so it needs OpenGL over EGL. Under any other renderer, such as Vulkan,
the bin falls back to passthrough, which on Linux means DMA_DRM
frames under Vulkan (see [DMABuf under Vulkan](#dmabuf-under-vulkan)) and system memory otherwise: gst-gl's GLMemory
would otherwise be downloaded and uploaded again for every frame.

When both are compiled, `QGC_GST_GPU_BIN` (`glupload` or `dmabuf`, case-insensitive, read once per process) forces
one of them for every stream, and an explicit value is used even under a renderer the variant doesn't suit, with a
warning. An unknown value logs a warning and keeps the default. Builds with only one variant ignore the variable:
Android always uses glupload, Linux without gst-gl always imports DMABuf directly. The choice is logged once under
`Video.GStreamer.GstQgcGpuBin`; the `qgc` GStreamer debug category logs each bin's variant.

Qt's xcb platform, which the AppImage uses even in Wayland sessions, picks GLX before EGL. Linux builds with the GL or
DMABuf path therefore set `QT_XCB_GL_INTEGRATION=xcb_egl` at startup unless it is already set (`HwBuffers::configureQtGraphics()`, which the platform startup code calls through `VideoBackend` before Qt starts); Qt still falls back to
GLX when EGL fails to initialize. Set `QT_XCB_GL_INTEGRATION=xcb_glx` to keep GLX, e.g. for a driver whose EGL
misbehaves; video then reaches Qt as GLMemory that glupload uploads, without DMABuf import.

#### DMABuf zero-copy diagnostics

The direct DMABuf path hands each plane to Qt Multimedia's DMABuf importer as its own EGLImage, with the DMA_DRM
modifier when the caps carry one. QGC adds what Qt's importer lacks: it waits for the decoder's writes first (on the
GPU through the dma-buf's exported fence, else with a bounded `poll()`), and it keeps the buffer alive as long as the
textures, and then until the GPU has finished drawing with them where EGL exports its fences
(`EGL_ANDROID_native_fence_sync`): a V4L2 decoder doesn't wait for a buffer's readers before it decodes into it again.

- It needs Qt to render with EGL: Wayland, eglfs, or xcb with `QT_XCB_GL_INTEGRATION=xcb_egl`. Under GLX, Qt's xcb
  default, the sink offers no DMABuf caps. Nor does it until Qt's EGLDisplay is known (`QGCRhiCapture::qtEglDisplay`:
  Qt's global share context, its platform integration, or the scene graph's OpenGL context); once it is, the sink asks
  upstream to renegotiate.
- The DMA_DRM caps offer the (format, modifier) pairs Qt's EGL display reports when every plane can be sampled with
  that modifier. Plain vendor tilings (Intel X/Y/4, AMD without DCC) qualify; compressed layouts and those with
  auxiliary planes (Intel CCS, AMD DCC, Arm AFBC, Qualcomm UBWC) don't. Buffers negotiated with plain
  `memory:DMABuf` caps import without a modifier, leaving the layout to the driver.
- `EGL_BAD_MATCH` (a broken EGL/driver pairing, QTBUG-112312), or 30 failed imports in a row of one layout (modifier,
  format and size, so that another stream's imports don't end the run), turn the path off until the scene graph is
  recreated: the sink stops offering DMABuf and DMA_DRM caps and asks upstream to renegotiate. Until upstream has,
  LINEAR frames take the CPU path. An import the driver refused counts, and so does any failed import of a tiled
  frame, which nothing else can show.
- A tiled frame (a DMA_DRM modifier other than LINEAR) has no CPU layout: when no import takes it, it is dropped rather
  than shown scrambled, and counted in the sink's `tiled-drops` frame stat.
- The `Video.GStreamer.HwBuffers.GstDmaBuf` and `Video.GStreamer.HwBuffers.GstDmaDrmCaps` debug categories log the
  offered modifiers and the first import; Qt's `qt.multimedia.dmabuf` category logs its EGL calls.

`GstHwBuffersGlReadbackTest` reads imported DMABuf planes back from a GL QRhi. CTest runs it on xcb with `xcb_egl` and
skips it without an X display; a plain memfd stands in for a dma-buf under Mesa's software rasterizer, and a VA-API
decoder's tiled output is compared with its system-memory output on real hardware.

#### DMABuf under Vulkan

Qt Quick renders with OpenGL on Linux unless told otherwise; `QSG_RHI_BACKEND=vulkan` selects Vulkan
instead. The DMABuf path then imports each plane of a frame as a single-plane VkImage on Qt's device, bound to the
dma-buf through `VK_EXT_external_memory_dma_buf` with the frame's modifier (`VK_EXT_image_drm_format_modifier`), in
the format Qt samples that plane as (R8, RG8, R16, RG16, RGBA8 or BGRA8). That is what Qt's EGL importer does with one
EGLImage per plane, so NV12, P010 and I420 need no multi-planar image and no YCbCr sampler. Built with
`QGC_GST_ENABLE_DMABUF_VULKAN` (on by default where Qt has Vulkan support and found the Vulkan headers); it needs
neither gst-vulkan nor a link to the Vulkan loader.

- Qt's device must have `VK_KHR_external_memory_fd`, `VK_EXT_external_memory_dma_buf` and
  `VK_EXT_image_drm_format_modifier` enabled, so QGC adds them to `QT_VULKAN_DEVICE_EXTENSIONS` before Qt starts;
  QRhi enables those the device has. When the scene graph starts, the device is asked once (`probeDevice()`) for the
  modifiers it imports each plane format with as a sampled image with one memory plane. It imports nothing when it
  lacks an extension or Vulkan 1.2: DMABuf frames then take the CPU path, and the
  `Video.GStreamer.HwBuffers.GstDmaBufVulkan` debug category says why.
- The sink offers DMA_DRM caps only, for the (format, modifier) pairs every plane of which the device imports. Vulkan
  takes no implicit layout, so it offers no legacy `memory:DMABuf` caps; a dma-buf that arrives under plain
  `video/x-raw` caps, as a V4L2 decoder exports it, is imported as LINEAR.
- Vulkan doesn't wait on a dma-buf's implicit fences, and QRhi's submission can't be handed the producer's fence, so
  the import first blocks, for at most 100 ms, until `poll()` reports the producer's writes complete, and takes the
  CPU path when it doesn't. The frame's buffer stays referenced until the GPU has finished the last frame that sampled
  its images, so the decoder can't write the next picture into it meanwhile. A thread of its own waits for that and
  hands the buffer back, so a decoder with a fixed pool gets it without another frame being drawn. The images and
  their memory are destroyed on the render thread, when it next releases a frame or its QRhi is destroyed.
- The import gives QRhi the images in `VK_IMAGE_LAYOUT_GENERAL`, the layout that keeps a foreign producer's contents,
  without the queue-family transfer from `VK_QUEUE_FAMILY_FOREIGN_EXT` the specification describes: QRhi issues the
  barriers and has no hook for one.
- Failed imports count towards the DMABuf path's disable latch as under OpenGL. One that failed because a plane's
  dma-buf fd could not be duplicated, as when the process is out of file descriptors, is no driver's refusal: it is
  warned about once, with the `errno`, counted as `fd-dup-failed` among the path's fallback reasons, and counts
  towards the latch only for a tiled frame, which has no other way to the screen.

`GstHwBuffersVulkanReadbackTest` imports VA-API frames (a decoder's NV12, and I420, P010, RGBA and BGRA from the
postprocessor, in whichever of the offered layouts the driver picks) into a Vulkan QRhi and compares
`QVideoFrame::toImage()` with the system-memory frame; it also plays a VA stream into `qgcvideosinkbin` under a
Vulkan render target. CTest runs it on xcb; it skips without a Vulkan device that imports dma-bufs, as Mesa's lavapipe
doesn't. Verified on Intel (Mesa ANV with the iHD VA driver, LINEAR and Tile4 layouts) only.

### macOS / Windows / iOS

GStreamer SDKs are auto-downloaded during CMake configure. To use a local installation instead, set `GStreamer_ROOT_DIR` (on iOS, a `GStreamer.xcframework` or `GStreamer.framework`, or a directory containing one). Configure fails when that SDK is older than the requested version (`gstreamer.version.<platform>` or `QGC_GST_VERSION`) or, on Windows, incomplete; an SDK Windows finds through `GSTREAMER_1_0_ROOT_*` or a standard install location is replaced by a download instead.

On macOS, configure uses, in this order: `GStreamer_ROOT_DIR`; Homebrew's GStreamer when `QGC_GST_USE_HOMEBREW` is `ON`; the SDK an earlier configure downloaded; `/Library/Frameworks/GStreamer.framework`; a new download. CI's macOS jobs install no framework, so they build, test and package with the download. Homebrew's GStreamer (`brew install gstreamer pkgconf`) is also used when `GStreamer_ROOT_DIR` is a Homebrew prefix (`$(brew --prefix)`) or one of its `opt/gstreamer` or `Cellar/gstreamer/<version>` directories. Its pkg-config modules, and GLib's, are read with Homebrew's own `pkg-config`, which also finds the macOS system libraries' modules; `gstreamer.version.macos` (or `QGC_GST_VERSION`) is its minimum version. Such a build only runs from the build tree: Homebrew's libraries and plugins load each other by absolute paths under its prefix, so `cmake --install` and packaging fail with an error instead of bundling them.

Windows and macOS installs bundle the allowlisted plugins, the GIO modules and the helpers, and only the SDK libraries that those and QGC itself load, which `install(RUNTIME_DEPENDENCY_SET)` resolves at install time (libproxy's backends, which are loaded with `dlopen`, are installed as files too; a macOS bundle has them in `Contents/Frameworks/libproxy`, where libproxy loads them as `@rpath/libproxy/<backend>`). A macOS `GStreamer.framework` is copied whole, without its GPL libraries.

On iOS only the `GStreamerMobile` framework links the SDK's merged static archive; `GStreamer::GStreamer` carries just its headers, defines and system-framework links. The app takes every GStreamer function from the framework, which exports those its registered plugins use plus `QGC_GST_APP_CORE_SYMBOLS` (`Mobile.cmake`). A new core call that no plugin uses therefore fails the app's link with an undefined-symbol error naming it: add the symbol to that list. The framework exports GStreamer's and GLib's API only (an exported-symbols list of name patterns, `platform/IOS.cmake`): the merged archive also holds FFmpeg, libpng, libjpeg and zlib, and the app's Qt code must link to Qt's own, which for FFmpeg are the frameworks `qt_add_ios_ffmpeg_libraries()` adds ([`cmake/platform/Apple.cmake`](../../../cmake/platform/Apple.cmake)). A post-build check (`SingleCoreCheck.cmake`) fails if the app still ends up defining core symbols. The plugin descriptors the configure reads from the merged archive (over 1 GB per `GStreamer.xcframework` slice) with `nm` are cached in the build directory, keyed on the archive's path, size and modification time. From 1.28.8 on, the iOS download is a tarball holding a `GStreamer.xcframework` instead of a `.pkg`, and only the xcframework's iOS slices are extracted, skipping its ~2 GB of tvOS slices.

### Android

Auto-downloaded during CMake configure. No manual setup required. Each ABI build extracts only its own tree from the universal SDK tarball. Windows and macOS hosts query it with the pkg-config the SDK bundles for them (`share/gst-android/ndk-build/tools/<host>/`); Linux hosts use their own.

As on iOS, and as the SDK's own `FindGStreamerMobile.cmake` does, the app does not link the SDK's static archives: `GStreamer::mobile` is a shared `libgstreamer_android.so` (`platform/Android.cmake`) holding GStreamer, the allowlisted plugins, the OpenSSL GIO module and the generated `gst_init_static_plugins()`. It exports GStreamer's and GLib's API and hides the third-party libraries inside it (FFmpeg, OpenSSL, ...); `QGC_GST_APP_CORE_SYMBOLS` and the post-build `SingleCoreCheck.cmake` work as on iOS. The library links no Qt target, so on the 64-bit ABIs it sets the 16 KB segment alignment (`-z max-page-size=16384`) Qt gives Qt-linked targets for its `android_16kb_pages` feature: devices with 16 KB pages cannot load a library aligned for 4 KB, and Google Play rejects it. CI's [`android_apk_check.py`](../../../tools/ci/android/android_apk_check.py) checks every library in the APK for that alignment and for dependencies the APK or the NDK does not provide. Qt's deployment packages the library into the APK and loads it before the app. GStreamer's `androidmedia` plugin finds the Java VM and the app's class loader through `gst_android_get_java_vm()` and `gst_android_get_application_class_loader()` in that library, which return the JNI state `src/Android/AndroidInit.cc` hands it with `qgc_gst_android_set_jni_state()` at `JNI_OnLoad` and when the activity starts (`Platform/gst_android_jni.c`), so the SDK's `GStreamer.java` and `GStreamer.init()` are not used.

The plugins' Java sources and GStreamer's fonts and CA bundle are staged in the build directory (`extra_java_sources/`, `extra_assets/`), which [`android/build.gradle`](../../../android/build.gradle) adds to the APK.

> **Windows users building for Android**: Enable Developer Mode (Settings > System > For developers) to support symbolic links during the build.

### Caching downloaded SDKs

Auto-downloaded SDK archives are cached to `${CPM_SOURCE_CACHE}/gstreamer-*`. `CPM_SOURCE_CACHE` defaults to `.cache/CPM` in the source tree; set it to a location shared by your checkouts (e.g. `~/.cache/CPM`) to download the archives only once (GStreamer 1.28.8: 0.3-1.2 GB per platform, e.g. ~1 GB for Android, 1.2 GB for iOS and 540 MB for Windows x64):

```bash
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
```

CI caches the extracted trees under a key covering the `gstreamer` section of `.github/build-config.json` and `cmake/GStreamer/{Packages,Acquire}.cmake`, so keep everything that decides an extracted tree in those two files. Cached archives are verified on every hit against the checksum pinned in `.github/build-config.json`, or the upstream `.sha256sum` sidecar for an unpinned version; an auto-downloaded SDK is fetched again when its `gstreamer.version.<platform>` changes, and cached GStreamer probe results are recomputed whenever the SDK changes. Each archive is extracted once into a directory beside it that is stamped with the archive hashes, and a tree stamped with the pinned checksums is used without downloading or even keeping its archives (CI caches only the trees); configures sharing the cache take a lock instead of downloading or extracting over each other. The macOS tree is the SDK root merged from the runtime and devel packages, without their static libraries (0.8 GB instead of 4 GB for 1.28.8), which QGC does not link. The Windows SDK installer runs in its portable mode, so it leaves no `GSTREAMER_1_0_ROOT_*` environment variable, registry key or uninstall entry behind.

## Testing Pipelines

### Sending test video (h.264 over UDP)

```bash
gst-launch-1.0 videotestsrc pattern=ball ! video/x-raw,width=640,height=480 ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5600
```

### Receiving test video

```bash
gst-launch-1.0 udpsrc port=5600 caps='application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264' ! rtpjitterbuffer ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink sync=false
```

### Supported protocols

`GstSourceFactory` accepts these URI schemes:

- `rtsp://`, `rtspt://`: RTSP (the video stream only), over UDP and, when no UDP data arrives within 5 s, over TCP;
  `rtspt://` over TCP only.
- `udp://`: H.264 over RTP/UDP; `udp265://`: H.265 over RTP/UDP. Port 0 listens on a free port, and a URI without a
  host (`udp://:5600`) on every IPv4 address. A port that another listener holds fails to bind: `udpsrc` would share
  it (`reuse`), and of two listeners on a unicast address only one gets a stream's packets. Only the port of a
  multicast group is shared.
- `mpegts://`: MPEG-TS over UDP. `tsdemux` adds its `latency` to that of a live stream, 700 ms by default: it is
  given the RTP jitter latency instead, and 100 ms at least, for the frame interval it holds each frame back by.
- `tcp://`: MPEG-TS over TCP. `tcpclientsrc` connects inside the state change, on the receiver's worker, so it
  has a `timeout` of 3 s: without one it waits as long as the OS does for a host that doesn't answer.

Both MPEG-TS sources store the parsed caps on the stream object that `tsdemux` sends with the stream. `decodebin3`
picks the decoder by that object's caps when the stream starts, and stores the parsed caps on it only then, on
another thread; `tsdemux` gives it those of the unparsed stream, which no decoder takes, and `parsebin` leaves them.
On a busy machine `decodebin3` could read them first and end the stream for want of a decoder.

### Automated tests

A test is in `test/VideoManager` at the path its source has in `src/VideoManager`: the tests of `Pipeline/` are in
`test/VideoManager/GStreamer/Pipeline/`, those of `Sink/gstqgc/` in `test/VideoManager/GStreamer/Sink/gstqgc/`. Code
in a `common` folder is tested from its parent's folder. Support that several folders share is in their nearest common
parent: `GStreamerTestBase` and `GStreamerTestHelpers` in `test/VideoManager/GStreamer/`, `VideoTestHelpers.h` in
`test/VideoManager/`. The cases of `GstHwBuffersTest` and `GstHwBuffersPipelineTest` are spread over part files that
define no class of their own, the cases of one GPU path in that path's folder
(`HwBuffers/dmabuf/GStreamerDmaBufTest.cc`).

Each class has a line here; the comment on the class in its header says more.

| Test | Label | Covers |
| --- | --- | --- |
| `GStreamerEnvironmentTest` | Unit | The environment `prepareEnvironment()` sets, and the bundled runtime's private plugin registry. |
| `GStreamerInitTest` | Unit | Init policy: log routing, missing elements, decoder ranks, and the boot self-test's verdict on the plugin scanner and a bundle's plugins. |
| `GStreamerTest` | Unit | The `GStreamer` facade without a running pipeline: decoder classification, recording sinks, init and the `VideoBackend` that forwards to it. |
| `GstQgcSinkTest` | Unit | `qgcqvideosink` and `qgcvideosinkbin` as elements, without a pipeline. |
| `GstQgcSinkControllerTest` | Unit | `QGCQVideoSinkController` binding `qgcqvideosink` to a `QVideoSink`. |
| `GstQgcVideoFormatsTest` | Unit | The sink's format tables. |
| `GstSourceFactoryTest` | Unit | The source bin built for each URI, without a stream. |
| `GstHwBuffersTest` | Unit | The `HwBuffers` paths without a pipeline or a GPU. |
| `GstQgcSinkHarnessTest` | Unit | `qgcvideosinkbin` under GstHarness, bound to a `QVideoSink`. |
| `GstSourceFactoryHarnessTest` | Unit | The parameter-set gate under GstHarness: on NAL units pushed by hand, and on a real H.264/H.265 depayloader. |
| `GstVideoReceiverComponentsTest` | Unit | The receiver's components on their own, without a running pipeline. |
| `GstQgcSinkPipelineTest` | Integration | `videotestsrc` pipelines played into `qgcvideosinkbin` bound to a `QVideoSink`. |
| `GstSourceFactoryStreamTest` | Integration | Source bins receiving real streams on loopback. |
| `GstRecordingSinkTest` | Integration | The recording file sink fed by running pipelines, with each file played back. |
| `GStreamerHelpersTest` | Integration | The pipeline helpers, and the `TeeBranchHead` every tee branch starts with. |
| `GstSelfTestPipelineTest` | Integration | The [boot self-test](#boot-self-test) against the GStreamer the tests run on. |
| `GstHwBuffersPipelineTest` | Integration, GPU | Pipelines that upload into or decode to GPU memory; each test skips without the device. |
| `GstHwBuffersGlReadbackTest` | Integration, GPU | GPU frames imported into a GL QRhi and read back (xcb with `xcb_egl`). |
| `GstHwBuffersVulkanReadbackTest` | Integration, GPU | DMABuf frames imported into a Vulkan QRhi (xcb), and the gst-vulkan bridge with `QGC_GST_ENABLE_VULKAN`. |
| `GstVideoReceiverPipelineTest` | Integration | `GstVideoReceiver` end to end: states, display, restarts, decoders and screenshots. |
| `GstVideoReceiverReconnectTest` | Integration | How `GstVideoReceiver` gives up on, or reconnects to, a stream that fails or stops, and what it reports of a stuck worker. |
| `GstVideoReceiverRecordingTest` | Integration | `GstVideoReceiver`'s recordings: playable files across errors, reconnects, slow or hung storage and teardown. |
| `GstVideoReceiverStatsTest` | Integration | The stream statistics measured from real H.264 and H.265 streams. |

A build without GStreamer (`QGC_ENABLE_GST_VIDEOSTREAMING=OFF`) has none of these classes:
`test/VideoManager/GStreamer` isn't added to it. The classes derived from `GStreamerTestBase` fail in `initTestCase()`
when `GStreamer::completeInit()` fails, as it does when an element video needs is missing, and their tests skip where
GStreamer can't be initialized at all. A test skips, rather than fails, when an optional
element it uses is missing (`GSTTEST_SKIP_UNLESS_FACTORIES` in `GStreamerTestHelpers.h`). The two `*HarnessTest`
classes use GstHarness from gst-check (`gstreamer-check-1.0`), which test builds request as an optional GStreamer
component (`Check`); they compile only where the SDK or distribution ships it. Receivers and servers in every
`GstVideoReceiver*Test` listen on port 0, so parallel runs never collide. Each class runs in its own process, so a
crash in a GPU path only fails its own class; `-LE GPU` leaves out the classes that use the host's GPU.

No test needs a software encoder: the streams they send are the short 320x240 H.264 and H.265 files, and a 160x120
H.264 one a stream switches to mid-way, in [`SelfTest/Media/`](SelfTest/Media/), which the app's self-test embeds too
(its `generate.sh` regenerates them), split into access units and paced through `appsrc`. Only three cases encode in
software: `GstVideoReceiverPipelineTest`'s `_testDecodesMockLinkStreams` and `_testDecoderThreads`, as MockLink's
stream server does (`openh264enc`, falling back to `x264enc`; `x265enc` for H.265), and
`GstVideoReceiverRecordingTest::_testCodecMp4CannotHoldIsRecordedAsMatroska`, which sends MPEG-2 video (libav's
`avenc_mpeg2video`) and skips without that encoder; the VA-API cases encode with the host's `vah264enc`.

```bash
ctest --test-dir build -R '^(GStreamer|Gst)[A-Za-z]*Test(\.[0-9]+)?$' --output-on-failure
```

CI runs these classes in the Linux coverage and sanitizer jobs and the macOS Debug leg with the rest of the suite, and
on Windows in `windows.yml`'s `gstreamer-tests` job, a Debug build that runs only the GStreamer and video classes
against the SDK's shipped plugins plus the sources and encoders the tests send streams with, with the D3D tests on WARP.
The packages are checked beyond their boot test: every DLL the Windows install imports must ship or come with Windows
(`pe_dependency_audit.py`), every library a Mach-O file in the macOS bundle loads must resolve inside the bundle or to
macOS (`macho_bundle_audit.py`), and the AppImage must boot in clean Ubuntu 24.04 and Debian 13 containers without
GStreamer or Qt (`appimage_container_boot.py`); see
[`.github/ci-notes/platforms.md`](../../../.github/ci-notes/platforms.md#platform-notes).

### Boot self-test

`QGroundControl --simple-boot-test`, which CI runs on every package (the installed Windows app, the macOS DMG, the
Linux build tree and AppImage, the Android emulator and the iOS simulator), initializes video and then runs
`GStreamer::SelfTest::run()` (`GStreamerSelfTest.cc`, through `VideoBackend::selfTest()`). The boot test fails when any of
these checks fails, each logged as a critical message under `Video.GStreamer.SelfTest` that names the plugin, element
or file:

- **Bundled plugins** (a desktop runtime bundled with the app, as `GStreamerEnvironment` set it up; not a build tree
  using the system GStreamer, nor a `QGC_GST_BUNDLE_PLUGINS=OFF` build): no plugin in the bundle's plugin directory is
  blacklisted, as one whose library dependency is missing is; every plugin GStreamer registered from a file comes from
  that directory; and the registry and the directory hold the same plugin files.
- **Plugin scanner** (bundled desktop runtimes): GStreamer did not log `External plugin loader failed` during init,
  i.e. the bundled `gst-plugin-scanner` ran. GStreamer for Windows tries the scanner before a scan and mentions one
  that doesn't start only in its debug log, so there the self-test starts the bundled scanner the same way: without
  arguments it must exit with 1 within 5 s.
- **Features**: every feature in `GStreamer.cc`'s plugin table has its elements. Unlike initialization, which needs
  only the display core, the self-test requires them all, since `gstreamer.plugins.common` and
  `gstreamer_plugin_policy.bundle_required` ship them on every platform.
- **GIO** (bundled runtimes, Android and iOS): GIO's default TLS backend supports TLS and has a certificate database;
  where the bundle ships the libproxy GIO module, libproxy looks up the proxy for `rtsp://127.0.0.1`, which loads the
  backend libproxy opens by name.
- **Decoding**: the 160x120 H.264 sample from [`SelfTest/Media/`](SelfTest/Media/),
  embedded in the app, decodes through `decodebin3` with the decoder ranks the app applied, to at least one raw frame;
  so does the 320x240 H.265 sample when an H.265 decoder is ranked for autoplugging (NVIDIA's HEVC decoders reject
  frames under 144 pixels high). When a hardware decoder fails mid-stream
  (as in a VM; `decodebin3` replaces only one that fails to start), the check is retried once with software decoders
  forced, then the ranks are restored; a software decoder that works only logs a warning.
- **Recording**: the H.264 sample, muxed by the recording branch's file sink into MP4 and MKV, makes a non-empty file
  that `qtdemux` or `matroskademux` reads back.

The whole self-test takes well under a second on a working runtime, gives each pipeline at most 4 s and stops within
15 s. It is quiet on success but for one summary line at info level, shown with
`--logging:Video.GStreamer.SelfTest`.

## GStreamer Logging

- In-app: The GStreamer debug level setting (Application settings) sets GStreamer's default threshold, and QGC forwards GStreamer's log to the `Video.GStreamer.GStreamerAPI` category; enable that category's debug output in QGC's logging settings to see levels below Info. Release builds forward up to the Debug level. The saved level and the forwarding are in place before `gst_init()` on the next launch, so what GStreamer logs while it initializes (the plugin scan) is forwarded too.
- Native output: Setting `GST_DEBUG`, `GST_DEBUG_FILE` or `GST_TRACERS` keeps GStreamer's own log handler and turns the forwarding off. The log then goes to stderr, or to `GST_DEBUG_FILE`, in GStreamer's format, which its tools (`gst-stats`, tracer plotters) parse, in every build type. `GST_DEBUG` replaces the in-app level, and `GST_DEBUG_NO_COLOR=1` drops the color codes. See <https://gstreamer.freedesktop.org/documentation/gstreamer/running.html>.
- Command line: QGC's option parser rejects options it doesn't know, GStreamer's (`--gst-debug-level`, `--gst-debug`, ...) among them, so use the setting or the environment variables. `GStreamer::initialize()` does hand the arguments it is given to `gst_init_check()`, and keeps a threshold they set.
- GLib messages (`g_warning()`, `g_critical()`, ...) always go to the `Video.GStreamer.GStreamerLogging` category.
- The [boot self-test](#boot-self-test) logs its failures under `Video.GStreamer.SelfTest`, and its summary at info level.
- Receiver flow (starts, stops, reconnects, the steps of an RTSP connection, the decoder chosen, pipeline latency, GPU path statistics): enable the `Video.GStreamer.GstVideoReceiver` category. The stream's health is also measured continuously: see [Stream statistics](#stream-statistics). Failures are logged as warnings even with it off, with the failing element's debug string. The message the user gets ends with the cause taken from it: what the element said (`bind failed: ...`), the RTSP status of a refused request, or the plugin an element reported missing.

## Pipeline Observability

QGC supports the standard GStreamer observability env vars. Set them before launching QGC. `GST_DEBUG`, `GST_DEBUG_FILE` and `GST_TRACERS` switch to GStreamer's native log output (see [GStreamer Logging](#gstreamer-logging)), so tracer records keep the format the GStreamer tools expect. The tracers below (`latency`, `dots`, `leaks`) are in GStreamer's `coretracers` plugin, which the plugin allowlist includes, so an installed QGC has them too.

### Pipeline graph dumps (`.dot` files)

When `GST_DEBUG_DUMP_DOT_DIR` is set, QGC writes each receiver's pipeline graph there at key transitions as `receiver-<stream>-<tag>.dot`. `<stream>` is the receiver's name (`videoContent`, `thermalVideo`, or `videoTile<N>` for the N-th stream that got a tile, which keeps the name if the stream becomes the main picture), so the receivers don't overwrite each other's files, and `<tag>` names the transition (`pipeline-initial`, `pipeline-started`, `pipeline-with-decoder`, `pipeline-with-videosink`, `pipeline-with-filesink`, `pipeline-error`, `pipeline-watchdog-timeout`, `pipeline-decoder-timeout`, `pipeline-recording-stopped`, …). A transition's next dump overwrites its last one.

```bash
export GST_DEBUG_DUMP_DOT_DIR=/tmp/qgc-pipeline-dots
./QGroundControl
dot -Tpng /tmp/qgc-pipeline-dots/receiver-videoContent-pipeline-started.dot -o pipeline.png
```

When the env var is **unset**, QGC instead writes a snapshot to `<CacheLocation>/qgc-pipeline-dot/receiver-<stream>-<tag>-<UTC timestamp>.dot` on a pipeline `ERROR` and on a source or decoder watchdog timeout, keeping the 10 newest, so field-bug-report bundles include the topology automatically.

Every dump shows topology, caps, media types and states, but leaves out element properties, which can hold stream credentials (such as `rtspsrc`'s `user-id` and `user-pw`).

### Latency tracer

Per-element latency from source to sink:

```bash
GST_TRACERS="latency(flags=pipeline+element)" GST_DEBUG=GST_TRACER:7 GST_DEBUG_FILE=latency.log ./QGroundControl
```

Plot the resulting `element-latency` / `latency` log lines with any of the GStreamer community tools (e.g. `gst-stats`, GStreamerLatencyPlotter).

### Live pipeline visualizer (`gst-dots-viewer`, GStreamer ≥ 1.26)

```bash
GST_TRACERS=dots ./QGroundControl &
gst-dots-viewer
# open http://localhost:3000
```

The viewer streams the live pipeline graph over WebSocket and exposes a snapshot button — no `xdot` viewer needed.

### Leak / lifetime debugging

```bash
QGC_GST_DEINIT=1 GST_TRACERS=leaks GST_DEBUG=GST_TRACER:7 ./QGroundControl
```

Logs every `GstObject` / `GstMiniObject` that wasn't released by exit. Useful when triaging pad-leak regressions in the source factory or sink bin. The leaks tracer reports when GStreamer is deinitialized: `QGC_GST_DEINIT=1` makes QGC call `gst_deinit()` as it exits, after its receivers are gone. It is opt-in, like Qt's `QT_GSTREAMER_DEINIT`, because GStreamer can't be used again in the process afterwards.
