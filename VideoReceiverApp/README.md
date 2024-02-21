Logging Support
The scene graph has support for a number of logging categories. These can be useful in tracking down both performance issues and bugs in addition to being helpful to Qt contributors.

qt.scenegraph.time.texture - logs the time spent doing texture uploads
qt.scenegraph.time.compilation - logs the time spent doing shader compilation
qt.scenegraph.time.renderer - logs the time spent in the various steps of the renderer
qt.scenegraph.time.renderloop - logs the time spent in the various steps of the render loop. With the threaded render loop this gives an insight into the time elapsed between the various frame preparation steps both on the GUI and the render thread. It can therefore also be a useful troubleshooting tool, for example, to confirm how vsync-based throttling and other low-level Qt enablers, such as QWindow::requestUpdate(), affect the rendering and presentation pipeline.
qt.scenegraph.time.glyph - logs the time spent preparing distance field glyphs
qt.scenegraph.general - logs general information about various parts of the scene graph and the graphics stack
qt.scenegraph.renderloop - creates a detailed log of the various stages involved in rendering. This log mode is primarily useful for developers working on Qt.
The legacy QSG_INFO environment variable is also available. Setting it to a non-zero value enables the qt.scenegraph.general category.

Note: When encountering graphics problems, or when in doubt which render loop or graphics API is in use, always start the application with at least qt.scenegraph.general and qt.rhi.* enabled, or QSG_INFO=1 set. This will then print some essential information onto the debug output during initialization.

# VideoReceiverApp
 
## Application
 
This is a simple test application developed to make VideoReceiver library development and testing easier. It can also be used as part of CI for system tests.
 
## Use cases and options
 
Application's behaviour depends on the executable name. There are two modes - QML and console. QML mode is enabled by renaming application executable to something that starts with **Q** (for example QVideoReceiverApp). In this case **video-sink** option is not available and application always tries to use **qmlglsink** for video rendering. In regular case (executable name does not start with **Q**) **autovideosink** or **fakesink** are used, depending on options.
 
### Available options and required arguments
 ```VideoReceiverApp  [options] url```
 
for example:
 
 ```VideoReceiverApp -d --stop-decoding 30 rtsp://127.0.0.1:8554/test```
 
#### Options
 ```-h, --help``` - displays help
 
 ```-t, --timeout <seconds>``` - specifies source timeout
 
 ```-c, --connect <attempts>``` - specifies number of connection attempts
 
 ```-d, --decode``` - enables or disables video decoding and rendering
 
 ```--no-decode``` - disables video decoding and rendering if it was enabled by default
 
 ```--stop-decoding <seconds>``` - specifies amount of seconds after which decoding should be stopped
 
 ```-r, --record <file>```  - enables record video into file
 
 ```-f, --format <format>``` - specifies recording file format, where format 0 - MKV, 1 - MOV, 2 - MP4
 
 ```--stop-recording <seconds>``` - specifies amount of seconds after which recording should be stopped
  ```--video-sink <sink>``` - specifies which video sink to use : 0 - autovideosink, 1 - fakesink
 
#### Arguments
 ```url``` - required, specifies video URL.
  Following URLs are supported:
  ```rtsp://<host>:<port>/mount/point``` - usual RTSP URL
 
 ```udp://<interface>:<port>``` - H.264 over RTP/UDP
 
 ```udp265://<interface>:<port>``` - H.265 over RTP/UDP
 
 ```tcp://<host>:<port>``` - MPEG-2 TS over TCP
 
 ```mpegts://<interface>:<port>``` - MPEG-2 TS over UDP
 
