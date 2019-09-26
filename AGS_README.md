# Auterion Ground Control Station

[![Releases](https://img.shields.io/github/release/mavlink/QGroundControl.svg)](https://github.com/Auterion/QGroundControl/releases)
[![Travis Build Status](https://travis-ci.org/mavlink/qgroundcontrol.svg?branch=master)](https://travis-ci.org/Auterion/qgroundcontrol)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/crxcm4qayejuvh6c/branch/master?svg=true)](https://ci.appveyor.com/project/Auterion/qgroundcontrol)


This is the official repo for Auterion's Ground Station based on the upstream QGC: https://github.com/mavlink/qgroundcontrol

## Customization over upstream QGC

Most of the code and resources that customize uptream QGC in order to add/remove functionality are found in the `custom` folder.
The following [configuraiton code](https://github.com/Auterion/auterion-qgroundcontrol/blob/d9d8114093f5cb9c5c46a0dbd194c494f6599688/qgroundcontrol.pro#L91-L103) calls the [`custom.pri`] subproject file which replaces or adds some of the build configuration files and options to the final AGS build

## Initial development environment setup

AGS `doesn't` work with CMake generator but only with `qmake` generators. AGS didn't extend the support that QGC upstream has for using CMake to generate the makefiles or IDE specific files.

Setup the development environment for building using Qt Creator by following the QGC [Developer Guide](https://dev.qgroundcontrol.com/en/)

Additional notes:
  - Use the official QT 5.12.4 on most platforms except Windows which works only with 5.11.3

## Using the command line 

Best reference is our [`.travis.yml`](https://github.com/Auterion/auterion-qgroundcontrol/blob/d9d8114093f5cb9c5c46a0dbd194c494f6599688/.travis.yml#L168) file which builds all platform in the CI environment

Steps for building and runnig the AGS release:
```bash
mkdir build
cd build
# Generate the file make files using `qmake`
~/Qt/5.12.4/gcc_64/bin/qmake -r ../qgroundcontrol.pro CONFIG+=release
make -j`nproc --all`
# Run the build
release/qgroundcontrol-start.sh
```
Use the `qmake` `CONFIG` variable to alter the build process
 - debug mode: `CONFIG+=debug`
 - installer: `CONFIG+=installer`
