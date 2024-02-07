# 下载和安装

The sections below can be used to download the [current stable release](../releases/release_notes.md) of _QGroundControl_ for each platform.

:::tip
See [Troubleshooting QGC Setup](../troubleshooting/qgc_setup.md) if _QGroundControl_ doesn't start and run properly after installation!
:::

## 系统配置要求

QGC should run well on any modern computer or mobile device. Performance will depend on the system environment, 3rd party applications, and available system resources.
More capable hardware will provide a better experience.
A computer with at least 8Gb RAM, an SSD, Nvidia or AMD graphics and an i5 or better CPU will be suitable for most applications.

为了获得最好的体验和兼容性，我们推荐您使用最新版本的操作系统。

## Windows 系统 {#windows}

_QGroundControl_ can be installed on 64 bit versions of Windows:

1. Download [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe).
2. 双击可执行文件来启动安装程序。

:::info
The Windows installer creates 3 shortcuts: **QGroundControl**, **GPU Compatibility Mode**, **GPU Safe Mode**.
Use the first shortcut unless you experience startup or video rendering issues.
For more information see [Troubleshooting QGC Setup > Windows: UI Rendering/Video Driver Issues](../troubleshooting/qgc_setup.md#opengl_troubleshooting).
:::

:::info
Prebuilt _QGroundControl_ versions from 4.0 onwards are 64-bit only.
It is possible to manually build 32 bit versions (this is not supported by the dev team).
:::

## Mac OS X 系统 {#macOS}

_QGroundControl_ can be installed on macOS 10.11 or later: <!-- match version using https://dev.qgroundcontrol.com/master/en/getting_started/#native-builds -->

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg).
2. 双击.dmg 文件以挂载它，然后将_QGroundControl_应用程序拖动到您的_应用程序_文件夹。

::: info
QGroundControl continues to not be signed which causes problem on Catalina. To open QGC app for the first time:

- 右键点击QGC 应用图标，从菜单中选择Open 您只有一个选项，就是Cancel 选择 Cancel。 You will only be presented with an option to Cancel. Select Cancel.
- 再次右键点击QGC 应用图标，从菜单中选择Open。 这次您会发现有 Open的选项了。 This time you will be presented with the option to Open.
  :::

## Ubuntu Linux 系统 {#ubuntu}

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
   sudo apt install libqt5gui5 -y
   sudo apt install libfuse2 -y
   ```
   <!-- Note, remove install of libqt5gui5 https://github.com/mavlink/qgroundcontrol/issues/10176 fixed -->
2. Logout and login again to enable the change to user permissions.

&nbsp; To install _QGroundControl_:

1. Download [QGroundControl.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.AppImage).
2. 使用终端命令安装(并运行)：
   sh
   chmod +x ./QGroundControl.AppImage
   ./QGroundControl.AppImage (or double click)
   ```sh
   **注意** QGroundControlControl在Catalina系统上如果没有被签名认证，会有些问题发生。 当您首次打开 QGC 应用：
   ```

There are known [video steaming issues](../troubleshooting/qgc_setup.md#dual_vga) on Ubuntu 18.04 systems with dual adaptors.
:::

:::info
Prebuilt _QGroundControl_ versions from 4.0 cannot run on Ubuntu 16.04.
To run these versions on Ubuntu 16.04 you can [build QGroundControl from source without video libraries](https://dev.qgroundcontrol.com/en/getting_started/).
:::

## Android {#android}

_QGroundControl_ is temporily unavailable from the Google Play Store. You can install manually from here:

- [Android 32 位 APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl32.apk)
- [Android 64 位 APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl64.apk)

## Old Stable Releases

Old stable releases can be found on <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>.

## Daily Builds

Daily builds can be [downloaded from here](../releases/daily_builds.md).
