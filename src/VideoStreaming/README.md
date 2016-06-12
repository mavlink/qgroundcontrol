# QGroundControl

## Video Streaming

For supported platforms, QGroundControl implements an UDP RTP video streaming receiver in its Main Flight Display. It uses GStreamer and a stripped down version of QtGstreamer.
To build video streaming support, you will need to install the GStreamer development packages for the desired target platform.

If you do have the proper GStreamer development libraries installed where QGC looks for it, the QGC build system will automatically use it and build video streaming support. If you would like to disable video streaming support, you can add **DISABLE_VIDEOSTREAMING** to the **DEFINES** build variable.

### Pipeline

For the time being, the pipeline is somewhat hardcoded, using h.264. It's best to use a camera capable of hardware encoding h.264, such as the Logitech C920. On the sender end, you would run something like this:

```
gst-launch-1.0 uvch264src initial-bitrate=1000000 average-bitrate=1000000 iframe-period=1000 device=/dev/video0 name=src auto-start=true src.vidsrc ! video/x-h264,width=1920,height=1080,framerate=24/1 ! h264parse ! rtph264pay ! udpsink host=xxx.xxx.xxx.xxx port=5600
```

Where xxx.xxx.xxx.xxx is the IP address where QGC is running. You may tweak the bitrate, the resolution and the FPS based on your needs and/or available bandwidth.

To test using a test source on localhost, you can run this command:
```
gst-launch-1.0 videotestsrc pattern=ball ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5600
```

On the receiving end, if you want to test it from the command line, you can use something like:
```
gst-launch-1.0 udpsrc port=5600 caps='application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264' ! rtph264depay ! avdec_h264 ! autovideosink fps-update-interval=1000 sync=false
```

### Linux

Use apt-get to install GStreamer 1.0
```
sudo apt-get install libgstreamer-plugins-base1.0-dev libgstreamer1.0-0:amd64 libgstreamer1.0-dev
```

The build system is setup to use pkgconfig and it will find the necessary headers and libraries automatically.

### Mac OS

Download the gstreamer framework from here: http://gstreamer.freedesktop.org/data/pkg/osx. The current version, as this is written is 1.5.2.

You need two packages:
- [gstreamer-1.0-devel-1.5.2-x86_64.pkg](http://gstreamer.freedesktop.org/data/pkg/osx/1.5.2/gstreamer-1.0-devel-1.5.2-x86_64.pkg)
- [gstreamer-1.0-1.5.2-x86_64.pkg](http://gstreamer.freedesktop.org/data/pkg/osx/1.5.2/gstreamer-1.0-1.5.2-x86_64.pkg)

The installer places them under /Library/Frameworks/GStreamer.framework, which is where the QGC build system will look for it. That's all that is needed. When you build QGC and it finds the gstreamer framework, it automatically builds video streaming support.

:point_right: To run gstreamer commands from the command line, you can add the path to find them (either in ~/.profile or ~/.bashrc):
```
export PATH=$PATH:/Library/Frameworks/GStreamer.framework/Commands
```

### iOS

TODO: Binaries found in http://gstreamer.freedesktop.org/data/pkg/ios
(work in progress)

### Android

Binaries found in http://gstreamer.freedesktop.org/data/pkg/android
Download the [gstreamer-1.0-android-armv7-1.5.2.tar.bz2](http://gstreamer.freedesktop.org/data/pkg/android/1.5.2/gstreamer-1.0-android-armv7-1.5.2.tar.bz2) archive (assuming you want the ARM V7 platform). 

Create a directory named "gstreamer-1.0-android-armv7-1.5.2" under the root qgroundcontrol directory (the same directory qgroundcontrol.pro is located). Extract the gstreamer tar file under this directory. That's where the build system will look for it. Make sure your archive tool doesn't create any additional top level directories. The structure after extracting the archive should look like this:

qgroundcontrol
├── gstreamer-1.0-android-armv7-1.5.2
│   ├── etc
│   ├── include
│   ├── lib
│   └── share

### Windows

TODO: Binaries found in http://gstreamer.freedesktop.org/data/pkg/windows
(work in progress)

