## Building QGC on a stock CentOS:

These were the options used to setup a CentOS development system. Use it as a guideline.

#### CentOS Installation
![CentOS Installation](https://github.com/mavlink/qgroundcontrol/blob/master/resources/CentOS/CentOS-installation.png)

#### CentOS Software Selection
![CentOS Software Selection](https://github.com/mavlink/qgroundcontrol/blob/master/resources/CentOS/CentOS-sw-selection.png)

Once the system is fully installed and running, we need to set it up for QGC development. First, we need to update GStreamer to a more recent version. This guide follows Alice Wonder's tips found here: https://media.librelamp.com

#### Update to a more current GStreamer
```
sudo yum install epel-release
wget http://awel.domblogger.net/7/media/x86_64/awel-media-release-7-6.noarch.rpm
sudo yum localinstall awel-media-release-7-6.noarch.rpm
sudo yum clean all
sudo yum update
sudo yum install gstreamer1*
```
Make sure these are installed (vaapi for Intel GPUs)
```
sudo yum install gstreamer1-vaapi
sudo yum install gstreamer1-libav
```

#### Installing SDL2

SDL2 is used for joystick support.

```
sudo yum install SDL2
sudo yum install SDL2-devel
```

#### Installing Qt
```
mkdir ~/devel
cd ~/devel
```

Install Qt 5.12.4 from the Qt installation script which can be downloaded [here](https://www.qt.io/download-thank-you?os=linux&hsLang=en). Once downloaded, make it executable and run it:
```
chmod +x qt-unified-linux-x64-3.1.1-online.run
./qt-unified-linux-x64-3.1.1-online.run
```

Select the following options and install it under ~/devel/Qt
![Qt Software Selection](https://github.com/mavlink/qgroundcontrol/blob/master/resources/CentOS/Qt-Setup.png)

#### Clone and Build QGC
```
git clone --recursive https://github.com/mavlink/qgroundcontrol.git
mkdir build
cd build
```
For a debug/test build:
```
../Qt/5.12.4/gcc_64/bin/qmake ../qgroundcontrol/qgroundcontrol.pro -spec linux-g++ CONFIG+=debug
```
For a release build:
```
../Qt/5.12.4/gcc_64/bin/qmake ../qgroundcontrol/qgroundcontrol.pro spec linux-g++ CONFIG+=qtquickcompiler
```
Build it: (use the appropriate number of CPU cores for you machine)
```
make -j32
```

You can alternativelly launch QtCreator (found under `~/devel/Qt/Tools/QtCreator/bin/qtcreator`), load the `qgroundcontro.pro` project and build/debug from within its IDE.

By default, this will build the regular QGC. To build the sample, customized UI version, follow [these instructions](https://github.com/mavlink/qgroundcontrol/blob/master/custom-example/README.md).

#### Running QGC
Before launching QGC, you need to make sure the current user has access to the dialout group (serial port access permission):
```
sudo usermod -a -G dialout $USER
```
