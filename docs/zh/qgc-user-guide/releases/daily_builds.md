# 每日构建

每日生成的 _QGroundControl_ 有绝对最新的 [新功能](../releases/daily_build_new_features.md)。

:::warning
每日构建版本的测试力度比稳定版本小。
请自行承担使用风险！
:::

这些可从以下链接下载（按照 [下载与安装](../getting_started/download_and_install.md) 中所述进行安装）：

- [Windows](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer.exe)
- [OS X](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.dmg)
- [Linux](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-x86_64.AppImage) - 在运行前执行以下操作：
  - `chmod +x QGroundControl-x86_64.AppImage`
  - 在命令行中输入（仅需输入一次）：
    \- `sudo usermod -a -G dialout $USER`
    \- `sudo apt-get remove modemmanager -y`
    \- Logout and login again to enable the change to user permissions.
- [Android](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.apk)
- iOS 当前不可用
