# GStreamer Video Streaming

QGroundControl uses GStreamer for UDP RTP and RTSP video streaming in the Main Flight Display.

This guide covers the GStreamer receiver architecture, platform setup, and diagnostics. See
[CODING_STYLE.md](../../../../CODING_STYLE.md) for repository-wide source conventions,
[test/README.md](../../../../test/README.md) for the test framework, and
[tools/README.md](../../../../tools/README.md) for general build and tooling commands.

## Source Code Architecture

The pipeline is split into focused components (all in this directory):

| Component | Role |
| --- | --- |
| `GstVideoReceiver` | The `VideoReceiver` the app uses: runs the pipeline (source → tee → the branches below) and its only bus consumer on its own worker thread, and is the only component that reconnects (with backoff) after a stream failure. The components below are plain classes it owns; each header states which thread calls what. |
| `DecodingBranch` | Display branch behind the tee: inserts `decodebin3` once the source streams and reports decoding, the decoder name and the video size. |
| `RecordingBranch` | Recording branch behind the tee: one `splitmuxsink` file per recording, started at a keyframe and, for MP4/MOV, muxed so a crash leaves a playable file; `RecordingFinalizer` tells when the file is closed. |
| `StreamWatchdog` | Notices stalled source or decoder output on a monotonic clock, and paces reconnects (1 s, 2 s, 4 s, … 30 s). |
| `PipelineDotDumper` | Writes the pipeline's `.dot` graphs; see [Pipeline Observability](#pipeline-observability). |
| `GstSourceFactory` | Constructs the source element from the stream URI and applies RTP jitter-buffer policy for `application/x-rtp` sources. |
| `GStreamerEnvironment` | Process-wide environment setup (plugin/scanner/helper/GIO paths, env hygiene) run before `gst_init()`. See [Runtime Environment Setup](#runtime-environment-setup). |
| `GStreamerHelpers` | Small utilities: decoder classification and rank overrides, tee branch heads, and taking a child element out of a running bin. |
| `GStreamerLogging` | Routes GStreamer's log output through QGC's logging and applies the persisted debug level. |
| `GstScoped` | RAII owners for transfer-full GStreamer returns so refs can't leak on early return. |

### `gstqgc` — custom GStreamer plugin

A QGC-owned plugin (`gstqgc/`) bridging the pipeline into Qt:

- **`qgcqvideosink`** — a `GstVideoSink` that pushes decoded frames into a Qt `QVideoSink`. Delivery keeps only the
  latest frame, so a stalled GUI thread never holds more than one undelivered frame (or the decoder buffer behind it).
  Its construct-only `gpu-zerocopy` property (passed down by the bin) enables zero-copy import of GPU memory, and its
  `frame-stats` property reports that sink's own frame counts per delivery path (`GstQgc::formatSinkStats` formats
  them); GstBaseSink's standard `stats` property is left as is.
- **`qgcvideosinkbin`** — a bin wrapping a format-restriction capsfilter plus `qgcqvideosink`.
- **`QGCQVideoSinkController`** — GUI-thread companion that mirrors negotiation/telemetry from the sink into QGC, and
  clears the frame its `QVideoSink` displays when the sink leaves the pipeline.

### `HwBuffers` — GPU paths

Per-platform import of decoded GPU frames without a copy, all sharing `GstHwFrameTexturesBase : QVideoFrameTextures`:

- **Platforms**: `dmabuf` (Linux/VA-API), `gl`, `vulkan`, `d3d` (D3D11/D3D12), `cuda`, `apple` (IOSurface), `android` (AHardwareBuffer).
- **`HwPathDescriptor`**: each backend describes its path once (memory match, buffer construction, cache reset,
  pool proposal, context bridge). `GstHwPathRegistry` lists the compiled ones in a constant array. The factory selects
  the first path whose descriptor matches a frame's memory, and the facade loops over the list for bus messages,
  context queries, restarts and resets.
- **`GstContextBridgeBase`**: GL, Vulkan, D3D11 and D3D12 bridges hand Qt's GPU device to GStreamer elements through
  `NEED_CONTEXT` and context queries.
- **`GstHwPathTelemetry`** tracks process-wide render-time counters (map failures, reuse hits, sync waits) for fallback
  diagnostics. A receiver's `GPU path stats` debug line pairs its own sink's `frame-stats` with them.
