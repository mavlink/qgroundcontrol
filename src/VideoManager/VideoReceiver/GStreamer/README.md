# GStreamer Video Streaming

QGroundControl uses GStreamer for UDP RTP and RTSP video streaming in the Main Flight Display.

## Build Configuration

- **Enable/disable**: Set `QGC_ENABLE_GST_VIDEOSTREAMING` CMake option to `ON`/`OFF`
- **Version & URLs**: Defined in [`.github/build-config.json`](../../../../.github/build-config.json), parsed by [`cmake/BuildConfig.cmake`](../../../../cmake/BuildConfig.cmake)
- **SDK discovery & auto-download**: [`cmake/find-modules/FindGStreamerQGC.cmake`](../../../../cmake/find-modules/FindGStreamerQGC.cmake)
- **Plugin allowlist**: `GSTREAMER_PLUGINS` in `FindGStreamerQGC.cmake` — controls both static linking (mobile) and dynamic install (desktop)
- **Install helpers**: [`cmake/find-modules/GStreamerHelpers.cmake`](../../../../cmake/find-modules/GStreamerHelpers.cmake)

## Runtime Environment Setup

QGC configures GStreamer runtime paths before `gst_init()`:

- **Desktop (Linux/macOS/Windows)**: `GStreamer::prepareEnvironment()` sets plugin paths, scanner/helper paths, and GIO module paths when bundled runtimes are detected.
- **Linux AppImage**: [`deploy/linux/AppRun`](../../../../deploy/linux/AppRun) exports `GST_PLUGIN_*`, `GST_PLUGIN_SCANNER*`, and `GST_PTP_HELPER*` only when bundled paths are valid.
- **Environment hygiene**: Python virtualenv/conda variables are cleared for scanner stability (`PYTHONHOME`, `PYTHONPATH`, `VIRTUAL_ENV`, `CONDA_*`), and `PYTHONNOUSERSITE=1` is set.
- **Validation**: If bundled plugin directories are present but `gst-plugin-scanner` is missing or non-executable, QGC fails initialization early with a clear error instead of running with a broken plugin loader.

## Platform Setup

### Linux

Use `tools/setup/install-dependencies-debian.sh`, or manually:

```
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev python3-gi python3-gst-1.0
```

### macOS / Windows / iOS

GStreamer SDKs are auto-downloaded during CMake configure. To use a local installation instead, set `GStreamer_ROOT_DIR`.

### Android

Auto-downloaded during CMake configure. No manual setup required.

> **Windows users building for Android**: Enable Developer Mode (Settings > System > For developers) to support symbolic links during the build.

## Testing Pipelines

### Sending test video (h.264 over UDP)

```
gst-launch-1.0 videotestsrc pattern=ball ! video/x-raw,width=640,height=480 ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5600
```

### Receiving test video

```
gst-launch-1.0 udpsrc port=5600 caps='application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264' ! rtpjitterbuffer ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink sync=false
```

### Supported protocols

UDP RTP, RTSP, TCP-MPEG2, MPEG-TS.

## GStreamer Logging

- In-app: Use QGroundControl's logging settings
- Environment variables: See <https://gstreamer.freedesktop.org/documentation/gstreamer/running.html>
- Command line options: `--gst-debug-level`, `--gst-debug`, `--gst-debug-help`, etc.
