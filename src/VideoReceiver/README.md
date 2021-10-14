# QGroundControl

## Video Streaming

For supported platforms, QGroundControl implements an UDP RTP and RSTP video streaming receiver in its Main Flight Display. It uses GStreamer and a stripped down version of QtGstreamer. We've standardized on **GStreamer 1.18.1**. It has been reliable and we will be using it until a good reason to change it surfaces. Newer versions of GStreamer may break the build as some dependent libraries may change.
To build video streaming support, you will need to install the GStreamer development packages for the desired target platform.

If you do have the proper GStreamer development libraries installed where QGC looks for it, the QGC build system will automatically use it and build video streaming support. If you would like to disable video streaming support, you can add **DISABLE_VIDEOSTREAMING** to the **DEFINES** build variable.

### Gstreamer logs

For cases, when it is need to have more control over gstreamer logging than is availabe via QGroundControl's UI, it is possible to configure gstreamer logging via environment variables. Please see https://developer.gnome.org/gstreamer/stable/gst-running.html for details.

### UDP Pipeline

For the time being, the RTP UDP pipeline is somewhat hardcoded, using h.264 or h.265. It's best to use a camera capable of hardware encoding either h.264 (such as the Logitech C920) or h.265. On the sender end, for RTP (UDP Streaming) you would run something like this:

h.264
```
gst-launch-1.0 uvch264src initial-bitrate=1000000 average-bitrate=1000000 iframe-period=1000 device=/dev/video0 name=src auto-start=true src.vidsrc ! video/x-h264,width=1920,height=1080,framerate=24/1 ! h264parse ! rtph264pay ! udpsink host=xxx.xxx.xxx.xxx port=5600
```

h.265
```
ffmpeg -f v4l2 -i /dev/video1 -pix_fmt yuv420p -c:v libx265 -preset ultrafast -x265-params crf=23 -strict experimental -f rtp udp://xxx.xxx.xxx.xxx:5600
```

Where xxx.xxx.xxx.xxx is the IP address where QGC is running.


To test using a test source on localhost, you can run this command:
```
gst-launch-1.0 videotestsrc pattern=ball ! video/x-raw,width=640,height=480 ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5600
```
Or this one:
```
gst-launch-1.0 videotestsrc ! video/x-raw,width=640,height=480 ! videoconvert ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5600
```

On the receiving end, if you want to test it from the command line, you can use something like:
```
gst-launch-1.0 udpsrc port=5600 caps='application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264' ! rtpjitterbuffer ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink fps-update-interval=1000 sync=false
```
Or this one:
```
gst-launch-1.0 udpsrc port=5600 ! application/x-rtp ! rtpjitterbuffer ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink
```

### Additional Protocols

QGC also supports RTSP, TCP-MPEG2 and MPEG-TS (h.264) pipelines.

### Linux

Use apt-get to install GStreamer 1.0
```
list=$(apt-cache --names-only search ^gstreamer1.0-* | awk '{ print $1 }' | sed -e /-doc/d | grep -v gstreamer1.0-hybris)
```
```
sudo apt-get install $list
```
```
sudo apt-get install libgstreamer-plugins-base1.0-dev
sudo apt-get install libgstreamer-plugins-bad1.0-dev 
```

The build system is setup to use pkgconfig and it will find the necessary headers and libraries automatically.

### Mac OS

Download the gstreamer framework from here: http://gstreamer.freedesktop.org/data/pkg/osx. Supported version is 1.18.1. QGC may work with newer version, but it is untested.

You need two packages:
- [gstreamer-1.0-devel-1.18.1-x86_64.pkg](https://gstreamer.freedesktop.org/data/pkg/osx/1.18.1/gstreamer-1.0-devel-1.18.1-x86_64.pkg)
- [gstreamer-1.0-1.18.1-x86_64.pkg](https://gstreamer.freedesktop.org/data/pkg/osx/1.18.1/gstreamer-1.0-1.18.1-x86_64.pkg)

The installer places them under /Library/Frameworks/GStreamer.framework, which is where the QGC build system will look for it. That's all that is needed. When you build QGC and it finds the gstreamer framework, it automatically builds video streaming support.

:point_right: To run gstreamer commands from the command line, you can add the path to find them (either in ~/.profile or ~/.bashrc):
```
export PATH=$PATH:/Library/Frameworks/GStreamer.framework/Commands
```

### iOS

Download the gstreamer framework from here: [gstreamer-1.0-devel-1.14.4-ios-universal.pkg](https://gstreamer.freedesktop.org/data/pkg/ios/1.14.4/gstreamer-1.0-devel-1.14.4-ios-universal.pkg)

The installer places them under ~/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework, which is where the QGC build system will look for it. That's all that is needed. When you build QGC and it finds the gstreamer framework, it automatically builds video streaming support.

### Android

Download the gstreamer from here: [gstreamer-1.0-android-universal-1.18.5.tar.xz](https://gstreamer.freedesktop.org/data/pkg/android/1.18.5/gstreamer-1.0-android-universal-1.18.5.tar.xz)

Create a directory named "gstreamer-1.0-android-universal-1.18.5" under the root qgroundcontrol directory (the same directory qgroundcontrol.pro is located). Extract the downloaded archive under this directory. That's where the build system will look for it. Make sure your archive tool doesn't create any additional top level directories. The structure after extracting the archive should look like this:
```
qgroundcontrol
├── gstreamer-1.0-android-universal-1.18.5
│   │
│   ├──armv7
│   │   ├── bin
│   │   ├── etc
│   │   ├── include
│   │   ├── lib
│   │   └── share
│   ├──x86
```
### Windows

Download the gstreamer framework from here: http://gstreamer.freedesktop.org/data/pkg/windows. Supported version is 1.18.1. QGC may work with newer version, but it is untested.

You need two packages:

#### 32-Bit: 
- [gstreamer-1.0-devel-msvc-x86-1.18.1.msi](https://gstreamer.freedesktop.org/data/pkg/windows/1.18.1/msvc/gstreamer-1.0-devel-msvc-x86-1.18.1.msi)
- [gstreamer-1.0-msvc-x86-1.18.1.msi](https://gstreamer.freedesktop.org/data/pkg/windows/1.18.1/msvc/gstreamer-1.0-msvc-x86-1.18.1.msi)

#### 64-Bit: 
- [gstreamer-1.0-devel-msvc-x86_64-1.18.1.msi](https://gstreamer.freedesktop.org/data/pkg/windows/1.18.1/msvc/gstreamer-1.0-devel-msvc-x86_64-1.18.1.msi)
- [gstreamer-1.0-msvc-x86_64-1.18.1.msi](https://gstreamer.freedesktop.org/data/pkg/windows/1.18.1/msvc/gstreamer-1.0-msvc-x86_64-1.18.1.msi)

Make sure you select "Complete" installation instead of "Typical" installation during the install process. The installer places them under c:\gstreamer, which is where the QGC build system will look for it.
