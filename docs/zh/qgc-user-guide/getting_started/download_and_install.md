# 下载和安装

下面的部分可以用于下载每个平台的 _QGroundControl_ 的当前 [稳定版本](../releases/release_notes.md)。

:::tip
如果 _QGroundControl_ 没有在安装后启动和正常运行，请参阅[故障排除QGC配置](../troubleshooting/qgc_setup.md)！
:::

## 系统配置要求

QGC可以在任何当下流行的计算机或移动设备上正常运行。 性能表现将取决于系统环境、第三方应用程序和当前系统可使用的资源状况。
性能更强的硬件将带来更好的体验。
一台拥有至少 8GB 内存、固态硬盘、Nvidia 或 AMD 显卡以及英特尔酷睿i5或更优CPU的电脑，将适用于大多数应用场景。

为了获得最好的体验和兼容性，我们推荐您使用最新版本的操作系统。

## Windows 系统 {#windows}

Supported versions: Windows 10 (1809 or later), Windows 11:

1. 下载 [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe)。
2. 双击可执行文件来启动安装程序。

:::info
Windows 安装程序创建 3 个快捷方式：**QGroundControl**，**GPU 兼容模式**，**GPU 安全模式**。
使用第一个快捷方式，除非您遇到启动或视频渲染问题。
更多信息请见[QGC 设置故障排查 > Windows：用户界面渲染 / 视频驱动问题](../troubleshooting/qgc_setup.md#opengl_troubleshooting)。
:::

## Mac OS {#macOS}

Supported versions: macOS 12 (Monterey) or later:

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg).
2. 双击 .dmg 文件以挂载它，然后将 _QGroundControl_ 应用程序拖动到您的 _Application_ 文件夹。

:::info
QGroundControl continues to not be signed. You will not to allow permission for it to install based on your macOS version.
:::

## Ubuntu Linux 系统 {#ubuntu}

Supported versions: Ubuntu 22.04, 24.04:

Ubuntu 自带一个串口调制解调器管理器，它会干扰串口（或 USB 转串口）在任何与机器人相关方面的使用。
在安装 _QGroundControl_ 之前，您应该删除调制解调器管理器并授予自己访问串行端口的权限。
您还需要安装 _GStreamer_以支持视频流。

**Before installing _QGroundControl_ for the first time:**

1. Enable serial-port access
   Add your user to the dialout group so you can talk to USB devices without root:

```
sudo usermod -aG dialout "$(id -un)"
```

:::info
At login, your shell takes a snapshot of your user and group memberships. Because you just changed groups, you need a fresh login shell to pick up “dialout” access. Logging out and back in reloads that snapshot, so you get the new permissions.
:::

1. (Optional) Disable ModemManager
   On some Ubuntu-based systems, ModemManager can claim serial ports that QGC needs. If you don't use it elsewhere, mask or remove it.

```
# preferred: stop and mask the service
sudo systemctl mask --now ModemManager.service

# or, if you’d rather remove the package
sudo apt remove --purge modemmanager
```

1. On the command prompt, enter:

```sh
sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
sudo apt install libfuse2 -y
sudo apt install libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev -y
```

**To install _QGroundControl_:**

1. Download [QGroundControl-x86_64.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-x86_64.AppImage).

2. Make the AppImage executable

```
chmod +x QGroundControl-<arch>.AppImage
```

1. Run QGroundControl
   Either double-click the AppImage in your file manager or launch it from a terminal:

```
./QGroundControl-<arch>.AppImage
```

## Android {#android}

Supported versions: Android 9 to 15 (arm 32/64):

- [Android 32/64 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl.apk)

## 旧稳定版本

旧稳定版本可以在 <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a> 上找到。

## 每日构建

每日构建版本可以[从这里下载](../releases/daily_builds.md)。
