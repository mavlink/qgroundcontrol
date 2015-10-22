# QGroundControl
## Open Source Micro Air Vehicle Ground Control Station

[![Travis Build Status](https://travis-ci.org/mavlink/qgroundcontrol.svg?branch=master)](https://travis-ci.org/mavlink/qgroundcontrol)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/crxcm4qayejuvh6c/branch/master?svg=true)](https://ci.appveyor.com/project/mavlink/qgroundcontrol)

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mavlink/qgroundcontrol?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


* Project:
<http://qgroundcontrol.org>

* Files:
<http://github.com/mavlink/qgroundcontrol>

* Credits:
<http://qgroundcontrol.org/credits>


## Obtaining source code
Source code for QGroundControl is kept on GitHub: https://github.com/mavlink/qgroundcontrol.
```
git clone https://github.com/mavlink/qgroundcontrol.git
cd qgroundcontrol
git submodule init
git submodule update
```
Each time you pull new source to your repository you should re-run "git submodule update" to get the latest submodules as well.

### Supported Builds
QGroundControl builds are supported for OSX, Linux, Windows and Android. QGroundControl uses [Qt](http://www.qt.io) as it's cross-platform support library and uses [QtCreator](http://doc.qt.io/qtcreator/index.html) as it's default build environment.
* OSX: 64 bit, clang compiler
* Ubuntu: 64 bit, gcc compiler
* Windows: 32 bit, [Visual Studio 2013 compiler](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop)
* Android: Jelly Bean (4.1) and higher
* Qt version: 5.5.1 (or higher)

#### Install QT

* Download the [Qt installer](http://www.qt.io/download-open-source)
    * Ubuntu: Set the downloaded file to executable using:`chmod +x`
    * Windows: Default installer not quite correct, use [this](http://download.qt.io/official_releases/qt/5.5/5.5.1/qt-opensource-windows-x86-msvc2013-5.5.1.exe) instead



#### Install additional packages:
    * Ubuntu: `sudo apt-get install espeak libespeak-dev libudev-dev libsdl1.2-dev`
    * Fedora: `sudo yum install espeak espeak-devel SDL-devel SDL-static systemd-devel`
    * Arch Linux: `pacman -Sy espeak`
    * Windows: [USB driver to connect to Pixhawk/PX4Flow/3DR Radio](http://www.pixhawk.org/firmware/downloads)

#### Building using Qt Creator
* Launch Qt Creator and open the `qgroundcontrol.pro` project.
* Select the appropriate kit for your needs:
    * OSX: Desktop Qt 5.5.1 clang 64 bit
    * Ubuntu: <tbd>
    * Windows: Desktop Qt 5.5.1 MSVC2013 32bit
    * Android: Android for armeabi-v7a (GCC 4.9, Qt 5.5.1)

### Additional build notes for all supported OS

* Warnings as Errors: Specifying `CONFIG+=WarningsAsErrorsOn` will turn all warnings into errors which break the build. If you are working on a pull request you plan to submit to github for consideration, you should always run with this settings turned on, since it is required for all pull requests. NOTE: Putting this line into a file called "user_config.pri" in the top-level directory will set this flag on all builds without interfering with the GIT history.
* Parallel builds: You can use the '-j#' option to run parellel builds.
* Location of built files: Individual build file results can be found in the `build_debug` or `build_release` directories. The built executable can be found in the `debug` or `release` directory.
* If you get this error when running qgroundcontrol: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version 'GLIBCXX_3.4.20' not found. You need to either update to the latest gcc, or install the latest libstdc++.6 using: sudo apt-get install libstdc++6.

## Additional functionality
QGroundcontrol has functionality that is dependent on the operating system and libraries installed by the user. The following sections describe these features, their dependencies, and how to disable/alter them during the build process. These features can be forcibly enabled/disabled by specifying additional values for variables either at the command line when calling `qmake` or in the `user_config.pri`. When calling `qmake` additional variables can be set using the syntax `VARIABLE="SPACE_SEPARATED_VALUES"`, which can be repeated for multiple variables. For example: `qmake DEFINES="DISABLE_SPEECH"` disables the QUpgrade widget, speech functionality, and sets the MAVLink dialect to sensesoar. These values can be more permanently specified by setting them in the `user_config.pri` file in the root directly. Create this file as a plain text file and ensure it ends in .pri (not .pri.txt!).

**NOTE:** Any variables specified at the command line call to `qmake` will override those set in `user_config.pri`.

### Opal-RT's RT-LAB simulator
Integration with Opal-RT's RT-LAB simulator can be enabled on Windows by installing RT-LAB 7.2.4. This allows vehicles to be simulated in RT-LAB and communicate directly with QGC on the same computer as if the UAS was actually deployed. This support is enabled by default once the requisite RT-LAB software is installed. Disabling this can be done by adding `DISABLE_RTLAB` to the `DEFINES` variable.

### Speech syntehsis
QGroundcontrol can notify the controller of information via speech synthesis. This requires the `espeak` library on Linux. On Mac and Windows support is built in to the OS as of OS X 10.6 (Snow Leopard) and Windows Vista. This support is enabled by default on all platforms if the dependencies are met. Disabling this functionality can be done by adding `DISABLE_SPEECH` to the `DEFINES` variable.

### 3D mouse support
Connexion's 3D mice are supported through the 3DxWARE driver available on Linux and Windows. Download and install the driver from 3DConnexion to enable support. This support is enabled by default with driver installation. To disable add `DISABLE_3DMOUSE` to the `DEFINES` variable.

### XBee support
QGroundControl can talk to XBee wireless devices using their proprietary protocol directly on Windows and Linux platforms. This support is not necessary if you're not using XBee devices or aren't using their proprietary protocol. On Windows, the necessary dependencies are included in this repository and no additional steps are required. For Linux, change to the `libs/thirdParty/libxbee` folder and run `make;sudo make install` to install libxbee on your system (uninstalling can be done with a `sudo make uninstall`). `qmake` will automatically detect the library on Linux, so no other work is necessary.

To disable XBee support you may add `DISABLE_XBEE` to the DEFINES argument.