- **`CpuVideoFramePool`** recycles CPU-backed `QVideoFrame` storage (WebRTC-style slab) for the software-fallback path, avoiding per-frame allocation.
- **Build options**: `HwBuffers/CMakeLists.txt` probes one table row per path; the `QGC_GST_ENABLE_<PATH>` switches
  that compile a path out are listed in [`cmake/README.md`](../../../../cmake/README.md#gstreamer). Configure lists
  compiled paths that the sink can't reach at runtime: the glupload bin (Linux and Android with gst-gl) offers only
  GLMemory caps, and no bin advertises CUDAMemory or VulkanImage caps. On Linux, `QGC_GST_ENABLE_GLMEMORY=OFF` switches
  the bin to direct DMABuf import.

## Build Configuration

- **Enable/disable**: Set the `QGC_ENABLE_GST_VIDEOSTREAMING` CMake option to `ON`/`OFF`.
- **Versions, checksums and plugins**: `gstreamer.version.<platform>`, `gstreamer.checksums`, `gstreamer.plugins` and the iOS CA-bundle pin live in [`.github/build-config.json`](../../../../.github/build-config.json), read through `qgc_config_read` ([`cmake/modules/BuildConfigReader.cmake`](../../../../cmake/modules/BuildConfigReader.cmake)). The download URLs (upstream and the QGC S3 mirror) are templated from the version in [`cmake/GStreamer/Download.cmake`](../../../../cmake/GStreamer/Download.cmake). Linux uses the system GStreamer and treats `gstreamer.version.minimum` as its floor.
- **Configure options**:
  - `GStreamer_ROOT_DIR`: the SDK to use. Configure never writes it; the SDK it selected is recorded as `QGC_GST_RESOLVED_ROOT` in `CMakeCache.txt`.
  - `QGC_GST_VERSION`: overrides the build-config version (the minimum on Linux, the SDK to download elsewhere).
  - `GStreamer_USE_STATIC_LIBS` / `GStreamer_USE_FRAMEWORK`: layout preferences with per-platform defaults; discovery overrides them for the SDK it finds.
  - `GStreamer_REQUIRE_CHECKSUM` and `QGC_GST_DOWNLOAD_TIMEOUT` (the SDK download's wall-clock cap, default 1200 s; its other limits are the shared `QGC_DOWNLOAD_*` ones) control downloads; `GStreamer_DEBUG` traces target creation.
- **Plugin allowlist**: `gstreamer.plugins` (`common` plus the platform's key) decides what is registered and bundled. Android links the requested plugins that have a static archive; iOS registers only the requested plugins found in its merged archive, so the archive's other plugins (some GPL) stay out of the app; desktop static SDKs register the requested plugins that have a pkg-config module; desktop installs bundle the allowlisted plugins found in the SDK and check after install that those and the runtime-required ones arrived. Debug mobile builds add the MockLink test-stream encoders. A missing runtime-required plugin fails a mobile configure; other missing plugins are warnings.
- **CMake modules** ([`cmake/GStreamer/`](../../../../cmake/GStreamer/)):
  - `Orchestrator.cmake`: the entry point this directory's `CMakeLists.txt` includes. It picks the version, calls the platform's discovery function (`Platform.cmake` names the platform), creates the targets, checks the plugins and records the resolved SDK; its header lists the variables it sets.
  - `platform/*.cmake`: per-platform discovery (user, system or downloaded SDK and its pkg-config setup), the Android `GStreamer::mobile` target and the iOS merged-archive targets.
  - `Download.cmake`: URL templates, checksums (in-tree pins, else the upstream `.sha256sum` sidecar) and `qgc_gst_acquire_sdk`, which downloads into the shared cache under a lock and extracts once into a stamped directory.
  - `Targets.cmake`: `GStreamer::GStreamer`, `::deps`, `::api_*` and, for static SDKs, per-plugin targets from pkg-config. `PkgConfig.cmake` points pkg-config at the SDK for this directory's subtree only.
  - `PluginPolicy.cmake`: alternate plugins, runtime-required plugins, the iOS skip list and the MockLink encoders.
  - `StaticInit.cmake`: generates `gst_init_static_plugins()` for static layouts from `gst_static_plugins.c.in`.
  - `Install.cmake`: desktop install helpers; each reports the plugin directory it installs to, which the install-time verifier checks.
  - `SdkState.cmake`: the resolved-SDK cache entries and the SDK fingerprint that drops cached probe results when the SDK changes.
  - `Platform.cmake`, `Components.cmake`, `Layout.cmake`, `Link.cmake` and `Probe.cmake`: the target platform, the component registry, the `GStreamer::Layout` target describing the SDK's on-disk layout, library resolution for the targets and the header probes.
  - `Helpers.cmake`: include aggregator for the modules above.
  - `tests/`: script tests and a fake-SDK configure fixture, run with `ctest -L GStreamer` (or `-L CMake`, with the shared-module tests).

## Runtime Environment Setup

QGC configures GStreamer runtime paths before `gst_init()`:

- **Desktop (Linux/macOS/Windows)**: `GStreamer::prepareEnvironment()` sets plugin paths, scanner/helper paths, and GIO module paths when bundled runtimes are detected.
- **Linux AppImage**: [`deploy/linux/AppRun`](../../../../deploy/linux/AppRun) exports `GST_PLUGIN_*`, `GST_PLUGIN_SCANNER*`, and `GST_PTP_HELPER*` only when bundled paths are valid.
- **Environment hygiene**: Python virtualenv/conda variables are cleared for scanner stability (`PYTHONHOME`, `PYTHONPATH`, `VIRTUAL_ENV`, `CONDA_*`), and `PYTHONNOUSERSITE=1` is set.
- **Plugin search path**: The bundled plugin directory is set as `GST_PLUGIN_SYSTEM_PATH*`, which replaces GStreamer's default system and user plugin directories. `GST_PLUGIN_PATH*` is cleared, because it would only rescan the same directory.
- **Validation**: A macOS app that ships `GStreamer.framework` without its plugin directory fails initialization with a clear error. A missing or non-executable bundled `gst-plugin-scanner` only logs a warning: QGC sets `GST_REGISTRY_FORK=no`, so GStreamer scans plugins in-process.

## Platform Setup

### Linux

Use `python3 -m tools.setup.install_dependencies`, or manually:

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev python3-gi python3-gst-1.0
```

#### DMABuf zero-copy diagnostics

Linux DMABuf zero-copy normally tries the single-EGLImage importer for shared-fd
multi-plane formats such as NV12/P010, matching the common VA-API exporter
layout. To force CPU fallback or per-plane import during driver triage, disable
that importer before launching QGC:

```bash
export QGC_GST_DMABUF_SINGLE_EGLIMAGE=0
```

Unset the variable, or set it to `1`, to restore the default behavior.

QGC also switches to per-plane import on its own when the single-EGLImage
importer fails with `EGL_BAD_MATCH`. It disables DMABuf zero-copy for the
session only when per-plane import fails the same way.

`QGC_GST_DMABUF_CACHE=1` keeps each pooled buffer's EGLImages across frames
instead of importing them every frame. It is off by default because no test or
CI host exercises the direct DMABuf importer.

### macOS / Windows / iOS

GStreamer SDKs are auto-downloaded during CMake configure. To use a local installation instead, set `GStreamer_ROOT_DIR` (on iOS, a `GStreamer.xcframework` or `GStreamer.framework`, or a directory containing one).

Configure never overwrites `GStreamer_ROOT_DIR`: the SDK it selected is recorded as `QGC_GST_RESOLVED_ROOT` in `CMakeCache.txt`, an auto-downloaded SDK is fetched again when its `gstreamer.version.<platform>` changes, and cached GStreamer probe results are recomputed whenever the SDK changes.

### Android

Auto-downloaded during CMake configure. No manual setup required. Each ABI build extracts only its own tree from the universal SDK tarball.

The plugins' Java sources and GStreamer's fonts and CA bundle are staged in the build directory (`extra_java_sources/`, `extra_assets/`), which [`android/build.gradle`](../../../../android/build.gradle) adds to the APK. Earlier builds copied them into `android/`; configure deletes those copies, which would otherwise duplicate the staged files.

> **Windows users building for Android**: Enable Developer Mode (Settings > System > For developers) to support symbolic links during the build.

### Caching downloaded SDKs

Auto-downloaded SDK archives are cached to `${CPM_SOURCE_CACHE}/gstreamer-*`. `CPM_SOURCE_CACHE` defaults to `.cache/CPM` in the source tree; set it to a location shared by your checkouts (e.g. `~/.cache/CPM`) to download the 200-700 MB archives only once:

```bash
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
```

Cached archives are verified on every hit against the checksum pinned in `.github/build-config.json`, or the upstream `.sha256sum` sidecar for an unpinned version. Each archive is extracted once into a directory beside it that is stamped with the version and archive hash; configures sharing the cache take a lock instead of downloading or extracting over each other. The Windows SDK installer runs in its portable mode, so it leaves no `GSTREAMER_1_0_ROOT_*` environment variable, registry key or uninstall entry behind.

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

- `rtsp://`, `rtspt://`: RTSP (the video stream only).
- `udp://`: H.264 over RTP/UDP; `udp265://`: H.265 over RTP/UDP.
- `mpegts://`: MPEG-TS over UDP; `tcp://`: MPEG-TS over TCP.

### Automated tests

| Test | Label | Covers |
| --- | --- | --- |
| `GStreamerTest` | Integration | Helpers, the source factory, the `gstqgc` sink and the `HwBuffers` paths. |
| `GstVideoReceiverComponentsTest` | Unit | The receiver's components without a stream: reconnect backoff, stall detection, recording finalization by ancestry, pipeline and `.dot` file names. |
| `GstVideoReceiverTest` | Integration | `GstVideoReceiver` end to end against a live H.264 RTP stream it sends itself on loopback. |

Each test skips, rather than fails, when GStreamer or a plugin it needs is missing (`GstVideoReceiverTest` needs `x264enc` among others).

```bash
cd build && QT_QPA_PLATFORM=offscreen ctest -R '^(GStreamerTest|GstVideoReceiverComponentsTest|GstVideoReceiverTest)$' --output-on-failure
```

## GStreamer Logging

- In-app: Use QGroundControl's logging settings
- Environment variables: See <https://gstreamer.freedesktop.org/documentation/gstreamer/running.html>
- Command line options: `--gst-debug-level`, `--gst-debug`, `--gst-debug-help`, etc.
- Receiver flow (starts, stops, reconnects, the decoder chosen, pipeline latency, GPU path statistics): enable the `Video.GStreamer.GstVideoReceiver` category. Failures are logged as warnings even with it off.

## Pipeline Observability

QGC supports the standard GStreamer observability env vars. Set them before launching QGC.

### Pipeline graph dumps (`.dot` files)

When `GST_DEBUG_DUMP_DOT_DIR` is set, QGC writes each receiver's pipeline graph there at key transitions as `receiver-<stream>-<tag>.dot`. `<stream>` is the receiver's name (`videoContent`, `thermalVideo`), so the two receivers don't overwrite each other's files, and `<tag>` names the transition (`pipeline-initial`, `pipeline-started`, `pipeline-with-videosink`, `pipeline-error`, `pipeline-watchdog-timeout`, `pipeline-recording-stopped`, …). A transition's next dump overwrites its last one.

```bash
export GST_DEBUG_DUMP_DOT_DIR=/tmp/qgc-pipeline-dots
./QGroundControl
dot -Tpng /tmp/qgc-pipeline-dots/receiver-videoContent-pipeline-started.dot -o pipeline.png
```

When the env var is **unset**, QGC instead writes a snapshot to `<CacheLocation>/qgc-pipeline-dot/receiver-<stream>-<tag>-<UTC timestamp>.dot` on a pipeline `ERROR` and on a watchdog timeout, keeping the 10 newest, so field-bug-report bundles include the topology automatically. `GstVideoReceiver::dumpPipelineGraph(tag)` (`Q_INVOKABLE`, so callable from QML) writes the same snapshot on demand for use from a debug menu.

Every dump shows topology, caps, media types and states, but leaves out element properties, which can hold stream credentials (such as `rtspsrc`'s `user-id` and `user-pw`).

### Latency tracer

Per-element latency from source to sink:

```bash
GST_TRACERS="latency(flags=pipeline+element)" GST_DEBUG=GST_TRACER:7 ./QGroundControl 2> latency.log
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
GST_TRACERS=leaks GST_DEBUG=GST_TRACER:7 ./QGroundControl
```

Logs every `GstObject` / `GstMiniObject` that wasn't released by exit. Useful when triaging pad-leak regressions in the source factory or sink bin.
