# Video Streaming Test Servers for QGroundControl

This directory contains minimal test servers for validating QGroundControl's HTTP/HTTPS MJPEG and WebSocket video streaming capabilities without requiring physical cameras or external hardware.

## Overview

These test servers generate synthetic video patterns (color bars, moving elements, timestamps) and stream them using industry-standard protocols. They follow the same pattern as other QGC test utilities (e.g., `test/ADSB/ADSB_Simulator.py`).

## Quick Start

### 1. Install Dependencies

```bash
# Navigate to this directory
cd test/VideoStreaming

# Install required Python packages
pip install -r requirements.txt
```

### 2. Start a Test Server

**HTTP MJPEG Server:**
```bash
python http_mjpeg_server.py
# Default: http://127.0.0.1:5077/video_feed
```

**WebSocket Server:**
```bash
python websocket_video_server.py
# Default: ws://127.0.0.1:5077/ws/video_feed
```

### 3. Configure QGroundControl

**For HTTP MJPEG:**
1. Open QGroundControl Settings → Video
2. Set **Video Source** to `HTTP / HTTPS Video Stream`
3. Set **URL** to `http://127.0.0.1:5077/video_feed`
4. Click **Apply**
5. View video in the main display

**For WebSocket:**
1. Open QGroundControl Settings → Video
2. Set **Video Source** to `WebSocket Video Stream`
3. Set **URL** to `ws://127.0.0.1:5077/ws/video_feed`
4. Click **Apply**
5. View video in the main display

## Server Options

Both servers support the same command-line arguments:

```bash
python http_mjpeg_server.py --help
python websocket_video_server.py --help
```

Common options:
- `--host HOST` - Bind address (default: 127.0.0.1)
- `--port PORT` - Server port (default: 5077 for both servers)
- `--width WIDTH` - Video width in pixels (default: 640)
- `--height HEIGHT` - Video height in pixels (default: 480)
- `--fps FPS` - Frames per second (default: 30)
- `--quality QUALITY` - JPEG quality 0-100 (default: 85)

### Examples

**Custom resolution and frame rate:**
```bash
python http_mjpeg_server.py --width 1280 --height 720 --fps 60
```

**Network accessible (for testing from another device):**
```bash
python http_mjpeg_server.py --host 0.0.0.0 --port 8080
# Then use: http://<your-ip>:8080/video_feed in QGC
```

**Lower quality for bandwidth testing:**
```bash
python websocket_video_server.py --quality 50 --fps 15
```

## Testing WebSocket in Browser

The WebSocket server includes a built-in test page:

1. Start the WebSocket server
2. Open in browser: http://127.0.0.1:5077/test
3. You should see the live video stream

This helps verify the server is working before testing in QGC.

## Alternative: Pure GStreamer Command-Line

If you prefer not to use Python, you can test with GStreamer command-line tools directly.

### HTTP MJPEG Streaming (GStreamer CLI)

**Server side - Generate and stream MJPEG:**
```bash
# Simple test pattern over TCP
gst-launch-1.0 videotestsrc ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    jpegenc ! multipartmux ! \
    tcpserversink host=127.0.0.1 port=5000

# With more realistic test pattern
gst-launch-1.0 videotestsrc pattern=smpte ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    timeoverlay ! jpegenc quality=85 ! multipartmux ! \
    tcpserversink host=0.0.0.0 port=5000
```

**Client side - Test reception (optional):**
```bash
gst-launch-1.0 tcpclientsrc host=127.0.0.1 port=5000 ! \
    multipartdemux ! jpegdec ! \
    videoconvert ! autovideosink
```

**Notes:**
- GStreamer's built-in `tcpserversink` doesn't provide HTTP headers, so you'll need an HTTP wrapper or use QGC's raw TCP support (if available)
- For true HTTP MJPEG, consider using `souphttpsrc` on the client side or third-party tools
- The Python scripts above are recommended as they provide proper HTTP headers and are easier to use

### WebSocket Streaming (GStreamer CLI)

Pure GStreamer command-line WebSocket streaming requires custom GStreamer plugins or external tools. **We recommend using the Python `websocket_video_server.py` script instead**, as it's simpler and more reliable.

If you need a GStreamer-native solution, consider:
- **gst-rtsp-server** - For RTSP streaming (different protocol but GStreamer-native)
- **Custom appsrc/appsink** - Requires C/Python code (similar to our Python script)

### RTSP Streaming (Alternative)

