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

Supported versions: Windows 10 (1809 or later), Windows 11:

1. Download [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe).
1. Double click the executable to launch the installer.

::: info
The Windows installer creates 3 shortcuts: **QGroundControl**, **GPU Compatibility Mode**, **GPU Safe Mode**.
Use the first shortcut unless you experience startup or video rendering issues.
For more information see [Troubleshooting QGC Setup > Windows: UI Rendering/Video Driver Issues](../troubleshooting/qgc_setup.md#opengl_troubleshooting).
:::

## Mac OS {#macOS}

Supported versions: macOS 12 (Monterey) or later:

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->
<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg).
1. Double-click the .dmg file to mount it, then drag the _QGroundControl_ application to your _Application_ folder.

::: info
QGroundControl continues to not be signed. You will not to allow permission for it to install based on your macOS version.
:::

## Ubuntu Linux {#ubuntu}

Supported versions: Ubuntu 22.04, 24.04:

Ubuntu comes with a serial modem manager that interferes with any robotics related use of a serial port (or USB serial).
Before installing _QGroundControl_ you should remove the modem manager and grant yourself permissions to access the serial port.
You also need to install _GStreamer_ in order to support video streaming.

**Before installing _QGroundControl_ for the first time:**

1. Enable serial-port access
Add your user to the dialout group so you can talk to USB devices without root:

```
sudo usermod -aG dialout "$(id -un)"
```

::: info
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

1. Make the AppImage executable
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

## Old Stable Releases

Old stable releases can be found on <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>.

## Daily Builds

Daily builds can be [downloaded from here](../releases/daily_builds.md).
