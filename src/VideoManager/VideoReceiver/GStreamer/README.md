# GStreamer Video Streaming

QGroundControl uses GStreamer for UDP RTP and RTSP video streaming in the Main Flight Display.

## Source Code Architecture

The pipeline is split into focused components (all in this directory):

| Component | Role |
| --- | --- |
| `GstVideoReceiver` | Pipeline owner: builds/starts/stops the receiver pipeline, recording, watchdog, and `.dot` dumps. Exposes decoder/telemetry as `Q_PROPERTY`s to QML. |
| `GstSourceFactory` | Constructs the source element from the stream URI and applies RTP jitter-buffer policy for `application/x-rtp` sources. |
| `GStreamerEnvironment` | Process-wide environment setup (plugin/scanner/helper/GIO paths, env hygiene) run before `gst_init()`. See [Runtime Environment Setup](#runtime-environment-setup). |
| `GStreamerHelpers` | Small utilities (element-rank overrides, etc.). |
| `GStreamerLogging` | Routes GStreamer's log output through QGC's logging and applies the persisted debug level. |
| `GstScoped` | RAII owners for transfer-full GStreamer returns so refs can't leak on early return. |

### `gstqgc` — custom GStreamer plugin

A QGC-owned plugin (`gstqgc/`) bridging the pipeline into Qt:

- **`qgcqvideosink`** — a `GstVideoSink` that pushes decoded frames into a Qt `QVideoSink`.
- **`qgcvideosinkbin`** — a bin wrapping a format-restriction capsfilter plus `qgcqvideosink`.
- **`QGCQVideoSinkController`** — GUI-thread companion that mirrors negotiation/telemetry from the sink into QGC.

### `HwBuffers` — zero-copy GPU paths

Per-platform zero-copy import of decoded GPU frames, all sharing `GstHwFrameTexturesBase : QVideoFrameTextures`:

- **Platforms**: `dmabuf` (Linux/VA-API), `gl`, `vulkan`, `d3d` (D3D11/D3D12), `cuda`, `apple` (IOSurface), `android` (AHardwareBuffer).
- **`GstContextBridgeRegistry`** fans `GstBus` sync messages out to every compiled context bridge for cross-thread GPU-context handoff.
- **`GstHwPathTelemetry`** tracks per-format/per-path counters (map failures, reuse hits, sync waits) for fallback diagnostics.
- **`CpuVideoFramePool`** recycles CPU-backed `QVideoFrame` storage (WebRTC-style slab) for the software-fallback path, avoiding per-frame allocation.

## Build Configuration

- **Enable/disable**: Set `QGC_ENABLE_GST_VIDEOSTREAMING` CMake option to `ON`/`OFF`
- **Version & URLs**: Defined in [`.github/build-config.json`](../../../../.github/build-config.json), parsed by [`cmake/BuildConfig.cmake`](../../../../cmake/BuildConfig.cmake)
- **SDK discovery & auto-download**: [`cmake/GStreamer/Orchestrator.cmake`](../../../../cmake/GStreamer/Orchestrator.cmake)
- **Plugin allowlist**: `gstreamer.plugins` in [`.github/build-config.json`](../../../../.github/build-config.json) — controls both static linking (mobile) and dynamic install (desktop)
- **Install helpers**: [`cmake/GStreamer/Helpers.cmake`](../../../../cmake/GStreamer/Helpers.cmake) (aggregator), with focused submodules in [`cmake/GStreamer/`](../../../../cmake/GStreamer/)

## Runtime Environment Setup

QGC configures GStreamer runtime paths before `gst_init()`:

- **Desktop (Linux/macOS/Windows)**: `GStreamer::prepareEnvironment()` sets plugin paths, scanner/helper paths, and GIO module paths when bundled runtimes are detected.
- **Linux AppImage**: [`deploy/linux/AppRun`](../../../../deploy/linux/AppRun) exports `GST_PLUGIN_*`, `GST_PLUGIN_SCANNER*`, and `GST_PTP_HELPER*` only when bundled paths are valid.
- **Environment hygiene**: Python virtualenv/conda variables are cleared for scanner stability (`PYTHONHOME`, `PYTHONPATH`, `VIRTUAL_ENV`, `CONDA_*`), and `PYTHONNOUSERSITE=1` is set.
- **Validation**: If bundled plugin directories are present but `gst-plugin-scanner` is missing or non-executable, QGC fails initialization early with a clear error instead of running with a broken plugin loader.

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

### macOS / Windows / iOS

GStreamer SDKs are auto-downloaded during CMake configure. To use a local installation instead, set `GStreamer_ROOT_DIR`.

### Android

Auto-downloaded during CMake configure. No manual setup required.

> **Windows users building for Android**: Enable Developer Mode (Settings > System > For developers) to support symbolic links during the build.

### Caching downloaded SDKs

Auto-downloaded SDK archives are cached to `${CPM_SOURCE_CACHE}/gstreamer-*` when `CPM_SOURCE_CACHE` is set, otherwise to `${CMAKE_BINARY_DIR}/_deps/gstreamer-*` (lost on `rm -rf build`). Set `CPM_SOURCE_CACHE` to a stable location (e.g. `~/.cache/CPM`) to avoid re-downloading the 200-700 MB archives on every clean build:

```bash
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
```

Cached archives are checksum-verified against the upstream `.sha256` on every hit.

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

UDP RTP, RTSP, TCP-MPEG2, MPEG-TS.

## GStreamer Logging

- In-app: Use QGroundControl's logging settings
- Environment variables: See <https://gstreamer.freedesktop.org/documentation/gstreamer/running.html>
- Command line options: `--gst-debug-level`, `--gst-debug`, `--gst-debug-help`, etc.

## Pipeline Observability

QGC supports the standard GStreamer observability env vars. Set them before launching QGC.

### Pipeline graph dumps (`.dot` files)

When `GST_DEBUG_DUMP_DOT_DIR` is set, QGC writes the receiver pipeline graph at key transitions (`pipeline-initial`, `pipeline-started`, `pipeline-with-videosink`, `pipeline-error`, `pipeline-watchdog-timeout`, `pipeline-recording-stopped`, …):

```bash
export GST_DEBUG_DUMP_DOT_DIR=/tmp/qgc-pipeline-dots
./QGroundControl
dot -Tpng /tmp/qgc-pipeline-dots/0.00.00.*-pipeline-started.dot -o pipeline.png
```

When the env var is **unset**, QGC still writes a rotating snapshot (≤10 files) to `<CacheLocation>/qgc-pipeline-dot/<tag>-<ts>.dot` on `ERROR` and on watchdog timeout, so field-bug-report bundles include the topology automatically. The `GstVideoReceiver::dumpPipelineGraph(tag)` slot (callable from QML) writes a snapshot on demand for use from a debug menu.

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
