# QGroundControl Ground Control Station

[![Releases](https://img.shields.io/github/release/mavlink/QGroundControl.svg)](https://github.com/mavlink/QGroundControl/releases)
[![Travis Build Status](https://travis-ci.org/mavlink/qgroundcontrol.svg?branch=master)](https://travis-ci.org/mavlink/qgroundcontrol)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/crxcm4qayejuvh6c/branch/master?svg=true)](https://ci.appveyor.com/project/mavlink/qgroundcontrol)

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mavlink/qgroundcontrol?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Website: <http://qgroundcontrol.com>

## Obtaining source code

Source code for QGroundControl is kept on GitHub: https://github.com/mavlink/qgroundcontrol.
```
git clone --recursive https://github.com/mavlink/qgroundcontrol.git
```
Each time you pull new source to your repository you should run `git submodule update` to get the latest submodules as well. Since QGroundControl uses submodules, using the zip file for source download will not work. You must use git.

The source code is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/COPYING.md).

### User Manual
https://docs.qgroundcontrol.com/en/

### Supported Builds

#### Native Builds
QGroundControl builds are supported for OSX, Linux, Windows, iOS and Android. QGroundControl uses [Qt](http://www.qt.io) as its cross-platform support library and uses [QtCreator](http://doc.qt.io/qtcreator/index.html) as its default build environment.

* OSX: OSX 10.7 or higher, 64 bit, clang compiler (IMPORTANT: XCode 8 requires a workaround described below)
* Ubuntu: 64 bit, gcc compiler
* Windows: Vista or higher, 32 bit, [Visual Studio 2015 compiler](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop)
* iOS: 8.0 and higher
* Android: Jelly Bean (4.1) and higher. Standard QGC is built against ndk version 19.
* Qt version: **5.9.3 only**

###### Install QT
You **need to install Qt as described below** instead of using pre-built packages from say, a Linux distribution, because QGroundControl needs access to private Qt headers.
* Download the [Qt installer](http://www.qt.io/download-open-source)
    * Make sure to install Qt version **5.9.3**. You will also need to install the Qt Speech package.
    * Ubuntu: Set the downloaded file to executable using:`chmod +x`. Install to default location for use with ./qgroundcontrol-start.sh. If you install Qt to a non-default location you will need to modify qgroundcontrol-start.sh in order to run downloaded builds.
    * Windows: Make sure to install VS 2015 32 bit package.

###### Install additional packages:
* Ubuntu: sudo apt-get install speech-dispatcher libudev-dev libsdl2-dev
* Fedora: sudo dnf install speech-dispatcher SDL2-devel SDL2 systemd-devel
* Arch Linux: pacman -Sy speech-dispatcher
* Windows: [USB Driver](http://www.pixhawk.org/firmware/downloads) to connect to Pixhawk/PX4Flow/3DR Radio
* Android: [Qt Android Setup](http://doc.qt.io/qt-5/androidgs.html)

###### Building using Qt Creator

* Launch Qt Creator and open the `qgroundcontrol.pro` project.
* Select the appropriate kit for your needs:
    * OSX: Desktop Qt 5.9.3 clang 64 bit
    * Ubuntu: Desktop Qt 5.9.3 GCC bit
    * Windows: Desktop Qt 5.9.3 MSVC2015 32bit
    * Android: Android for armeabi-v7a (GCC 4.9, Qt 5.9.3)
* Note: iOS builds must be built using xCode: http://doc.qt.io/qt-5/ios-support.html. Use Qt Creator to generate the XCode project (*Run Qmake* from the context menu).

#### Vagrant

A Vagrantfile is provided to build QGroundControl using the [Vagrant](https://www.vagrantup.com/) system. This will produce a native Linux build which can be run in the Vagrant Virtual Machine or on the host machine if it is compatible.

* [Download](https://www.vagrantup.com/downloads.html) Vagrant
* [Install](https://www.vagrantup.com/docs/getting-started/) Vagrant
* From the root directory of the QGroundControl repository run "vagrant up"
* To use the graphical environment run "vagrant reload"

#### Additional build notes for all supported OS

* Warnings as Errors: Specifying `CONFIG+=WarningsAsErrorsOn` will turn all warnings into errors which breaks the build. If you are working on a pull request you plan to submit to github for consideration, you should always run with this setting turned on, since it is required for all pull requests. NOTE: Putting this line into a file called "user_config.pri" in the top-level directory (same directory as `qgroundcontrol.pro`) will set this flag on all builds without interfering with the GIT history.
* Parallel builds: For non Windows builds, you can use the '-j#' option to run parellel builds.
* Location of built files: Individual build file results can be found in the `build_debug` or `build_release` directories. The built executable can be found in the `debug` or `release` directory.
* If you get this error when running qgroundcontrol: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version 'GLIBCXX_3.4.20' not found. You need to either update to the latest gcc, or install the latest libstdc++.6 using: sudo apt-get install libstdc++6.
* Unit tests: To run unit tests it's necessary to build in `debug` mode with `UNITTEST_BUILD` definition. After that it's necessary to copy `deploy/qgroundcontrol-start.sh` script in `debug` directory before running `qgroundcontrol-start.sh --unittests`.

## Additional functionality
QGroundControl has functionality that is dependent on the operating system and libraries installed by the user. The following sections describe these features, their dependencies, and how to disable/alter them during the build process. These features can be forcibly enabled/disabled by specifying additional values to qmake. 

### Opal-RT's RT-LAB simulator
Integration with Opal-RT's RT-LAB simulator can be enabled on Windows by installing RT-LAB 7.2.4. This allows vehicles to be simulated in RT-LAB and communicate directly with QGC on the same computer as if the UAS was actually deployed. This support is enabled by default once the requisite RT-LAB software is installed. Disabling this can be done by adding `DEFINES+=DISABLE_RTLAB` to qmake.

### XBee support
QGroundControl can talk to XBee wireless devices using their proprietary protocol directly on Windows and Linux platforms. This support is not necessary if you're not using XBee devices or aren't using their proprietary protocol. On Windows, the necessary dependencies are included in this repository and no additional steps are required. For Linux, change to the `libs/thirdParty/libxbee` folder and run `make;sudo make install` to install libxbee on your system (uninstalling can be done with a `sudo make uninstall`). `qmake` will automatically detect the library on Linux, so no other work is necessary.

To disable XBee support you may add `DEFINES+=DISABLE_XBEE` to qmake.

### Video Streaming
Check the [Video Streaming](https://github.com/mavlink/qgroundcontrol/tree/master/src/VideoStreaming) directory for further instructions.
