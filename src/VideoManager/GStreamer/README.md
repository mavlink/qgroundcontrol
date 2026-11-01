# GStreamer Video Streaming

QGroundControl uses GStreamer for UDP RTP and RTSP video streaming in the Main Flight Display.

This guide covers the GStreamer receiver architecture, platform setup, and diagnostics. See
[CODING_STYLE.md](../../../CODING_STYLE.md) for repository-wide source conventions,
[test/README.md](../../../test/README.md) for the test framework, and
[tools/README.md](../../../tools/README.md) for general build and tooling commands.

## Source Code Architecture

The backend is split into focused components, by folder:

- `Runtime/`: initialization, environment, logging, helpers, the boot self-test and `GStreamerVideoBackend`.
- `Pipeline/`: the receiver, its worker, sessions and branches, and the source bins.
- `Sink/`: `QGCQVideoSinkController` and the `gstqgc` plugin (`Sink/gstqgc/`).
- `HwBuffers/`: the zero-copy GPU paths.
- `Platform/`: platform shims (`IOSPipe2Compat.c`) and the Android JNI state `GStreamer::mobile` keeps for GStreamer's Android plugins (`gst_android_jni.c`).

The backend-neutral `VideoReceiver`, `VideoBackend` and `VideoStreamStats` are in `../Receiver/`. `../QtMultimedia/`
holds the other `VideoReceiver`, `QtMultimediaVideoReceiver`, which plays streams through `QMediaPlayer` (FFmpeg): in
builds without GStreamer, and where `VideoSettings::videoBackend` picks it.

