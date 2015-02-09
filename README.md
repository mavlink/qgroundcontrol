# QGroundControl
## Open Source Micro Air Vehicle Ground Control Station

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mavlink/qgroundcontrol?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


* Project:
<http://qgroundcontrol.org>

* Files:
<http://github.com/mavlink/qgroundcontrol>

* Credits:
<http://qgroundcontrol.org/credits>


## Documentation
For generating documentation, refer to this readme or the <http://qgroundcontrol.org> website.

## Obtaining source code
There are three ways to obtain the QGroundControl source code from our GitHub repository. You can either download a snapshot of the code in a ZIP file, clone the repository, or fork the repository if you plan to contribute to development. If you prefer one of the last two options you will need Git installed on your system; goto GitHub Help and see Set Up Git.

### Clone the Repository
This option assumes that you have Git already. To clone (checkout) the QGC repository, run the following command in the directory that you want the qgroundcontrol folder to be created:

```
git clone --recursive git://github.com/mavlink/qgroundcontrol.git
```

### Fork the Repository
If you plan to contribute to the development of QGC, you will want this option, which also requires that you have Git set up. To fork the QGC repository, do the following:

Goto GitHub Help and see Fork A Repo
Fork the QGC Repo

### Initialize submodules
After cloning or forking you will need to initialize and update the submodules using these commands in you qgroundcontrol source directory:

```
git submodule init
git submodule update
```

Each time you pull new source to your repository you should re-run "git submodule update" to get the latest submodules as well.

## Building QGroundControl
QGroundControl builds are supported for OSX, Linux and Windows. See the individual sections below for instruction on how to build on each OS. Also make sure to look at the "Additional Build Notes" section below.

### Build on Mac OSX
Supported builds are 64 bit, built using the clang compiler.

#### Install QT
- - -
1. Download Qt 5.3 from: <http://download.qt-project.org/official_releases/qt/5.3/5.3.1/qt-opensource-mac-x64-clang-5.3.1.dmg>
2. Double click the package installer and follow instructions.

#### Build QGroundControl
1. From the terminal change directory to your `groundcontrol` directory
2. Run `~/Qt5.3.1/5.3/clang_64/bin/qmake qgroundcontrol.pro -r -spec macx-clang`. If you installed a different version of Qt, or installed to a different location you may need to change the first portion of the path.
3. Run `make`

### Build on Linux
Supported builds for Linux are 32 or 64-bit, built using gcc.

#### Install Qt5.3+ and SDL1.2 prerequistites
* For Ubuntu (requires 14.10 for Qt5.3): `sudo apt-get install qtcreator qttools5-dev qtbase5-dev qt5-default qtdeclarative5-dev libqt5serialport5-dev libqt5svg5-dev libqt5webkit5-dev libsdl1.2-dev build-essential libudev-dev qml-module-qtgraphicaleffects`
* For Fedora: `sudo yum install qt-creator qt5-qtbase-devel qt5-qtdeclarative-devel qt5-qtserialport-devel qt5-qtsvg-devel qt5-qtwebkit-devel SDL-devel SDL-static systemd-devel`
* For Arch Linux: `pacman -Sy qtcreator qt5-base qt5-declarative qt5-serialport qt5-svg qt5-webkit`

##### Install Qt5.3+ from PPA
Note: Please be aware that the time of writing, Qt5.3 is unavailable in the official repositories Ubuntu 14.04/Mint 17.*. If it has become available since, please follow the instructions for installing Qt5.3 on Ubuntu.
* Add this PPA to your sources.list: `ppa:beineri/opt-qt532-trusty`
* Run the following in your terminal: `sudo apt-get update && sudo apt-get install qt53tools qt53base qt53declarative qt53serialport qt53svg qt53webkit`
* Next, set the environment variables by executing in the terminal: `source /opt/qt53/bin/qt53-env.sh` or copy and paste the contents to your `~/.profile` file to set them on login.
* Verify that the variables have been set: `echo $PATH && echo $QTDIR`. The output should read `/opt/qt53/bin:...` and `/opt/qt53`.

