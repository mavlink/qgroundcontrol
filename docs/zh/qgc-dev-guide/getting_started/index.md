---
qt_version: 6.8.2
---

# Getting Started with Source and Builds

本主题说明如何获取QGroundControl源代码并在本机或在Vagrant(虚拟机)环境中构建它。 本主题还提供其他可选功能信息及特定于操作系统的功能信息。
It also provides information about optional or OS specific functionality.

## 每日构建

If you just want to test (and not debug) a recent build of _QGroundControl_ you can use the [Daily Build](../../qgc-user-guide/releases/daily_builds.md).
Versions are provided for all platforms.

## 源代码

_QGroundControl_ 的源代码保存在 github 上，下载地址为: https://github.com/mavlink/qgroundcontrol。 QGroundControl源代码在Apache 2.0和GPLv3下是双许可的。 有关更多信息，请参阅：许可证。
It is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md).

要获取源文件, 请执行以下操作:

1. Clone the repo (or your fork) including submodules:

  ```sh
  克隆存储库 (或您的分叉), 包括子模块: `git clone --recursive -j8 https://github.com/mavlink/qgroundcontrol.git`
  ```

2. Update submodules (required each time you pull new source code):

  ```sh
  2.更新子模块（每次拉新源代码时都这样做）： `git submodule update --recursive`
  ```

:::tip
提示：不能使用Github以zip形式下载源文件，因为zip压缩包中不包含相应的子模块源代码。 你必须使用git工具！
You must use git!
:::

## 构建QGroundControl开发环境

### Using Containers

We support Linux builds using a container found on the source tree of the repository, which can help you develop and deploy the QGC apps without having to install any of the requirements on your local environment.

[Container Guide](../getting_started/container.md)

### Native Builds

_QGroundControl_ builds are supported for macOS, Linux, Windows, and Android. Creating a version of QGC for iOS is theoretically possible but is no longer supported as a standard build.
_QGroundControl_ uses [Qt](http://www.qt.io) as its cross-platform support library.

The required version of Qt is {{ $frontmatter.qt_version }} **(only)**.

:::warning
**Do not use any other version of Qt!** QGC has been thoroughly tested with the specified version of Qt ({{ $frontmatter.qt_version }}).
There is a significant risk that other Qt versions will inject bugs that affect stability and safety (even if QGC compiles).
:::

For more information see: [Qt 6 supported platform list](https://doc.qt.io/qt-6/supported-platforms.html).

#### 安装Qt

You **must install Qt as described below** instead of using pre-built packages from say, a Linux distribution.

To install Qt:

1. Download and run the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss)
  - **Ubuntu:**
    - 使用以下命令将下载的文件设置为可执行文件：`chmod + x`
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

1. Make sure you cloned the repository and updated the submodules before, see chapter _Source Code_ above and switch into the repository folder: `cd qgroundcontrol`

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

4. Run the QGroundcontrol binary that was just built: `./staging/QGroundControl`

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
