# QGroundControl
## Open Source Micro Air Vehicle Ground Control Station


* Project:
<http://qgroundcontrol.org>

* Files:
<http://github.com/mavlink/qgroundcontrol>

* Credits:
<http://qgroundcontrol.org/credits>


## Documentation
For generating documentation, refer to /doc/README.

## Notes
Please make sure to delete your build folder before re-building. Independent of which
build system you use (this is not related to Qt or your OS) the dependency checking and
cleaning is based on the current project revision. So if you change the project and don't remove the build folder before your next build, incremental building can leave you with stale object files.

## Additional functionality
QGroundcontrol has functionality that is dependent on the operating system and libraries installed by the user. The following sections describe these features, their dependencies, and how to disable/alter them during the build process. These features can be forcibly enabled/disabled by specifying additional values for variables either at the command line when calling `qmake` or in the `user_config.pri`. When calling `qmake` additional variables can be set using the syntax `VARIABLE="SPACE_SEPARATED_VALUES"`, which can be repeated for multiple variables. For example: `qmake DEFINES="DISABLE_QUPGRADE DISABLE_SPEECH" MAVLINK_CONF="sensesoar"` disables the QUpgrade widget, speech functionality, and sets the MAVLink dialect to sensesoar. These values can be more permanently specified by setting them in the `user_config.pri` file in the root directly. Copy the `user_config.pri.dist` file and name the copy `user_config.pri`, uncommenting the lines with the variables to modify and set their values as you desire.

**NOTE:** Any variables specified at the command line call to `qmake` will override those set in `user_config.pri`.

### QUpgrade
QUpgrade is a submodule (a Git feature like a sub-repository) that contains extra functionality. It is compiled in by default if it has initialized and updated. It can be disabled by specifying `DISABLE_QUPGRADE` in the `DEFINES` variable.

To include QUpgrade functionality run the following (only needs to be done once after cloning the qggroundcontrol git repository):
  * `git submodule init`
  * `git submodule update`

The QUpgrade module relies on `libudev` on Linux platforms, so be sure to install the development version of that package.

### Specifying MAVLink dialects
The MAVLink dialect compiled by default by QGC is for the ardupilotmega. This will happen if no other dialects are specified. Setting the `MAVLINK_CONF` variable sets the dialects, with more than one specified in a space-separated list. Note that doing this may result in compilation errors as certain dialects may conflict with each other!

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

# Build on Mac OSX

To build on Mac OSX (10.6 or later):
- - -
### Install SDL

1. Download SDL from:  <http://www.libsdl.org/release/SDL-1.2.14.dmg>
2. From the SDL disk image, copy the `sdl.framework` bundle to `/Library/Frameworks` directory (if you are not an admin copy to `~/Library/Frameworks`)

### Install QT
- - -
1. Download Qt 4.8+ from: <http://download.qt-project.org/official_releases/qt/4.8/4.8.5/qt-mac-opensource-4.8.5.dmg>
2. Double click the package installer and follow instructions.
3. If you are building on Mavericks or later you will need to modify /Library/Frameworks/QtCore.framework/Versions/4/Headers/qglobal.h to add the new 10.9 os version number. Search for MAC_OS_X_VERSION_10_8 to find the right spot.

### Build QGroundControl
- - -
 (use clang compiler - not gcc)

1. From the terminal go to the `groundcontrol` directory
2. Run `qmake qgroundcontrol.pro -r -spec unsupported/macx-clang CONFIG+=x86_64`
3. Run `make -j4`


# Build on Linux


To build on Linux:
- - -
1. Install base dependencies (QT + phonon/webkit, SDL)
  * For Ubuntu: `sudo apt-get install libqt4-dev libphonon-dev libphonon4 phonon-backend-gstreamer qtcreator libsdl1.2-dev build-essential libudev-dev`
  * For Fedora: `sudo yum install qt qt-creator qt-webkit-devel phonon-devel SDL-devel SDL-static systemd-devel`
  * For Arch Linux: `pacman -Sy qtwebkit phonon-qt4`

2. **[OPTIONAL]** Install additional libraries
  * For text-to-speech (espeak)
	* For Ubuntu: `sudo apt-get install espeak libespeak-dev`
	* For Fedora: `sudo yum install espeak espeak-devel`
	* For Arch Linux: `pacman -Sy espeak`
  * For 3D flight view (openscenegraph)
	* For Ubuntu: `sudo apt-get install libopenscenegraph-dev`
	* For Fedora: `sudo yum install OpenSceneGraph-qt-devel`

3. Clone the repository
  1. `cd PROJECTS_DIRECTORY`
  2. git clone https://github.com/mavlink/qgroundcontrol.git
  3. **[OPTIONAL]** For QUpgrade integration:
	1. `cd qgroundcontrol`
	2. `git submodule init`
	3. `git submodule update`

4. **[OPTIONAL]** Build and install XBee support:
  1. ` cd libs/thirdParty/libxbee`
  2. `make`
  3. `sudo make install`

5. Build QGroundControl:
  1. Go back to root qgroundcontrol directory
  2. `qmake`
  3. `make`
	* To enable parallel compilation add the `-j` argument with the number of cores you have. So on a quad-core processor: `make -j4`

6. Run qgroundcontrol
  1. `./release/qgroundcontrol`

# Build on Windows
- - -

Steps for Visual Studio 2010:

1. Download and install Visual Studio 2010 Express Edition (free) from here: <http://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx>. Make sure to install VS 2010 SP1 as well to fix a linking error. Download from here: <http://www.microsoft.com/en-us/download/details.aspx?id=23691>.

2. Download and install the Qt libraries for Windows (VS 2010 version) from here: <http://download.qt-project.org/official_releases/qt/4.8/4.8.5/qt-win-opensource-4.8.5-vs2010.exe>

3. Go to the QGroundControl folder and then to thirdParty/libxbee and build it following the instructions in win32.README

4. Open the Qt Command Prompt program (should be in the Start Menu), navigate to the source folder of QGroundControl and create the Visual Studio project by typing `qmake -tp vc qgroundcontrol.pro`

5. Now start Visual Studio 2010 and load qgroundcontrol.vcxproj.

6. Compile and edit in Visual Studio. If you need to add new files, add them to qgroundcontrol.pro and re-run `qmake -tp vc qgroundcontrol.pro`

7. If you already have VS 2008 that works as well. Download the appropriate Qt from here: <http://download.qt-project.org/official_releases/qt/4.8/4.8.5/qt-win-opensource-4.8.5-vs2008.exe>. And use qgroundcontrol.vcproj instead.


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