#### [Optional] Install additional libraries
* For text-to-speech (espeak)
	* For Ubuntu: `sudo apt-get install espeak libespeak-dev`
	* For Fedora: `sudo yum install espeak espeak-devel`
	* For Arch Linux: `pacman -Sy espeak`
* For 3D flight view (openscenegraph)
	* For Ubuntu: `sudo apt-get install libopenscenegraph-dev`
	* For Fedora: `sudo yum install OpenSceneGraph`
	* For Arch Linux: `pacman -Sy openscenegraph`

#### Build QGroundControl
1. Change directory to you `qgroundcontrol` source directory.
2. Run `qmake`
3. Run `make`

### Build on Windows
Supported builds for Windows are 32 bit only built using Visual Studio 2013 or higher.

#### Install Visual Studio Express 2013
Only compilation using Visual Studio 2013 is supported. Download and install Visual Studio Express Edition (free) from here: <http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop>. Make sure you install the Windows Desktop version.

#### Install QT
Download Qt 5.3 from here: <http://download.qt-project.org/official_releases/qt/5.3/5.3.2/qt-opensource-windows-x86-msvc2013_opengl-5.3.2.exe>
  * The Qt variant should be for VS 2013, 32 bit (not 64) and include opengl.

#### Build QGroundControl
1. Open the Qt Command Prompt program from the Start Menu
2. Change directory to your 'qgroundcontrol' source folder.
3. Run `qmake -tp vc qgroundcontrol.pro`.  This will create a 'qgroundcontrol.vcxproj' project file which is capable of building both debug and release configurations.
4. Now open the generated 'qgroundcontrol.vcxproj' file in Visual Studio.
5. Compile and edit in Visual Studio. If you need to add new files, add them to qgroundcontrol.pro and re-run qmake from step 3.

### Additional build notes for all supported OS

