# 每日构建

每日生成的 _QGroundControl_ 有绝对最新的 [新功能](../releases/daily_build_new_features.md)。

:::warning
每日构建版本的测试力度比稳定版本小。
Use at your own risk.
:::

这些可从以下链接下载（按照 [下载与安装](../getting_started/download_and_install.md) 中所述进行安装）：

- Windows
  - [x86_64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer-AMD64.exe)
  - [Arm_64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer-ARM64.exe)
- [OS X](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.dmg)
- Linux - (See installation instructions below)
  - [Linux x86_64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-x86_64.AppImage)
  - [Linux aarch64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-aarch64.AppImage)
- [Android](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.apk)
- iOS is currently unavailable

## Linux Installation Instructions

1. Make the AppImage executable

```
chmod +x QGroundControl-<arch>.AppImage
```

2. Enable serial-port access
   Add your user to the dialout group so you can talk to USB devices without root:

```
sudo usermod -aG dialout "$(id -un)"
```

:::info
At login, your shell takes a snapshot of your user and group memberships. Because you just changed groups, you need a fresh login shell to pick up “dialout” access. Logging out and back in reloads that snapshot, so you get the new permissions.
:::

3. (Optional) Disable ModemManager
   On some Ubuntu-based systems, ModemManager can claim serial ports that QGC needs. If you don't use it elsewhere, mask or remove it.

```
# preferred: stop and mask the service
sudo systemctl mask --now ModemManager.service

# or, if you’d rather remove the package
sudo apt remove --purge modemmanager
```

4. Run QGroundControl
   Either double-click the AppImage in your file manager or launch it from a terminal:

```
./QGroundControl-<arch>.AppImage
```
