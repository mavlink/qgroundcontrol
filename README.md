# QGroundControl Ground Control Station
## Open Source Micro Air Vehicle Ground Control Station

[![Releases](https://img.shields.io/github/release/mavlink/QGroundControl.svg)](https://github.com/mavlink/QGroundControl/releases)
[![Travis Build Status](https://travis-ci.org/mavlink/qgroundcontrol.svg?branch=master)](https://travis-ci.org/mavlink/qgroundcontrol)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/crxcm4qayejuvh6c/branch/master?svg=true)](https://ci.appveyor.com/project/mavlink/qgroundcontrol)

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mavlink/qgroundcontrol?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

The license terms are set in the COPYING.md file.

* Project:
<http://qgroundcontrol.org>

* Files:
<http://github.com/mavlink/qgroundcontrol>

* Credits:
<http://qgroundcontrol.org/credits>


## Obtaining source code
Source code for QGroundControl is kept on GitHub: https://github.com/mavlink/qgroundcontrol.
```
git clone --recursive https://github.com/mavlink/qgroundcontrol.git
```
Each time you pull new source to your repository you should run `git submodule update` to get the latest submodules as well. Since QGroundControl uses submodules, using the zip file for source download will not work. You must use git.

### User Manual
https://donlakeflyer.gitbooks.io/qgroundcontrol-user-guide/content/

### Supported Builds

#### Native Builds
QGroundControl builds are supported for OSX, Linux, Windows, iOS and Android. QGroundControl uses [Qt](http://www.qt.io) as its cross-platform support library and uses [QtCreator](http://doc.qt.io/qtcreator/index.html) as its default build environment.
* OSX: OSX 10.7 or higher, 64 bit, clang compiler (IMPORTANT: XCode 8 requires a workaround described below)
* Ubuntu: 64 bit, gcc compiler
* Windows: Vista or higher, 32 bit, [Visual Studio 2013 compiler](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop)
* iOS: 8.0 and higher
* Android: Jelly Bean (4.1) and higher
* Qt version: 5.5.1 ONLY

###### Install QT
You need to install Qt as described below instead of using pre-built packages from say, a Linux distribution because QGroundControl needs access to private Qt headers.
* Download the [Qt installer](http://www.qt.io/download-open-source)
    * Make sure to install Qt version **5.5.1** NOT 5.4.x, 5.6.x, 5.7.x, etc.  
    * Ubuntu: Set the downloaded file to executable using:`chmod +x`. Install to default location for use with ./qgroundcontrol-start.sh. If you install Qt to a non-default location you will need to modify qgroundcontrol-start.sh in order to run downloaded builds.
    * Windows: Default installer not quite correct, use [this](http://download.qt.io/official_releases/qt/5.5/5.5.1/qt-opensource-windows-x86-msvc2013-5.5.1.exe) instead

###### Install additional packages:
* Ubuntu: sudo apt-get install espeak libespeak-dev libudev-dev libsdl2-dev
* Fedora: sudo dnf install espeak espeak-devel SDL2-devel SDL2 systemd-devel
* Arch Linux: pacman -Sy espeak
* Windows: [USB Driver](http://www.pixhawk.org/firmware/downloads) to connect to Pixhawk/PX4Flow/3DR Radio
* Android: [Qt Android Setup](http://doc.qt.io/qt-5/androidgs.html)

###### Building using Qt Creator

* Launch Qt Creator and open the `qgroundcontrol.pro` project.
* Select the appropriate kit for your needs:
    * OSX: Desktop Qt 5.5.1 clang 64 bit
    * Ubuntu: Desktop Qt 5.5.1 GCC bit
    * Windows: Desktop Qt 5.5.1 MSVC2013 32bit
    * Android: Android for armeabi-v7a (GCC 4.9, Qt 5.5.1)
* Note: iOS builds must be built using xCode: http://doc.qt.io/qt-5/ios-support.html. Use Qt Creator to generate the XCode project (*Run Qmake* from the context menu).

###### Special case for Xcode 8 and Qt 5.5.1

Xcode 8 broke the Qt 5.5.1 build system (qmake). As this is the version of Qt we're using for the moment, you will need to patch your Qt installation to make the build work. This is for both Mac OS X and iOS.

Under the tools directory, you will find a script used by the CI build to patch Qt (`patch_qt_for_xcode8.sh`). You can use this script as a starting point. You will need to edit a few variables it expects to run:

```
IOSDIR=/<your_qt_path>/Qt/5.5/ios
OSXDIR=/<your_qt_path>/Qt/5.5/clang_64
TRAVIS_BUILD_DIR=/<your_github_repo>/qgroundcontrol
```

#### Vagrant

A Vagrantfile is provided to build QGroundControl using the [Vagrant](https://www.vagrantup.com/) system. This will produce a native Linux build which can be run in the Vagrant Virtual Machine or on the host machine if it is compatible.

* [Download](https://www.vagrantup.com/downloads.html) Vagrant
* [Install](https://www.vagrantup.com/docs/getting-started/) Vagrant
* From the root directory of the QGroundControl repository run "vagrant up"

#### Additional build notes for all supported OS

* Warnings as Errors: Specifying `CONFIG+=WarningsAsErrorsOn` will turn all warnings into errors which breaks the build. If you are working on a pull request you plan to submit to github for consideration, you should always run with this setting turned on, since it is required for all pull requests. NOTE: Putting this line into a file called "user_config.pri" in the top-level directory (same directory as `qgroundcontrol.pro`) will set this flag on all builds without interfering with the GIT history.
* Parallel builds: For non Windows builds, you can use the '-j#' option to run parellel builds.
* Location of built files: Individual build file results can be found in the `build_debug` or `build_release` directories. The built executable can be found in the `debug` or `release` directory.
* If you get this error when running qgroundcontrol: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version 'GLIBCXX_3.4.20' not found. You need to either update to the latest gcc, or install the latest libstdc++.6 using: sudo apt-get install libstdc++6.

## Additional functionality
QGroundcontrol has functionality that is dependent on the operating system and libraries installed by the user. The following sections describe these features, their dependencies, and how to disable/alter them during the build process. These features can be forcibly enabled/disabled by specifying additional values to qmake. 

### Opal-RT's RT-LAB simulator
Integration with Opal-RT's RT-LAB simulator can be enabled on Windows by installing RT-LAB 7.2.4. This allows vehicles to be simulated in RT-LAB and communicate directly with QGC on the same computer as if the UAS was actually deployed. This support is enabled by default once the requisite RT-LAB software is installed. Disabling this can be done by adding `DEFINES+=DISABLE_RTLAB` to qmake.

### XBee support
QGroundControl can talk to XBee wireless devices using their proprietary protocol directly on Windows and Linux platforms. This support is not necessary if you're not using XBee devices or aren't using their proprietary protocol. On Windows, the necessary dependencies are included in this repository and no additional steps are required. For Linux, change to the `libs/thirdParty/libxbee` folder and run `make;sudo make install` to install libxbee on your system (uninstalling can be done with a `sudo make uninstall`). `qmake` will automatically detect the library on Linux, so no other work is necessary.

To disable XBee support you may add `DEFINES+=DISABLE_XBEE` to qmake.

### Video Streaming
Check the [Video Streaming](https://github.com/mavlink/qgroundcontrol/tree/master/src/VideoStreaming) directory for further instructions.