* Debug Builds: By default qmake will create makefiles for a release build. If you want a debug build add `CONFIG+=debug` to the command line.
* Warnings as Errors: Specifying `CONFIG+=WarningsAsErrorsOn` will turn all warnings into errors which break the build. If you are working on a pull request you plan to submit to github for consideration, you should always run with this settings turned on, since it is required for all pull requests. NOTE: Putting this line into a file called "user_config.pri" in the top-level directory will set this flag on all builds without interfering with the GIT history.
* Parallel builds: For Linux and OSX you can use the '-j#' option to run parellel builds. On Windows parallel builds are on by default.
* Location of built files: Individual build file results can be found in the `build_debug` or `build_release` directories. The built executable can be found in the `debug` or `release` directory.
* Incremental builds: Incremental builds may not be 100% reliable. If you find something strange happening that seems like a bad build. Delete your `build_debug` and `build_release` directories and run qmake again.
* Single release congfiguration on Windows: If you want to build a vs project that does not create both debug and release builds. Include `CONFIG-=debug_and_release` on the qmake command line plus either `CONFIG+=debug` or 
`CONFIG+=release` depending on the build type you want.
* Build using QtCreator: It is possible to build and debug using QtCreator as well. The above instructions should provide you with enough information to setup QtCreator on OSX and Linux. QtCreator on Windows is supposedly possible but we don't have any instructions available for that.
* Google Earth is only supported on 32 bit builds, which by default means only Windows builds.
* QGroundControl is using the Qt QLoggingCategory class for logging (<http://qt-project.org/doc/qt-5/qloggingcategory.html>). Since logging is on by default, an example qtlogging.ini file is included at the root of the repository which disables logging. Follow the instructions from Qt as to why and where to put this file. You can then edit the file to get a logging level that suits your needs.

## Additional functionality
QGroundcontrol has functionality that is dependent on the operating system and libraries installed by the user. The following sections describe these features, their dependencies, and how to disable/alter them during the build process. These features can be forcibly enabled/disabled by specifying additional values for variables either at the command line when calling `qmake` or in the `user_config.pri`. When calling `qmake` additional variables can be set using the syntax `VARIABLE="SPACE_SEPARATED_VALUES"`, which can be repeated for multiple variables. For example: `qmake DEFINES="DISABLE_QUPGRADE DISABLE_SPEECH" MAVLINK_CONF="sensesoar"` disables the QUpgrade widget, speech functionality, and sets the MAVLink dialect to sensesoar. These values can be more permanently specified by setting them in the `user_config.pri` file in the root directly. Create this file as a plain text file and ensure it ends in .pri (not .pri.txt!).

**NOTE:** Any variables specified at the command line call to `qmake` will override those set in `user_config.pri`.

### QUpgrade
QUpgrade is a submodule (a Git feature like a sub-repository) that contains extra functionality. It is compiled in by default if it has initialized and updated. It can be disabled by specifying `DISABLE_QUPGRADE` in the `DEFINES` variable.

To include QUpgrade functionality run the following (only needs to be done once after cloning the qggroundcontrol git repository):
* `git submodule init`
* `git submodule update`

The QUpgrade module relies on `libudev` on Linux platforms, so be sure to install the development version of that package.

### Specifying MAVLink dialects
The MAVLink dialect compiled by default by QGC is for the pixhawk. Setting the `MAVLINK_CONF` variable sets the dialects.

### Opal-RT's RT-LAB simulator
Integration with Opal-RT's RT-LAB simulator can be enabled on Windows by installing RT-LAB 7.2.4. This allows vehicles to be simulated in RT-LAB and communicate directly with QGC on the same computer as if the UAS was actually deployed. This support is enabled by default once the requisite RT-LAB software is installed. Disabling this can be done by adding `DISABLE_RTLAB` to the `DEFINES` variable.

### Speech syntehsis
QGroundcontrol can notify the controller of information via speech synthesis. This requires the `espeak` library on Linux. On Mac and Windows support is built in to the OS as of OS X 10.6 (Snow Leopard) and Windows Vista. This support is enabled by default on all platforms if the dependencies are met. Disabling this functionality can be done by adding `DISABLE_SPEECH` to the `DEFINES` variable.

### 3D view
The OpenSceneGraph libraries provide 3D rendering to the map overlays that QGC can provide.

OpenSceneGraph support is built-in to Mac OS X. On Linux it is commonly available through the libopenscenegraph and libopenscenegraph-qt developer packages. Windows support does not currently exist. This functionality with be automatically built if the proper libraries are installed. Disabling this feature can be done by adding `DISABLE_OPEN_SCENE_GRAPH` to the `DEFINES` variable.

### 3D mouse support
Connexion's 3D mice are supported through the 3DxWARE driver available on Linux and Windows. Download and install the driver from 3DConnexion to enable support. This support is enabled by default with driver installation. To disable add `DISABLE_3DMOUSE` to the `DEFINES` variable.

### XBee support
QGroundControl can talk to XBee wireless devices using their proprietary protocol directly on Windows and Linux platforms. This support is not necessary if you're not using XBee devices or aren't using their proprietary protocol. On Windows, the necessary dependencies are included in this repository and no additional steps are required. For Linux, change to the `libs/thirdParty/libxbee` folder and run `make;sudo make install` to install libxbee on your system (uninstalling can be done with a `sudo make uninstall`). `qmake` will automatically detect the library on Linux, so no other work is necessary.

To disable XBee support you may add `DISABLE_XBEE` to the DEFINES argument.

## Repository Layout
The following describes the directory structure and important files in the QGroundControl repository

Folders:

  * data     - Miscellaneous support files.
  * deploy   - Contains scripts for packaging QGC for all supported systems.
  * doc      - Output directory for generated Doxygen documentation. See README contained within for details.
  * files    - Contains miscellaneous data including vehicle models and autopilot-specific data files.
  * images   - UI images.
  * libs     - Library dependencies for QGC.
  * qupgrade - Source file for the qupgrade, a firmware flashing utility for the APM. Compiled into QGC by default.
  * qml      - QML source files for the project.
  * src      - Source code for QGroundControl. Split into subfolders for communications, user interface, autopilot-specific files, etc.
  * tools    - Additional tools for developers.

Important files:

  * qgroundcontrol.pro - Primary project file for building QGC. Open this in qtcreator or pass this to qmake on the command line to build QGC.
  * qgcvideo.pro       - Builds a standalone executable for viewing UDP video streams from a vehicle.
