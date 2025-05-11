# 下载和安装

The sections below can be used to download the [current stable release](../releases/release_notes.md) of _QGroundControl_ for each platform.

:::tip
See [Troubleshooting QGC Setup](../troubleshooting/qgc_setup.md) if _QGroundControl_ doesn't start and run properly after installation!
:::

## 系统配置要求

QGC可以在任何当下流行的计算机或移动设备上正常运行。 性能表现将取决于系统环境、第三方应用程序和当前系统可使用的资源状况。
性能更强的硬件将带来更好的体验。
一台拥有至少 8GB 内存、固态硬盘、Nvidia 或 AMD 显卡以及英特尔酷睿i5或更优CPU的电脑，将适用于大多数应用场景。

为了获得最好的体验和兼容性，我们推荐您使用最新版本的操作系统。

## Windows 系统 {#windows}

_QGroundControl_ can be installed on 64 bit versions of Windows:

1. Download [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe).
2. 双击可执行文件来启动安装程序。

:::info
Windows 安装程序创建 3 个快捷方式：**QGroundControl**，**GPU 兼容模式**，**GPU 安全模式**。
使用第一个快捷方式，除非您遇到启动或视频渲染问题。
更多信息请见[QGC 设置故障排查 > Windows：用户界面渲染 / 视频驱动问题](../troubleshooting/qgc_setup.md#opengl_troubleshooting)。
:::

:::info
Prebuilt _QGroundControl_ versions from 4.0 onwards are 64-bit only.
It is possible to manually build 32 bit versions (this is not supported by the dev team).
:::

## Mac OS X 系统 {#macOS}

_QGroundControl_ 可安装在 macOS 10.11 或更高版本上：

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg).
2. 双击 .dmg 文件以挂载它，然后将 _QGroundControl_ 应用程序拖动到您的 _Application_ 文件夹。

::: info
QGroundControl continues to not be signed which causes problem on Catalina. To open QGC app for the first time:

- 右键点击 QGC 应用图标，从菜单中选择 Open。 届时你只会看到一个“取消”选项。 选择取消。
- 再次右键点击QGC 应用图标，从菜单中选择Open。 这次您会发现有 Open的选项了。 这次您会发现有 Open 的选项了。
  :::

## Ubuntu Linux 系统 {#ubuntu}

_QGroundControl_ can be installed/run on Ubuntu LTS 22.04 (and later).

Ubuntu 自带一个串口调制解调器管理器，它会干扰串口（或 USB 转串口）在任何与机器人相关方面的使用。
在安装 _QGroundControl_ 之前，您应该删除调制解调器管理器并授予自己访问串行端口的权限。
您还需要安装 _GStreamer_以支持视频流。

在首次安装 _QGroundControl_ 之前：

1. 在命令提示符下输入：
  ```sh
  sudo usermod -a -G dialout $USER
  sudo apt-get remove modemmanager -y
  sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
  sudo apt install libfuse2 -y
  sudo apt install libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev -y
  ```
  <!-- Note, remove install of libqt5gui5 https://github.com/mavlink/qgroundcontrol/issues/10176 fixed -->
2. 注销并重新登录以启用对用户权限的更改。

&nbsp; 若要安装 _QGroundControl_：

1. 下载 [QGroundControl.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.AppImage)。
2. 使用终端命令安装(并运行)：
  ```sh
  chmod +x ./QGroundControl.AppImage
  ./QGroundControl.AppImage  (或双击)
  ```

:::info
在配备双适配器的 Ubuntu 18.04 系统上，存在已知的[视频流问题](../troubleshooting/qgc_setup.md#dual_vga) 。
:::

:::info
4.0 及以上版本的预构建 _QGroundControl_ 无法在 Ubuntu 16.04 上运行。
若要在 Ubuntu 16.04 上运行这些版本，您可以[从源代码构建QGroundControl，无需视频库](https://dev.qgroundcontrol.com/en/getting_started/)。
:::

## Android {#android}

- [Android 32 位 APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl32.apk)
- [Android 64 位 APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl64.apk)

## 旧稳定版本

旧稳定版本可以在 <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a> 上找到。

## 每日构建

每日构建版本可以[从这里下载](../releases/daily_builds.md)。
