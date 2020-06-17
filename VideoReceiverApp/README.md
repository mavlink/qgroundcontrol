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
 
 ```tsusb://<interface>:<port>``` - Taisync's forwarded H.264 byte aligned NALU stream over UDP
 
 ```tcp://<host>:<port>``` - MPEG-2 TS over TCP
 
 ```mpegts://<interface>:<port>``` - MPEG-2 TS over UDP
 
