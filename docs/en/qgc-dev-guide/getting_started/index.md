---
qt_version: 5.15.2
---

# Getting Started with Source and Builds

This topic explains how to get the _QGroundControl_ source code and build it either natively or within a _Vagrant_ environment.
It also provides information about optional or OS specific functionality.

## Daily Builds

If you just want to test (and not debug) a recent build of _QGroundControl_ you can use the [Daily Build](../../qgc-user-guide/releases/daily_builds.md).
Versions are provided for all platforms.

## Source Code

Source code for _QGroundControl_ is kept on GitHub here: https://github.com/mavlink/qgroundcontrol.
It is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/COPYING.md).

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

_QGroundControl_ builds are supported for macOS, Linux, Windows, iOS and Android.
_QGroundControl_ uses [Qt](http://www.qt.io) as its cross-platform support library and uses [QtCreator](http://doc.qt.io/qtcreator/index.html) as its default build environment.

- **macOS:** v10.11 or higher
- **Ubuntu:** 64 bit, gcc compiler
- **Windows:** Vista or higher, [Visual Studio 2019 compiler](#vs) (64 bit)
- **iOS:** 10.0 and higher
- **Android:** Android 5.0 and later.
  - Standard QGC is built against ndk version 19.
  - Java JDK 11 is required.
- **Qt version:** {{ $frontmatter.qt_version }} **(only)** <!-- NOTE {{ $frontmatter.qt_version }}  is set in the frontmatter above -->

  ::: warning
  **Do not use any other version of Qt!**
  QGC has been thoroughly tested with the specified version of Qt ({{ $frontmatter.qt_version }}).
  There is a significant risk that other Qt versions will inject bugs that affect stability and safety (even if QGC compiles).
  :::

For more information see: [Qt 5 supported platform list](http://doc.qt.io/qt-5/supported-platforms.html).

::: info
Native [CentOS Builds](../getting_started/cent_os.md) are also supported, but are documented separately (as the tested environment is different).
:::

#### Install Visual Studio 2019 (Windows Only) {#vs}

The Windows compiler can be found here: [Visual Studio 2019 compiler](https://visualstudio.microsoft.com/vs/older-downloads/) (64 bit)

When installing, select _Desktop development with C++_ as shown:

![Visual Studio 2019 - Select Desktop Environment with C++](../../../assets/dev_getting_started/visual_studio_select_features.png)

::: info
Visual Studio is ONLY used to get the compiler. Actually building _QGroundControl_ should be done using [Qt Creator](#qt-creator) or [qmake](#qmake) as outlined below.
:::

#### Install Qt

You **need to install Qt as described below** instead of using pre-built packages from say, a Linux distribution, because _QGroundControl_ needs access to private Qt headers.

To install Qt:

1. Download and run the [Qt Online Installer](http://www.qt.io/download-open-source)
   - **Ubuntu:**
     - Set the downloaded file to executable using: `chmod +x`.
     - Install to default location for use with **./qgroundcontrol-start.sh.** If you install Qt to a non-default location you will need to modify **qgroundcontrol-start.sh** in order to run downloaded builds.
1. In the installer _Select Components_ dialog choose: {{ $frontmatter.qt_version }}.

   ::: info
   If the version needed is not displayed, check the archive (show archive and refresh).
   :::

   Then install just the following components:

   - **Windows**: _MSVC 2019 64 bit_
   - **MacOS**: _macOS_
   - **Linux**: _Desktop gcc 64-bit_
   - All:
     - _Qt Charts_ <!-- and *Qt Remote Objects (TP)* -->
     - _Android ARMv7_ (optional, used to build Android)

   ![QtCreator Select Components (Windows)](../../../assets/dev_getting_started/qt_creator_select_components.jpg)

1. Install Additional Packages (Platform Specific)

   - **Ubuntu:** `sudo apt-get install speech-dispatcher libudev-dev libsdl2-dev patchelf build-essential curl`
   - **Fedora:** `sudo dnf install speech-dispatcher SDL2-devel SDL2 systemd-devel patchelf`
   - **Arch Linux:** `pacman -Sy speech-dispatcher patchelf`
   - **Android:** [Qt Android Setup](http://doc.qt.io/qt-5/androidgs.html)

     ::: info
     JDK11 is required (install if needed)!
     :::

1. Install Optional/OS-Specific Functionality

   ::: info
   Optional features that are dependent on the operating system and user-installed libraries are linked/described below.
   These features can be forcibly enabled/disabled by specifying additional values to qmake.
   :::

   - **Video Streaming/Gstreamer:** - see [Video Streaming](https://github.com/mavlink/qgroundcontrol/blob/master/src/VideoReceiver/README.md).
   - **Airmap SDK:** - TBD.

1. Disable platform-specific optional features that are enabled (but not installed), by default.

   ::: info
   This currently applies to Airmap on Linux, which is optional but enabled by default.
   :::

   - **Ubuntu:**

     - Airmap: Create a file named **user_config.pri** (in the repo root directory) containing the text `DEFINES += DISABLE_AIRMAP`.
       This can be done in a bash terminal using the command:

       ```sh
       echo -e "DEFINES += DISABLE_AIRMAP\r\n" | tee user_config.pri
       ```

#### Building using Qt Creator {#qt-creator}

1. Launch _Qt Creator_ and open the **qgroundcontrol.pro** project.
1. In the **Projects** section, select the appropriate kit for your needs:

   - **OSX:** Desktop Qt {{ $frontmatter.qt_version }} clang 64 bit

     ::: info
     iOS builds must be built using [XCode](http://doc.qt.io/qt-5/ios-support.html).
     :::

   - **Ubuntu:** Desktop Qt {{ $frontmatter.qt_version }} GCC 64bit
   - **Windows:** Desktop Qt {{ $frontmatter.qt_version }} MSVC2019 **64bit**
   - **Android:** Android for armeabi-v7a (GCC 4.9, Qt {{ $frontmatter.qt_version }})
     - JDK11 is required.
       You can confirm it is being used by reviewing the project setting: **Projects > Manage Kits > Devices > Android (tab) > Android Settings > _JDK location_**.

1. Build using the "hammer" (or "play") icons:

   ![QtCreator Build Button](../../../assets/dev_getting_started/qt_creator_build_qgc.png)

#### Build using qmake on CLI {#qmake}

Example commands to build a default QGC and run it afterwards:

1. Make sure you cloned the repository and updated the submodules before, see chapter _Source Code_ above and switch into the repository folder:

   ```sh
   cd qgroundcontrol
   ```

1. Create and enter a shadow build directory:

   ```sh
   mkdir build
   cd build
   ```

1. Configure the build using the qmake script in the root of the repository:

   ```sh
   qmake ../
   ```

1. Run make to compile and link.
   To accelerate the process things you can use the `-j{number of threads}` parameter.

   ```sh
   make -j12
   ```

   ::: info
   You can also specify build time flags here.
   For example, you could disable airmap inclusion using the command:

   ```sh
   DEFINES+=DISABLE_AIRMAP make build
   ```

   :::

1. Run the QGroundcontrol binary that was just built:

   ```sh
   ./staging/QGroundControl
   ```

### Vagrant

[Vagrant](https://www.vagrantup.com/) can be used to build and run _QGroundControl_ within a Linux virtual machine (the build can also be run on the host machine if it is compatible).

1. [Download](https://www.vagrantup.com/downloads.html) and [Install](https://www.vagrantup.com/docs/getting-started/) Vagrant
1. From the root directory of the _QGroundControl_ repository run `vagrant up`
1. To use the graphical environment run `vagrant reload`

### Additional Build Notes for all Supported OS

- **Parallel builds:** For non Windows builds, you can use the `-j#` option to run parellel builds.
- **Location of built files:** Individual build file results can be found in the `build_debug` or `build_release` directories. The built executable can be found in the `debug` or `release` directory.
- **If you get this error when running _QGroundControl_**: `/usr/lib/x86_64-linux-gnu/libstdc++.so.6: version 'GLIBCXX_3.4.20' not found.`, you need to either update to the latest _gcc_, or install the latest _libstdc++.6_ using: `sudo apt-get install libstdc++6`.
- **Unit tests:** To run the [unit tests](../contribute/unit_tests.md), build in `debug` mode with `UNITTEST_BUILD` definition, and then copy `deploy/qgroundcontrol-start.sh` script into the `debug` directory before running the tests.

## Building QGC Installation Files

You can additionally create installation file(s) for _QGroundControl_ as part of the normal build process.

::: tip
On Windows you will need to first install [NSIS](https://sourceforge.net/projects/nsis/).
:::

To add support for installation file creation you need to add `CONFIG+=installer` to your project file, or when you call _qmake_.

To do this in _Qt Creator_:

- Open **Projects > Build > Build Steps > qmake > Additional arguments**.
- Enter `CONFIG+=installer` as shown:
  ![Installer](../../../assets/dev_getting_started/qt_project_installer.png)
