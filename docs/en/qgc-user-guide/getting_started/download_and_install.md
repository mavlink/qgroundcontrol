# Download and Install

The sections below can be used to download the [current stable release](../releases/release_notes.md) of _QGroundControl_ for each platform.

:::tip
See [Troubleshooting QGC Setup](../troubleshooting/qgc_setup.md) if _QGroundControl_ doesn't start and run properly after installation!
:::

## System Requirements

QGC should run well on any modern computer or mobile device. Performance will depend on the system environment, 3rd party applications, and available system resources.
More capable hardware will provide a better experience.
A computer with at least 8Gb RAM, an SSD, Nvidia or AMD graphics and an i5 or better CPU will be suitable for most applications.

For the best experience and compatibility, we recommend you the newest version of your operating system.

## Windows {#windows}

_QGroundControl_ can be installed on 64 bit versions of Windows:

1. Download [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe).
1. Double click the executable to launch the installer.

::: info
The Windows installer creates 3 shortcuts: **QGroundControl**, **GPU Compatibility Mode**, **GPU Safe Mode**.
Use the first shortcut unless you experience startup or video rendering issues.
For more information see [Troubleshooting QGC Setup > Windows: UI Rendering/Video Driver Issues](../troubleshooting/qgc_setup.md#opengl_troubleshooting).
:::

::: info
Prebuilt _QGroundControl_ versions from 4.0 onwards are 64-bit only.
It is possible to manually build 32 bit versions (this is not supported by the dev team).
:::

## Mac OS X {#macOS}

_QGroundControl_ can be installed on macOS 10.11 or later:

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->
<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg).
1. Double-click the .dmg file to mount it, then drag the _QGroundControl_ application to your _Application_ folder.

::: info
QGroundControl continues to not be signed which causes problem on Catalina. To open QGC app for the first time:

- Right-click the QGC app icon, select Open from the menu. You will only be presented with an option to Cancel. Select Cancel.
- Right-click the QGC app icon again, Open from the menu. This time you will be presented with the option to Open.
  :::

## Flatpak {#flatpak}

_QGroundControl_ can be installed/run on linux distributions via [flatpak](https://flathub.org/apps/org.mavlink.qgroundcontrol)
 (Mint Linux, Debian, Ubuntu, Fedora, Suse, etc...)

## Ubuntu Linux {#ubuntu}

_QGroundControl_ can be installed/run on Ubuntu LTS 20.04 (and later).

Ubuntu comes with a serial modem manager that interferes with any robotics related use of a serial port (or USB serial).
Before installing _QGroundControl_ you should remove the modem manager and grant yourself permissions to access the serial port.
You also need to install _GStreamer_ in order to support video streaming.

Before installing _QGroundControl_ for the first time:

1. On the command prompt enter:
   ```sh
   sudo usermod -a -G dialout $USER
   sudo apt-get remove modemmanager -y
   sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
   sudo apt install libfuse2 -y
   sudo apt install libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev -y
   ```
   <!-- Note, remove install of libqt5gui5 https://github.com/mavlink/qgroundcontrol/issues/10176 fixed -->
1. Logout and login again to enable the change to user permissions.

&nbsp;
To install _QGroundControl_:

1. Download [QGroundControl.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.AppImage).
1. Install (and run) using the terminal commands:
   ```sh
   chmod +x ./QGroundControl.AppImage
   ./QGroundControl.AppImage  (or double click)
   ```

::: info
There are known [video steaming issues](../troubleshooting/qgc_setup.md#dual_vga) on Ubuntu 18.04 systems with dual adaptors.
:::

::: info
Prebuilt _QGroundControl_ versions from 4.0 cannot run on Ubuntu 16.04.
To run these versions on Ubuntu 16.04 you can [build QGroundControl from source without video libraries](https://dev.qgroundcontrol.com/en/getting_started/).
:::

## Android {#android}

_QGroundControl_ is temporily unavailable from the Google Play Store. You can install manually from here:

- [Android 32 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl32.apk)
- [Android 64 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl64.apk)

## Old Stable Releases

Old stable releases can be found on <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>.

## Daily Builds

Daily builds can be [downloaded from here](../releases/daily_builds.md).
