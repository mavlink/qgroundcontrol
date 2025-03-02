---
qt_version: 6.8.2
---

# Getting Started with Source and Builds

This topic explains how to get the _QGroundControl_ source code and build it either natively or within a _Vagrant_ environment.
It also provides information about optional or OS specific functionality.

## Daily Builds

If you just want to test (and not debug) a recent build of _QGroundControl_ you can use the [Daily Build](../../qgc-user-guide/releases/daily_builds.md).
Versions are provided for all platforms.

## Source Code

Source code for _QGroundControl_ is kept on GitHub here: https://github.com/mavlink/qgroundcontrol.
It is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md).

To get the source files:

1. Clone the repo (or your fork) including submodules:

  ```sh
  git clone --recursive -j8 https://github.com/mavlink/qgroundcontrol.git
  ```

2. Update submodules (required each time you pull new source code):

  ```sh
  git submodule update --recursive
  ```

:::tip
Github source-code zip files cannot be used because these do not contain the appropriate submodule source code.
You must use git!
:::

## Build QGroundControl

### Using Containers

We support Linux builds using a container found on the source tree of the repository, which can help you develop and deploy the QGC apps without having to install any of the requirements on your local environment.

[Container Guide](../getting_started/container.md)

### Native Builds

_QGroundControl_ builds are supported for macOS, Linux, Windows, and Android. Creating a version of QGC for iOS is theoretically possible but is no longer supported as a standard build.
_QGroundControl_ uses [Qt](http://www.qt.io) as its cross-platform support library.

The required version of Qt is {{ $frontmatter.qt_version }} **(only)**.

::: warning
**Do not use any other version of Qt!**
QGC has been thoroughly tested with the specified version of Qt ({{ $frontmatter.qt_version }}).
There is a significant risk that other Qt versions will inject bugs that affect stability and safety (even if QGC compiles).
:::

For more information see: [Qt 6 supported platform list](https://doc.qt.io/qt-6/supported-platforms.html).

#### Install Qt

You **must install Qt as described below** instead of using pre-built packages from say, a Linux distribution.

To install Qt:

1. Download and run the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss)
  - **Ubuntu:**
    - Set the downloaded file to executable using: `chmod +x`.
    - You may also need to install libxcb-cursor0

2. On the _Installation Folder_ page select "Custom Installation"

3. On the _Select Components_ page:

  - I you don't see _Qt {{ $frontmatter.qt_version }}_ listed check the _Archive_ checkbox and click _Filter_.

- Under Qt -> _Qt {{ $frontmatter.qt_version }}_ select:
  - **Windows**: MSVC 2022 _arch_ - where _arch_ is the architecture of your machine
  - **Mac**: Desktop
  - **Linux**: Desktop gcc 64-bit
  - **Android**: Android
- Select all _Additional Libraries_
- Deselect QT Design Studio

1. Install Additional Packages (Platform Specific)

  - **Ubuntu:** `sudo bash ./qgroundcontrol/tools/setup/install-dependencies-debian.sh`
  - **Fedora:** `sudo dnf install speech-dispatcher SDL2-devel SDL2 systemd-devel patchelf`
  - **Arch Linux:** `pacman -Sy speech-dispatcher patchelf`
  - **Mac** `sh qgroundcontrol/tools/setup/macos-dependencies.sh`
  - **Android** [Setup](https://doc.qt.io/qt-6/android-getting-started.html). JDK17 is required for the latest updated versions. NDK Version: 25.1.8937393
    You can confirm it is being used by reviewing the project setting: **Projects > Manage Kits > Devices > Android (tab) > Android Settings > _JDK location_**.
    Note: Visit here for more detailed configurations [android.yml](.github/workflows/android.yml)

2. Install Optional/OS-Specific Functionality

  ::: info
  Optional features that are dependent on the operating system and user-installed libraries are linked/described below.
  These features can be forcibly enabled/disabled by specifying additional values to qmake.
  :::

  - **Video Streaming/Gstreamer:** - see [Video Streaming](https://github.com/mavlink/qgroundcontrol/blob/master/src/VideoManager/VideoReceiver/GStreamer/README.md)

#### Install Visual Studio (Windows Only) {#vs}

Install [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/).

When installing, select _Desktop development with C++_ as shown:

![Visual Studio 2019 - Select Desktop Environment with C++](../../../assets/dev_getting_started/visual_studio_select_features.png)

:::info
Visual Studio is ONLY used to get the compiler. Building _QGroundControl_ is done using [Qt Creator](#qt-creator) or [cmake](#cmake) directly as outlined below.
:::

#### Building using Qt Creator {#qt-creator}

1. Launch _Qt Creator_, select Open Project and select the **CMakeLists.txt** file.

2. On the _Configure Project_ page it should default to the version of Qt you just installed using the instruction above. If not select that kit from the list and click _Configure Project_.

3. Build using the "hammer" (or "play") icons or the menus:

  ![QtCreator Build Button](../../../assets/dev_getting_started/qt_creator_build_qgc.png)

#### Build using CMake on CLI {#cmake}

Example commands to build a default QGC and run it afterwards:

1. Make sure you cloned the repository and updated the submodules before, see chapter _Source Code_ above and switch into the repository folder:

  ```sh
  cd qgroundcontrol
  ```

2. Configure:

  ```sh
  ~/Qt/6.8.2/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
  ```

  Change the directory for qt-cmake to match your install location for Qt and the kit you want to use.

3. Build

  ```sh
  cmake --build build --config Debug
  ```

4. Run the QGroundcontrol binary that was just built:

  ```sh
  ./build/Debug/QGroundControl
  ```

### Vagrant

[Vagrant](https://www.vagrantup.com/) can be used to build and run _QGroundControl_ within a Linux virtual machine (the build can also be run on the host machine if it is compatible).

1. [Download](https://www.vagrantup.com/downloads.html) and [Install](https://www.vagrantup.com/docs/getting-started/) Vagrant
2. From the root directory of the _QGroundControl_ repository run `vagrant up`
3. To use the graphical environment run `vagrant reload`

### Additional Build Notes for all Supported OS

- **Parallel builds:** For non Windows builds, you can use the `-j#` option to run parellel builds.
- **If you get this error when running _QGroundControl_**: `/usr/lib/x86_64-linux-gnu/libstdc++.so.6: version 'GLIBCXX_3.4.20' not found.`, you need to either update to the latest _gcc_, or install the latest _libstdc++.6_ using: `sudo apt-get install libstdc++6`.
- **Unit tests:** To run the [unit tests](../contribute/unit_tests.md), build in `debug` mode with `QGC_UNITTEST_BUILD` definition, and then copy `deploy/qgroundcontrol-start.sh` script into the `debug` directory before running the tests.

## Building QGC Installation Files

You can additionally create installation file(s) for _QGroundControl_ as part of the normal build process.

```sh
cmake --install . --config Release
```