| Component | Role |
| --- | --- |
| `GstVideoReceiver` | The `VideoReceiver` the app uses. Callers state what they want (config, active, video output, recording), which it hands to its `GstReceiverWorker`. It owns the display sink, built from its `VideoReceiver::DisplayConfig` when an output is set and rebuilt by `restart()`. |
| `GstReceiverWorker` | Runs on the receiver's worker thread and keeps one `PipelineSession` in line with what the receiver was asked for, reconnecting with backoff after a stream failure and giving up (`State::Stopped` plus `errorOccurred`) when reconnecting is off or can't help (unsupported URI, missing plugin). Reports back to the receiver through queued signals. |
| `PipelineSession` | One pipeline (source → tee → the branches below) and everything about it: elements, bus, probes. Lives on the worker; replaces a decoder that posts an error by itself (a stalled one when the worker's watchdog asks) and reports failures of the stream to the worker. |
| `DecodingBranch` | Display branch behind the tee: inserts `decodebin3` once the source streams, asks upstream for a keyframe, and reports decoding and the decoder name. Its 2-buffer queue drops the oldest frame rather than hold up the tee; after a drop it also drops delta frames until the next keyframe, so the picture freezes briefly instead of smearing. |
| `RecordingBranch` | Recording branch behind the tee: a `parser ! muxer ! filesink` bin for each file the session is asked to record, started at a keyframe (requested upstream) and, for MP4/MOV, muxed so a crash leaves a playable file. After a reconnect the recording goes on in `<base>_part<N>.<ext>`. Its queue holds 64 MB of backlog for slow storage, then ends the recording rather than stall the display. `RecordingFinalizer` tells when the file is closed. |
| `BranchContext` | What `PipelineSession` lends its branches: the pipeline, the tee, the watchdog, the dot dumper and a way to queue work on the worker. Also the branch marks (`GStreamer::markBranch()`, `GStreamer::branchOf()`) that blame a bus message on the branch it came from. |
| `StreamWatchdog` | Notices stalled source or decoder output on a monotonic clock (allowing `rtspsrc` its UDP→TCP fallback before the first frame), and paces reconnects with exponential backoff. |
| `PipelineDotDumper` | Writes the pipeline's `.dot` graphs; see [Pipeline Observability](#pipeline-observability). |
| `GstSourceFactory` | Constructs the source element from the stream URI and applies RTP jitter-buffer policy for `application/x-rtp` sources. |
| `GstSourceUri` | The transport each video URI scheme selects (`rtsp*`, `udp`, `udp265`, `mpegts`, `tcp`), without GStreamer. |
| `GstRtspSource` | The `rtspsrc` the source factory builds: the RTSP URI check, transports, timeouts, latency, retransmission timing and video-only stream selection. |
| `GstH26xParameterSetGate` | H.264/H.265 setup for the depayloaders and parsers the source factory builds or `parsebin` plugs: parameter sets repeated at every IDR, keyframe requests after loss, and output held until a full parameter set arrives. |
| `QGCQVideoSinkController` | GUI-thread companion of the display sink; see [`gstqgc`](#gstqgc--custom-gstreamer-plugin). |
| `GStreamer` | The facade QGC calls, through `VideoBackend` where it must also build without GStreamer: initialization (`initialize()`, with `completeInit()` checking the plugin features), the display sink (`createVideoSink()` configured from the receiver's `VideoReceiver::DisplayConfig`, bound to a `QVideoSink` by `setupQVideoSinkElement()`), and `createVideoReceiver()`. |
| `GStreamerEnvironment` | Process-wide environment setup (plugin/scanner/helper/GIO paths, private registry, mobile CA file) run before `gst_init()`. See [Runtime Environment Setup](#runtime-environment-setup). |
| `GStreamerDecoderRanks` | Video decoder classification (hardware or software, and the `forceVideoDecoder` family) and the autoplug rank overrides behind that setting, which a change re-applies from the decoders' original ranks; also the decoder families offered and the rank dump logged under `Video.GStreamer.DecoderRanks`. |
| `GStreamerHelpers` | Small utilities: tee branch heads, keyframe requests, events sent without blocking, holding a new pad's data until it is linked, and taking a child element out of a running bin. |
| `GStreamerLogging` | Routes GStreamer's log output through QGC's logging, unless GStreamer's own output was requested, and applies the persisted debug level. See [GStreamer Logging](#gstreamer-logging). |
| `GStreamerSelfTest` | `GStreamer::selfTest()`, which the boot test runs to prove the packaged runtime decodes and records. See [Boot self-test](#boot-self-test). |
| `GStreamerSelfTestMedia` | The self-test's media pipelines: decoding the embedded samples and recording and reading back MP4 and MKV. |
| `GstScoped` | RAII owners for transfer-full GStreamer returns so refs can't leak on early return, `makeElement()` (never floating), `PadProbe` and `ScopedSignal`. |
| `GStreamerVideoBackend` | The `VideoBackend` implementation: the only way the app reaches GStreamer's lifecycle (environment, init, self-test, decoder priorities, debug level, Android JNI state). Settings reach the backend as values: `VideoReceiver::Config`, `VideoReceiver::DisplayConfig` and the persisted debug level. |

MockLink's `MockVideoStreamServer` (`src/Comms/MockLink`, Debug builds only) serves a synthetic `videotestsrc` stream (RTP H.264 or H.265 over UDP, RTSP, MPEG-TS over UDP or TCP) that MockLink advertises, so the receiver runs without a vehicle; builds without GStreamer compile a null server instead.

### Threading

- **GUI thread**: `GstVideoReceiver`'s API and every signal it emits, the display sink's `QGCQVideoSinkController`,
  and the video size, read from each frame the output's `QVideoSink` shows: its viewport (decoder padding and crop left
  out), turned as Qt presents it.
- **Worker thread** (one per receiver, running its `GstReceiverWorker`): the reconnect logic, every change to the
  pipeline, and its bus. A sync handler only does what can't wait: answers `NEED_CONTEXT`, selects the video streams,
  and closes the valve in front of a branch that posted an `ERROR`, so the error stays out of the tee. Everything else
  is popped on the worker, which a `QSocketNotifier` (`QWinEventNotifier` on Windows) on the bus's poll fd wakes up,
  as Qt's `QGstBusObserver` does.
- **Streaming threads**: probes and element signals only note what happened and queue the rest on their
  `PipelineSession`, which drops it once the pipeline is gone.
- **Teardown** is bounded: an open recording drains through an EOS sent from GStreamer's thread pool, the worker waits
  for it at most 3 s and then flushes the recorder, and the receiver's destructor waits at most 5 s for the worker,
  leaving one stuck in the kernel (a hung network mount) to finish on its own. The worker reaches the receiver only
  through signals queued to it, which Qt drops once the receiver is gone.

### `gstqgc` — custom GStreamer plugin

A QGC-owned plugin (`gstqgc/`) bridging the pipeline into Qt:

- **`qgcqvideosink`** — a `GstVideoSink` that pushes decoded frames into a Qt `QVideoSink`. Delivery keeps only the
  latest frame, so a stalled GUI thread never holds more than one undelivered frame (or the decoder buffer behind it).
  Frames carry the rotation and mirroring of the stream's `image-orientation` tag, as a camera mounted sideways sends
  it, unless a buffer has its own orientation meta. Its construct-only `gpu-zerocopy` property (passed down by the bin) enables zero-copy import of GPU memory, and its
  `frame-stats` property reports that sink's own frame counts per delivery path, GPU fallbacks, dropped tiled frames
  (`GstQgc::formatSinkStats` in `GstQgcSinkStats.h` formats them) and delivered frames a newer one replaced before the `QVideoSink` took them
  (`superseded-frames`); GstBaseSink's standard `stats` property is left as is.
- **Sink caps** — `qgcqvideosink` answers caps queries itself (`GstQgc::buildSinkCaps`), with the formats Qt renders
  (`kVideoFormatTable`):
  - System memory always, with `pixel-aspect-ratio=1/1` unless the bin's `disable-par` is set.
  - With `gpu-zerocopy`, ahead of it, only the GPU memory the renderer Qt Quick uses can import. Under OpenGL: DMA_DRM
    and DMABuf on EGL while the DMABuf import works (see
    [DMABuf zero-copy diagnostics](#dmabuf-zero-copy-diagnostics)), AHardwareBuffer on EGL (Android), and GLMemory,
    which reaches Qt through the GL share group, on EGL or GLX. Under Direct3D11: D3D11Memory. Under Direct3D12, the
    d3d12convert variant (below). Vulkan, Metal and OpenGL on WGL or CGL get system memory only. vtdec's
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
  sink doesn't take (such as a software decoder's `Y444`), and passes GPU memory through.
- **Allocation proposal** — the sink asks the decoder for at least five buffers (`kMaxPinnedFrames` + 1), covering the
  frames Qt may still hold, and keeps the video, crop and GL sync metas it reads.
  `QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS` overrides the count, as Qt's `QT_GSTREAMER_PROPOSE_ALLOCATION_MIN_BUFFERS`
  does, e.g. to save CMA memory with 4K decoders on i.MX or Raspberry Pi.
- **`QGCQVideoSinkController`** — GUI-thread companion that binds the sink to a `QVideoSink`, lets it deliver only
  while the output's window is visible, and clears the frame that `QVideoSink` displays when decoding stops.

### `HwBuffers` — GPU paths

Per-platform import of decoded GPU frames without a copy. Most paths wrap their native textures in
`GstHwFrameTexturesBase : QVideoFrameTextures`; the DMABuf path hands its planes to Qt Multimedia's DMABuf importer
(`dmabuf/GstQtDmaBufImport`, the only code using that private Qt API):

- **Platforms**: `dmabuf` (Linux/VA-API), `gl` (Linux, Android), `vulkan`, `d3d` (D3D11/D3D12), `cuda`, `apple` (IOSurface), `android` (AHardwareBuffer).
- **`HwPathDescriptor`**: each backend describes its path once (memory match, buffer construction, cache reset,
  pool proposal, context bridge). `GstHwPathRegistry` lists the compiled ones in a constant array. The factory selects
  the first path whose descriptor matches a frame's memory, and the facade loops over the list for bus messages,
  context queries, restarts and resets.
- **`GstContextBridgeBase`**: GL, Vulkan, D3D11 and D3D12 bridges hand Qt's GPU device to GStreamer elements through
  `NEED_CONTEXT` and context queries. That is the main window's device: a VideoOutput popped out into its own window
  renders on that window's device, so on Direct3D11 its frames take the CPU path.
- **D3D11 decoder padding**: decoders pad their textures to their alignment (16 rows for H.264, 128 for HEVC/AV1)
  without crop meta. The picture is copied out on the render thread, as a texture-array slice is, so frames keep the
  picture's size and a CPU layout for that fallback and for screenshots.
- **`GstHwPathTelemetry`** tracks process-wide render-time counters (map failures, reuse hits, sync waits) for fallback
  diagnostics. A receiver's `GPU path live` and `GPU path stats` debug lines pair its own sink's `frame-stats` with
  them, both counted from when its session took the sink. Reading never resets the counters: each reader reports the
  change since its own snapshot, so receivers don't take each other's counts.
- **`CpuVideoFramePool`** recycles CPU-backed `QVideoFrame` storage (WebRTC-style slab) for the software-fallback path, avoiding per-frame allocation.
- **Build options**: `HwBuffers/CMakeLists.txt` probes one table row per path; the `QGC_GST_ENABLE_<PATH>` switches
  that compile a path in or out are listed in [`cmake/README.md`](../../../cmake/README.md#gstreamer). The CUDA,
  AHardwareBuffer and Vulkan paths are opt-in (`QGC_GST_ENABLE_<PATH>=ON`): no bin advertises CUDAMemory or
  VulkanImage caps, and Android's glupload bin offers only GLMemory. Configure lists compiled paths that the sink can't
  reach at runtime. Windows builds leave out the GLMemory path: Qt 6's OpenGL
  there is WGL, which gst-gl can't share. Linux builds with gst-gl and DMABuf compile both GPU bins; see
  [GPU bin selection](#gpu-bin-selection).

### Stream statistics

`VideoReceiver::stats` (`VideoStreamStats`, a QML value type that `VideoStreamController` forwards) tells how healthy a
stream is; the Fly View toolbar's video indicator shows it. While the stream is `Streaming`, `GstReceiverWorker` samples
its `PipelineSession` on each watchdog tick (once a second), and `VideoStreamStatsMeter` (`../VideoStreamStatsMeter.h`,
a pure class with an injectable clock) turns the cumulative counters into rates. The first sample is the rates'
baseline, so stats are valid from the second one on. The worker hands them to the receiver like its other reports, so
`statsChanged` is emitted on the GUI thread, and sends invalid stats when its session stops: on a stop, a reconnect or a
failure. The streaming threads only add to atomic counters, with no lock and no allocation per frame; everything else is
read on the worker.

| Field | Measured from |
| --- | --- |
| `fps` | Frames the display sink delivered to the `QVideoSink` (its `frame-stats` counts per path) less those it superseded, per second over the last `VideoStreamStatsMeter::kRateWindowIntervals` samples. |
| `bitrateKbps` | The compressed elementary stream reaching the tee, after the depayloader or demuxer and the parser, counted by the tee's buffer probe, over the same window as `fps`. |
| `framesDropped`, `droppedFps` | Since the stream started, and per second over the last interval: buffers the display queue leaked (one per `overrun`), delta frames its keyframe gate dropped, and superseded frames. |
| `packetsLost`, `lossPercent`, `jitterMs` | The `stats` of every `rtpjitterbuffer` in the source bin, `rtspsrc`'s internal ones included: `num-lost` since the stream started, `num-lost` against `num-pushed` over the last second, and `avg-jitter`. -1 without a jitter buffer: MPEG-TS, and UDP RTP in low-latency mode. |
| `latencyMs` | The minimum latency of a latency query on the pipeline, as logged when it reaches PLAYING. 0 in low-latency mode, whose display sink doesn't sync. |
| `decoder`, `hardwareDecoder` | The decoder factory decodebin3 plugged last, and `GStreamer::isHardwareDecoderFactory()`. |
| `gpuPath` | The delivery path most frames took since the previous sample, as `GstHw::toString()` names it: `CPU`, `DMABuf`, `GL`, `D3D11`, `D3D12`, `IOSurface`, `AHWBuf` or `Vulkan`. Empty for a display sink other than `qgcvideosinkbin`. |
| `codec` | The caps on the tee: `H.264`, `H.265`, `AV1`, `VP8`, `VP9`, `MJPEG`, `MPEG-2`, ... |
| `packetsLate`, `packetsDuplicate`, `rtxRequested`, `rtxRecovered` | The same jitter buffers' `num-late`, `num-duplicates`, `rtx-count` and `rtx-success-count`, since the stream started. -1 without a jitter buffer. |
| `keyframeIntervalMs` | The tee's buffer probe: the timestamps (PTS, else DTS) of the last two buffers without `DELTA_UNIT`. -1 until two keyframes arrived, so a stream without keyframes, such as one with intra refresh, never has one. |
| `gpuFallbacks` | The display sink's `gpu-fallbacks` count since the stream started, the sinks a decoder restart replaced included. |
| `decoderRestarts` | Decoders `DecodingBranch::restartDecoder()` replaced since the stream started. |
| `uptimeMs`, `reconnects` | The worker: the time since its session reported its first frame, and the sessions that streamed since the stream was started or restarted, less one. |
| `recordingMs`, `recordingBytes` | The worker, while recording: the time since the first part got its first frame, and the sizes of the parts' files. -1 otherwise. |

`VideoStreamStatsMeterTest` covers the rate math on a fake clock, `GstVideoReceiverStatsTest` the stats of real streams.

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
- **Plugin allowlist**: `gstreamer.plugins` (`common` plus the platform's key) decides what is registered and bundled. Android links the requested plugins that have a static archive into `libgstreamer_android.so`; iOS registers only the requested plugins found in its merged archive, so the archive's other plugins (some GPL) stay out of the app; desktop installs bundle the allowlisted plugins found in the SDK and check after install that those and the bundle-required ones (`GSTREAMER_BUNDLE_REQUIRED_PLUGINS` in `PluginPolicy.cmake`) arrived. Debug mobile builds add `videotestsrc` and `mpegtsmux` for MockLink's test stream, which `MockVideoStreamServer` encodes with `openh264enc` (H.265 only where a desktop GStreamer has `x265enc`).
- **GPL**: QGC ships LGPL GStreamer only. `GSTREAMER_GPL_DENYLIST` (`PluginPolicy.cmake`) names the GPL plugins and libraries of the official SDKs (`x264`, `x265`, `a52dec`, `dvdread`, ...); a desktop install that would ship one fails, and the macOS framework copy leaves them out. CI's [artifact report](../../../.github/ci-overview.md#artifact-report) also fails a package that ships one anywhere in it, or an Android or iOS binary with x264 or x265 linked in; the Linux AppImage only warns, a known issue: Ubuntu's `libavcodec`, which the bundled `libav` plugin loads, pulls in GPL `libx264` and `libx265`. A missing bundle-required plugin fails a mobile configure. Configure warns once about allowlist names the SDK or system does not ship as plugins (misspelled, not built for the platform, or not installed) and once about shipped ones a static build cannot link; missing hardware-specific or alternative decoders (`GSTREAMER_OPTIONAL_PLUGINS` in `PluginPolicy.cmake`, e.g. `qsv`, `va`, `libav`, `dav1d`) only appear in a status line, as do optional components (`RtspServer`, `Gl`, ...) that were not found.
- **License notice**: configure writes `LGPL-2.1.txt` and a `NOTICE.txt` naming the GStreamer version, its source code and, for an auto-downloaded SDK, the official packages and their SHA-256 (`Notice.cmake`). It also names the third-party libraries the shipped plugins bring (FFmpeg for `libav`, libvpx, dav1d, OpenH264 with a note on Cisco's patent license, and OpenSSL outside Linux) and, on iOS, which links GStreamer statically, how to relink the app with a modified GStreamer. They ship in `share/licenses/gstreamer/` of desktop installs (macOS: `QGroundControl.app/Contents/Resources/licenses/gstreamer/`) and under `licenses/gstreamer/` in the Android APK assets and the iOS app bundle. The dependency SBOM CI attaches to each package ([`generate_cpm_sbom.py`](../../../tools/ci/security/generate_cpm_sbom.py)) lists GStreamer from the same `QGC_GST_RESOLVED_*` cache entries.
- **CMake modules** ([`cmake/GStreamer/`](../../../cmake/GStreamer/)): `Orchestrator.cmake` is the entry point this directory's `CMakeLists.txt` includes; its header lists inputs and outputs. `platform/<platform>.cmake` discovers each platform's SDK, and `Acquire.cmake` downloads its packages (`Packages.cmake` names them and their checksums) and extracts them. `Targets.cmake` creates the targets the way Qt's `FindGStreamer.cmake` does, one `pkg_check_modules(IMPORTED_TARGET)` per library: `GStreamer::Core`, `GStreamer::<Component>` (`Base`, `Video`, `Gl`, `RtspServer`, ...) and `GStreamer::GStreamer`, which QGC links. The other modules are the helpers it includes, each described in its header. `tests/` holds script tests and a fake-SDK configure fixture, run with `ctest -L GStreamer`.

## Runtime Environment Setup

QGC configures GStreamer runtime paths before `gst_init()`:

- **Desktop (Linux/macOS/Windows)**: `GStreamer::prepareEnvironment()` sets plugin paths, scanner/helper paths, and GIO module paths when bundled runtimes are detected (never in a Linux build with `QGC_GST_BUNDLE_PLUGINS=OFF`). A bundled runtime also gets its own plugin registry, `gstreamer-<runtime version>-<arch>-<install hash>.bin` in the app's cache directory (`QStandardPaths::CacheLocation`), unless `GST_REGISTRY` or `GST_REGISTRY_1_0` is set, so it does not share the default per-user registry with the system GStreamer. The install hash keeps two installs of one GStreamer version (such as a stable and a daily build) from rescanning over each other's registry. It names what stays put across launches: the bundled plugin directory, the AppImage file (`$APPIMAGE`) rather than its mount point, which changes on every launch, and the app's name rather than the random directory macOS translocates a quarantined app to (such launches still rescan the plugins, whose recorded paths have moved). Each launch touches its registry and removes the `gstreamer-*.bin` registries there that no install has used for 30 days, such as those of an earlier GStreamer version or a replaced AppImage.
- **Linux AppImage**: the same Linux bundle detection applies to the AppImage's `usr/lib`; it is skipped when the user already set a `GST_PLUGIN_PATH*` or `GST_PLUGIN_SYSTEM_PATH*` variable. [`deploy/linux/AppRun`](../../../deploy/linux/AppRun) only sets the library, GIO and Qt environment.
- **Plugin search path**: The bundled plugin directory is set as `GST_PLUGIN_SYSTEM_PATH*`, which replaces GStreamer's default system and user plugin directories. `GST_PLUGIN_PATH*` is cleared, because it would only rescan the same directory.
- **Plugin scanner**: On desktop, GStreamer loads each new or changed plugin in its `gst-plugin-scanner` helper process, as it does by default, so a plugin that crashes while loading (a GPU driver probe, say) is blacklisted instead of taking QGC down on every launch. One scanner process loads every plugin of a scan, and GStreamer starts a new one only after such a crash. QGC points `GST_PLUGIN_SCANNER*` at the bundled scanner, and on Windows puts the app directory on `PATH` so the scanner finds the GStreamer DLLs. A missing, non-executable or failing scanner only logs a warning: GStreamer then loads the plugins in-process (the [boot self-test](#boot-self-test) fails on it). Android and iOS set `GST_REGISTRY_FORK=no`: their plugins are static, they ship no scanner, and iOS forbids `fork()`.
- **TLS certificates**: GStreamer's TLS users (RTSP over TLS, HTTPS sources) verify against GIO's default TLS database. On macOS and Windows, the bundled `gioopenssl` fills it from the system trust store (the Keychain's trust settings, the Windows `ROOT` and `CA` certificate stores), so roots an administrator or user added keep working; QGC leaves it alone and installs no CA bundle there. On Android and iOS, `gioopenssl` only knows OpenSSL's compiled-in certificate paths, which don't exist on the device, so the static plugin init makes the bundled `ca-certificates.crt` (`CA_CERTIFICATES`) the default database. On Linux, `libgiognutls` reads GnuTLS's system trust store.
- **Decoder ranks**: QGC's decoder rank changes (the video decoder setting, the zero-copy preferences) skip the features named in `GST_PLUGIN_FEATURE_RANK`, which keep the user's rank.
- **Validation**: A macOS app that ships `GStreamer.framework` without its plugin directory fails initialization with a clear error.
- **Plugin checks**: After `gst_init()`, `GStreamer::completeInit()` checks the elements each feature needs, as Qt's `qGstErrorMessageIfElementsNotAvailable` does. Only the display core is required: the `qgc` sink elements, `decodebin3`, `tee`, `queue`, `valve`, `capsfilter`, a video converter, and the RTP/UDP source with its H.264/H.265 depayloaders and parsers. Without it, video is disabled. A missing optional element logs one warning naming the feature it disables: RTSP (`rtspsrc`), MPEG-TS (`tsdemux`), TCP (`tcpclientsrc`), MKV recording (`matroskamux`) or MP4/MOV recording (`mp4mux`, `qtmux`).

## Platform Setup

### Linux

Use `python3 -m tools.setup.install_dependencies`, or manually:

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev python3-gi python3-gst-1.0
```

Debug builds also use `libgstrtspserver-1.0-dev`, when present, for MockLink's RTSP test stream and the RTSP receiver tests.

Configure finds GStreamer with pkg-config (`PKG_CONFIG_PATH`, or the toolchain's sysroot setup in cross builds) and takes its layout from `gstreamer-1.0.pc`: the plugin directory from `pluginsdir`, the plugin scanner's from `pluginscannerdir`, and the GIO modules' from `gio-2.0.pc`'s `giomoduledir`.

Unless `QGC_GST_BUNDLE_PLUGINS` is `OFF`, installs and AppImages bundle the system's allowlisted plugins, the plugin scanner and only the GIO modules for TLS and proxies (`libgiognutls`, `libgiolibproxy`, `libdconfsettings`). libproxy reads GNOME's and KDE's proxy settings itself; glib-networking's GNOME resolver (`libgiognomeproxy`) stays out because it aborts QGC wherever the `org.gnome.system.proxy` GSettings schema isn't installed. The VFS modules stay out: the AppImage, and a bundle on GLib 2.76+, run GIO with `GIO_USE_VFS=local`.

`udpsrc` before gst-plugins-good 1.24.10 (Ubuntu 24.04 ships 1.24.2) corrupts the heap when a UDP stream stops right after it starts: its `unlock()` cancels a `GCancellable` that `unlock_stop()` frees on the streaming thread. `GStreamer::completeInit()` serializes the two for those versions, as the upstream fix ([gstreamer!7897](https://gitlab.freedesktop.org/gstreamer/gstreamer/-/merge_requests/7897)) does.

#### GPU bin selection

With `gpu-zerocopy`, `qgcvideosinkbin` picks one of four variants when it is built:

- `glupload` (the default on Linux and Android): the converter feeds glupload → glcolorconvert → GLMemory. On EGL,
  gst-gl imports LINEAR and vendor-tiled DMABuf, at the cost of its own copy/convert; on GLX it uploads system memory.
- `dmabuf`: DMABuf and DMA_DRM go straight to Qt's DMABuf importer, skipping gst-gl. It takes LINEAR and the plain
  tilings Qt's EGL display reports (see [DMABuf zero-copy diagnostics](#dmabuf-zero-copy-diagnostics)).
- `d3d12convert`: the converter feeds d3d12upload → d3d12convert → RGBA/BGRA D3D12Memory, for Qt Quick on Direct3D12
  (`QSG_RHI_BACKEND=d3d12`), whatever `QGC_GST_GPU_BIN` says. It falls back to passthrough if the d3d12 plugin lacks
  those elements.
- passthrough: the converter feeds the sink, which takes the GPU memory the renderer imports (see
  [`gstqgc`](#gstqgc--custom-gstreamer-plugin)). Windows on Direct3D11 and Apple builds use it.

`glupload` hands GLMemory to Qt through the GL share group, so it needs Qt Quick on OpenGL, over EGL or GLX. That
holds for any OpenGL on Linux and Android, so the bin doesn't wait to learn which: it may be built before the scene
graph runs, and Android can't tell EGL until then. `dmabuf` hands EGLImages to Qt's EGL display, so it needs OpenGL
over EGL. Under any other renderer, such as Vulkan, the bin falls back to passthrough, which on Linux means system
memory: gst-gl's GLMemory would otherwise be downloaded and uploaded again for every frame.

When both are compiled, `QGC_GST_GPU_BIN` (`glupload` or `dmabuf`, case-insensitive, read once per process) picks
between them, and an explicit value is used even under a renderer the variant doesn't suit, with a warning. An unknown
value logs a warning and keeps the default. Builds with only one variant ignore the variable: Android always uses
glupload, Linux without gst-gl always imports DMABuf directly. The choice is logged once under
`Video.GStreamer.GstQgcGpuBin`; the `qgc` GStreamer debug category logs each bin's variant.

Qt's xcb platform, which the AppImage uses even in Wayland sessions, picks GLX before EGL. Linux builds with the GL or
DMABuf path therefore set `QT_XCB_GL_INTEGRATION=xcb_egl` at startup unless it is already set (`HwBuffers::configureQtGraphics()`, which the platform startup code calls through `VideoBackend` before Qt starts); Qt still falls back to
GLX when EGL fails to initialize. Set `QT_XCB_GL_INTEGRATION=xcb_glx` to keep GLX, e.g. for a driver whose EGL
misbehaves; video then reaches Qt as GLMemory that glupload uploads, without DMABuf import.

#### DMABuf zero-copy diagnostics

The direct DMABuf path hands each plane to Qt Multimedia's DMABuf importer as its own EGLImage, with the DMA_DRM
modifier when the caps carry one. QGC adds what Qt's importer lacks: it waits for the decoder's writes first (on the
GPU through the dma-buf's exported fence, else with a bounded `poll()`), and it keeps the buffer alive as long as the
textures.

- It needs Qt to render with EGL: Wayland, eglfs, or xcb with `QT_XCB_GL_INTEGRATION=xcb_egl`. Under GLX, Qt's xcb
  default, the sink offers no DMABuf caps. Nor does it until Qt's EGLDisplay is known (`QGCRhiCapture::qtEglDisplay`:
  Qt's global share context, its platform integration, or the scene graph's OpenGL context); once it is, the sink asks
  upstream to renegotiate.
- The DMA_DRM caps offer the (format, modifier) pairs Qt's EGL display reports when every plane can be sampled with
  that modifier. Plain vendor tilings (Intel X/Y/4, AMD without DCC) qualify; compressed layouts and those with
  auxiliary planes (Intel CCS, AMD DCC, Arm AFBC, Qualcomm UBWC) don't. Buffers negotiated with plain
  `memory:DMABuf` caps import without a modifier, leaving the layout to the driver.
- `EGL_BAD_MATCH` (a broken EGL/driver pairing, QTBUG-112312), or 30 failed imports in a row, turn the path off until
  the scene graph is recreated: the sink stops offering DMABuf and DMA_DRM caps and asks upstream to renegotiate.
  Until upstream has, LINEAR frames take the CPU path.
- A tiled frame (a DMA_DRM modifier other than LINEAR) has no CPU layout: when no import takes it, it is dropped rather
  than shown scrambled, and counted in the sink's `tiled-drops` frame stat.
- The `Video.GStreamer.HwBuffers.GstDmaBuf` and `Video.GStreamer.HwBuffers.GstDmaDrmCaps` debug categories log the
  offered modifiers and the first import; Qt's `qt.multimedia.dmabuf` category logs its EGL calls.

`GstHwBuffersGlReadbackTest` reads imported DMABuf planes back from a GL QRhi. CTest runs it on xcb with `xcb_egl` and
skips it without an X display; a plain memfd stands in for a dma-buf under Mesa's software rasterizer, and a VA-API
decoder's tiled output is compared with its system-memory output on real hardware.

### macOS / Windows / iOS

GStreamer SDKs are auto-downloaded during CMake configure. To use a local installation instead, set `GStreamer_ROOT_DIR` (on iOS, a `GStreamer.xcframework` or `GStreamer.framework`, or a directory containing one). Configure fails when that SDK is older than the requested version (`gstreamer.version.<platform>` or `QGC_GST_VERSION`) or, on Windows, incomplete; an SDK Windows finds through `GSTREAMER_1_0_ROOT_*` or a standard install location is replaced by a download instead.

On macOS, configure uses, in this order: `GStreamer_ROOT_DIR`; Homebrew's GStreamer when `QGC_GST_USE_HOMEBREW` is `ON`; the SDK an earlier configure downloaded; `/Library/Frameworks/GStreamer.framework`; a new download. Homebrew's GStreamer (`brew install gstreamer pkgconf`) is also used when `GStreamer_ROOT_DIR` is a Homebrew prefix (`$(brew --prefix)`) or one of its `opt/gstreamer` or `Cellar/gstreamer/<version>` directories. Its pkg-config modules, and GLib's, are read with Homebrew's own `pkg-config`, which also finds the macOS system libraries' modules; `gstreamer.version.macos` (or `QGC_GST_VERSION`) is its minimum version. Such a build only runs from the build tree: Homebrew's libraries and plugins load each other by absolute paths under its prefix, so `cmake --install` and packaging fail with an error instead of bundling them.

Windows and macOS installs bundle the allowlisted plugins, the GIO modules and the helpers, and only the SDK libraries that those and QGC itself load, which `install(RUNTIME_DEPENDENCY_SET)` resolves at install time (libproxy's backends, which are loaded with `dlopen`, are installed as files too). A macOS `GStreamer.framework` is copied whole, without its GPL libraries.

On iOS only the `GStreamerMobile` framework links the SDK's merged static archive; `GStreamer::GStreamer` carries just its headers, defines and system-framework links. The app takes every GStreamer function from the framework, which exports those its registered plugins use plus `QGC_GST_APP_CORE_SYMBOLS` (`Mobile.cmake`). A new core call that no plugin uses therefore fails the app's link with an undefined-symbol error naming it: add the symbol to that list. A post-build check (`SingleCoreCheck.cmake`) fails if the app still ends up defining core symbols. The plugin descriptors the configure reads from the merged archive (over 1 GB per `GStreamer.xcframework` slice) with `nm` are cached in the build directory, keyed on the archive's path, size and modification time. From 1.28.8 on, the iOS download is a tarball holding a `GStreamer.xcframework` instead of a `.pkg`, and only the xcframework's iOS slices are extracted, skipping its ~2 GB of tvOS slices.

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

CI caches the extracted trees under a key covering the `gstreamer` section of `.github/build-config.json` and `cmake/GStreamer/{Packages,Acquire}.cmake`, so keep everything that decides an extracted tree in those two files. Cached archives are verified on every hit against the checksum pinned in `.github/build-config.json`, or the upstream `.sha256sum` sidecar for an unpinned version; an auto-downloaded SDK is fetched again when its `gstreamer.version.<platform>` changes, and cached GStreamer probe results are recomputed whenever the SDK changes. Each archive is extracted once into a directory beside it that is stamped with the archive hashes, and a tree stamped with the pinned checksums is used without downloading or even keeping its archives (CI caches only the trees); configures sharing the cache take a lock instead of downloading or extracting over each other. The Windows SDK installer runs in its portable mode, so it leaves no `GSTREAMER_1_0_ROOT_*` environment variable, registry key or uninstall entry behind.

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

- `rtsp://`, `rtspt://`: RTSP (the video stream only), over UDP and, when no UDP data arrives within 5 s, over TCP.
- `udp://`: H.264 over RTP/UDP; `udp265://`: H.265 over RTP/UDP. Port 0 listens on a free port.
- `mpegts://`: MPEG-TS over UDP; `tcp://`: MPEG-TS over TCP.

### Automated tests

| Test | Label | Covers |
| --- | --- | --- |
| `GStreamerEnvironmentTest` | Unit | The environment `prepareEnvironment()` sets for a fake bundled desktop install (plugin path and scanner, which still forks and is reused) and for the running app, and the bundled runtime's private plugin registry: named per install, also across AppImage mounts and macOS translocation, pruned of registries unused for 30 days, and replaced by a user-set `GST_REGISTRY` or `GST_REGISTRY_1_0`; a system GStreamer keeps the default registry. |
| `GStreamerInitTest` | Unit | Init policy: how GLib's and GStreamer's logs reach Qt logging, which missing elements disable video and which only a feature, and the decoder ranks the decoder setting applies, except those set with `GST_PLUGIN_FEATURE_RANK`; the boot self-test's verdict on a failing plugin scanner and on a bundle's blacklisted, foreign, stale or unloaded plugins. |
| `GStreamerTest` | Unit | The `GStreamer` facade: URI checks, decoder classification, init, and the recording sink's accepted streams. |
| `GstQgcSinkTest` | Unit | `qgcqvideosink` and `qgcvideosinkbin` as elements: construction, caps, allocation, CPU zero-copy frames, and frames pushed straight into the sink (latest frame only, with the superseded ones counted, flushes, orientation tags, crop, renegotiation). |
| `GstQgcSinkControllerTest` | Unit | `QGCQVideoSinkController` binding `qgcqvideosink` to a `QVideoSink`: inactive without a window, and unbound when its owner or the `QVideoSink` goes away or the sink is released. |
| `GstQgcVideoFormatsTest` | Unit | The sink's format tables: GStreamer formats, colorimetry and orientations mapped to Qt's, and the formats the sink advertises. |
| `GstSourceFactoryTest` | Unit | The source bin built for each URI, its jitter-buffer and retransmission policy, RTSP stream selection and keyframe requests from RTSP depayloaders, dynamic-pad linking and the parameter-set gate, without a stream. |
| `GstHwBuffersTest` | Unit | The `HwBuffers` paths without a GPU: path selection, import caches, frame-texture bundles on the Null `QRhi`, DMABuf modifier policy, CPU fallbacks, crop matrices and telemetry. |
| `GstQgcSinkHarnessTest` | Unit | `qgcvideosinkbin` under GstHarness, bound to a `QVideoSink`: resolution and format changes renegotiated through the converter, and orientation tags crossing it. |
| `GstSourceFactoryHarnessTest` | Unit | The parameter-set gate on a real H.264/H.265 depayloader under GstHarness, fed RTP payloaded from the test streams as a receiver joining mid-GOP sees it. |
| `GstVideoReceiverComponentsTest` | Unit | The receiver's components without a stream: reconnect backoff, stall detection, recording finalization by ancestry, pipeline and `.dot` file names, and the size `VideoReceiver::presentedSize()` reports for a padded, cropped or rotated frame. |
| `GstQgcSinkPipelineTest` | Integration | `videotestsrc` pipelines played into `qgcvideosinkbin` bound to a `QVideoSink`: formats, timestamps, colorimetry, non-square pixels, software frames through the GPU bins that need no GPU for them, teardown under load. |
| `GstSourceFactoryStreamTest` | Integration | Source bins receiving real streams on loopback: RTP over UDP joining mid-GOP, and an RTSP session that sets up only the video stream; and their `udpsrc` stopped right after it starts, thousands of times. |
| `GstRecordingSinkTest` | Integration | The recording file sink fed by running pipelines: a mid-stream H.265 MP4 start and frames that carry only a DTS. |
| `GstSelfTestPipelineTest` | Integration | The [boot self-test](#boot-self-test) against the GStreamer the tests run on, and its software retry after a hardware decoder fails mid-stream, which restores the decoder ranks. |
| `GstHwBuffersPipelineTest` | Integration, GPU | Pipelines that upload into or decode to GPU memory (DMABuf, GLMemory, Vulkan, D3D11/D3D12, IOSurface) and the path each frame takes; D3D11/D3D12 and IOSurface frames are also converted with `QVideoFrame::toImage()` on a QRhi of that API and compared with the CPU frame; the D3D QRhi uses the WARP software adapter when `QSG_RHI_PREFER_SOFTWARE_RENDERER` is set or DXGI lists no hardware adapter. Software frames through the glupload bin and the Windows D3D11 sink. Skips without the device. |
| `GstHwBuffersGlReadbackTest` | Integration, GPU | DMABuf planes and glupload's GLMemory frames imported into a GL QRhi that shares Qt's EGL context, read back pixel by pixel and converted with `QVideoFrame::toImage()` (xcb with `xcb_egl`). |
| `GstVideoReceiverPipelineTest` | Integration | `GstVideoReceiver` end to end against the H.264 test stream it sends itself on loopback: states, display, restarts, config changes, a mid-stream resolution change, H.265, MockLink's streams, screenshots. Receivers and servers in every `GstVideoReceiver*Test` listen on port 0, so parallel runs never collide. |
| `GstVideoReceiverReconnectTest` | Integration | Permanent failures, reconnect backoff after a failed start, the restart policy on stream loss, and an RTSP server that never answers. |
| `GstVideoReceiverRecordingTest` | Integration | Recordings that stay playable: decoder-only recovery, across reconnects, slow and hung storage, recorder errors, stream end, deactivation and bounded teardown. |
| `GstVideoReceiverStatsTest` | Integration | The stream statistics of H.264 and H.265 RTP with and without a jitter buffer. |

The classes derived from `GStreamerTestBase` fail in `initTestCase()` when `GStreamer::completeInit()` fails, as it does when an element video needs is missing, and their tests skip where GStreamer can't be initialized at all (as under AddressSanitizer). A test skips, rather than fails, when an optional element it uses is missing (`GSTTEST_SKIP_UNLESS_FACTORIES` in `GStreamerTestHelpers.h`). The two `*HarnessTest` classes use GstHarness from gst-check (`gstreamer-check-1.0`), which test builds request as an optional GStreamer component (`Check`); they compile only where the SDK or distribution ships it. No test needs a software encoder: the streams they send are the short 320x240 H.264 and H.265 files, and a 160x120 H.264 one a stream switches to mid-way, in [`Runtime/Media/`](Runtime/Media/), which the app's self-test embeds too (its `generate.sh` regenerates them), split into access units and paced through `appsrc`. Only `GstVideoReceiverPipelineTest::_testDecodesMockLinkStreams` encodes in software, as MockLink's stream server does (`openh264enc`, falling back to `x264enc`; `x265enc` for H.265); the VA-API cases encode with the host's `vah264enc`. Each class runs in its own process, so a crash in a GPU path only fails its own class; `-LE GPU` leaves out the classes that use the host's GPU.

```bash
ctest --test-dir build -R '^(GStreamer|Gst)[A-Za-z]*Test$' --output-on-failure
```

CI runs these classes in the Linux coverage and sanitizer jobs and the macOS Debug leg with the rest of the suite, and on
Windows in `windows.yml`'s `gstreamer-tests` job, a Debug build that runs only the GStreamer and video classes against
the SDK's shipped plugins plus the sources and encoders the tests send streams with, with the D3D tests on WARP. The
packages are checked beyond their boot test: every DLL the Windows install imports must ship or come with Windows
(`pe_dependency_audit.py`), every library a Mach-O file in the macOS bundle loads must resolve inside the bundle or to
macOS (`macho_bundle_audit.py`), and the AppImage must boot in clean Ubuntu 24.04 and Debian 13 containers without
GStreamer or Qt (`appimage_container_boot.py`); see [`.github/ci-overview.md`](../../../.github/ci-overview.md).

### Boot self-test

`QGroundControl --simple-boot-test`, which CI runs on every package (the installed Windows app, the macOS DMG, the
Linux build tree and AppImage, the Android emulator and the iOS simulator), initializes video and then runs
`GStreamer::selfTest()` (`GStreamerSelfTest.cc`, through `VideoBackend::selfTest()`). The boot test fails when any of
these checks fails, each logged as a critical message under `Video.GStreamer.SelfTest` that names the plugin, element
or file:

- **Bundled plugins** (a desktop runtime bundled with the app, as `GStreamerEnvironment` set it up; not a build tree
  using the system GStreamer, nor a `QGC_GST_BUNDLE_PLUGINS=OFF` build): no plugin in the bundle's plugin directory is
  blacklisted, as one whose library dependency is missing is; every plugin GStreamer registered from a file comes from
  that directory; and the registry and the directory hold the same plugin files.
- **Plugin scanner** (bundled desktop runtimes): GStreamer did not log `External plugin loader failed` during init,
  i.e. the bundled `gst-plugin-scanner` ran.
- **Features**: every feature in `GStreamer.cc`'s plugin table has its elements. Unlike initialization, which needs
  only the display core, the self-test requires them all, since `gstreamer.plugins.common` and
  `GSTREAMER_BUNDLE_REQUIRED_PLUGINS` ship them on every platform.
- **GIO** (bundled runtimes, Android and iOS): GIO's default TLS backend supports TLS and has a certificate database;
  where the bundle ships the libproxy GIO module, libproxy looks up the proxy for `rtsp://127.0.0.1`, which loads the
  backend libproxy opens by name.
- **Decoding**: the 160x120 H.264 sample from [`Runtime/Media/`](Runtime/Media/),
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

- In-app: The GStreamer debug level setting (Application settings) sets GStreamer's default threshold, and QGC forwards GStreamer's log to the `Video.GStreamer.GStreamerAPI` category; enable that category's debug output in QGC's logging settings to see levels below Info. Release builds forward up to the Debug level.
- Native output: Setting `GST_DEBUG`, `GST_DEBUG_FILE` or `GST_TRACERS` keeps GStreamer's own log handler and turns the forwarding off. The log then goes to stderr, or to `GST_DEBUG_FILE`, in GStreamer's format, which its tools (`gst-stats`, tracer plotters) parse, in every build type. `GST_DEBUG` replaces the in-app level, and `GST_DEBUG_NO_COLOR=1` drops the color codes. See <https://gstreamer.freedesktop.org/documentation/gstreamer/running.html>.
- Command line options: `--gst-debug-level`, `--gst-debug`, `--gst-debug-help`, etc.
- GLib messages (`g_warning()`, `g_critical()`, ...) always go to the `Video.GStreamer.GStreamerLogging` category.
- The [boot self-test](#boot-self-test) logs its failures under `Video.GStreamer.SelfTest`, and its summary at info level.
- Receiver flow (starts, stops, reconnects, the decoder chosen, pipeline latency, GPU path statistics): enable the `Video.GStreamer.GstVideoReceiver` category. The stream's health is also measured continuously: see [Stream statistics](#stream-statistics). Failures are logged as warnings even with it off.

## Pipeline Observability

QGC supports the standard GStreamer observability env vars. Set them before launching QGC. `GST_DEBUG`, `GST_DEBUG_FILE` and `GST_TRACERS` switch to GStreamer's native log output (see [GStreamer Logging](#gstreamer-logging)), so tracer records keep the format the GStreamer tools expect.

### Pipeline graph dumps (`.dot` files)

When `GST_DEBUG_DUMP_DOT_DIR` is set, QGC writes each receiver's pipeline graph there at key transitions as `receiver-<stream>-<tag>.dot`. `<stream>` is the receiver's name (`videoContent`, `thermalVideo`), so the two receivers don't overwrite each other's files, and `<tag>` names the transition (`pipeline-initial`, `pipeline-started`, `pipeline-with-decoder`, `pipeline-with-videosink`, `pipeline-with-filesink`, `pipeline-error`, `pipeline-watchdog-timeout`, `pipeline-decoder-timeout`, `pipeline-recording-stopped`, …). A transition's next dump overwrites its last one.

```bash
export GST_DEBUG_DUMP_DOT_DIR=/tmp/qgc-pipeline-dots
./QGroundControl
dot -Tpng /tmp/qgc-pipeline-dots/receiver-videoContent-pipeline-started.dot -o pipeline.png
```

When the env var is **unset**, QGC instead writes a snapshot to `<CacheLocation>/qgc-pipeline-dot/receiver-<stream>-<tag>-<UTC timestamp>.dot` on a pipeline `ERROR` and on a source or decoder watchdog timeout, keeping the 10 newest, so field-bug-report bundles include the topology automatically. `GstVideoReceiver::dumpPipelineGraph(tag)` (`Q_INVOKABLE`, so callable from QML) writes the same snapshot on demand for use from a debug menu.

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