RTSP is another common protocol supported by GStreamer:

```bash
# Requires gst-rtsp-server (separate package)
gst-rtsp-server \
    --gst-debug=3 \
    --factory /test "videotestsrc ! x264enc ! rtph264pay name=pay0"

# Then connect to: rtsp://127.0.0.1:8554/test
```

## Using with Real Video Sources

While these scripts generate synthetic test patterns, you can easily modify them to use:

### Webcam
```python
# In http_mjpeg_server.py or websocket_video_server.py
# Replace generate_frame() with:
cap = cv2.VideoCapture(0)  # 0 = default webcam
ret, frame = cap.read()
```

### Video File
```python
# Replace generate_frame() with:
cap = cv2.VideoCapture('test_video.mp4')
ret, frame = cap.read()
if not ret:
    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)  # Loop video
    ret, frame = cap.read()
```

### External Tools

Instead of these test scripts, you can also use:

- **[PixEagle](https://github.com/alireza787b/PixEagle)** - Full-featured drone simulator with HTTP MJPEG video streaming
- **[GStreamer RTSP Server](https://gstreamer.freedesktop.org/documentation/gst-rtsp-server/)** - For RTSP protocol testing
- **[FFmpeg](https://ffmpeg.org/)** - For advanced streaming scenarios
- **[OBS Studio](https://obsproject.com/)** - Can stream via RTMP/RTSP with plugins

## Troubleshooting

### "ModuleNotFoundError: No module named 'fastapi'"
Install dependencies: `pip install -r requirements.txt`

### "Address already in use"
Another service is using the port. Either:
- Stop the other service
- Use a different port: `--port 8080`

### "Cannot connect from QGC"
1. Check firewall settings (allow Python or the specific port)
2. Verify the server is running (you should see startup logs)
3. Check the URL in QGC matches exactly (including http:// or ws://)
4. Try accessing http://127.0.0.1:5077/ in a browser to verify server is responding

### Video is choppy or delayed
- Reduce frame rate: `--fps 15`
- Lower quality: `--quality 60`
- Reduce resolution: `--width 320 --height 240`

### WebSocket disconnects immediately
- Check QGC logs for errors
- Verify WebSocket URL starts with `ws://` not `http://`
- Test with the browser test page first: http://127.0.0.1:5077/test

## Technical Details

### HTTP MJPEG Format

The HTTP MJPEG server streams video using the `multipart/x-mixed-replace` content type, which is the standard for MJPEG-over-HTTP:

```
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg

<JPEG image data>
--frame
Content-Type: image/jpeg

<JPEG image data>
...
```

### WebSocket Protocol (QGC/PixEagle Format)

The WebSocket server implements the QGC/PixEagle protocol with a two-message sequence per frame:

**1. Frame Metadata (Text JSON message):**
```json
{
  "type": "frame",
  "size": 12345,
  "quality": 85
}
```

**2. Frame Data (Binary message):**
- Raw JPEG image bytes

**Additional Protocol Messages:**

**Heartbeat (QGC → Server):**
```json
{"type": "ping", "timestamp": 1234567890}
```

**Heartbeat Response (Server → QGC):**
```json
{"type": "pong"}
```

**Quality Change Request (QGC → Server):**
```json
{"type": "quality", "quality": 60}
```

**Error Message (Server → QGC):**
```json
{"type": "error", "message": "Error description"}
```

This protocol ensures proper frame synchronization and allows QGC to adapt video quality dynamically.

### GStreamer Pipeline (QGC Side)

When QGC receives these streams, it uses GStreamer pipelines similar to:

**HTTP MJPEG:**
```
souphttpsrc → queue → multipartdemux → jpegdec → [display/record]
```

**WebSocket:**
```
appsrc → queue → jpegdec → [display/record]
```

## Contributing

If you improve these test servers or add new features:
1. Ensure they remain minimal and easy to run
2. Keep dependencies limited (fastapi, uvicorn, opencv-python, numpy)
3. Update this README with new features
4. Test on multiple platforms (Windows, Linux, macOS)

## License

These test scripts follow the same license as QGroundControl itself.
See the root `COPYING.md` for details.

## Related Resources

- [QGroundControl Developer Guide](https://dev.qgroundcontrol.com/)
- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [FastAPI Documentation](https://fastapi.tiangolo.com/)
- [OpenCV Python Documentation](https://docs.opencv.org/4.x/d6/d00/tutorial_py_root.html)
