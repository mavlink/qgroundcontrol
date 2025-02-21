# Daily Builds

Daily Builds of _QGroundControl_ have the absolute latest set of [new features](../releases/daily_build_new_features.md).

::: warning
Daily Builds are less tested than stable builds.
Use at your own risk!
:::

These can be downloaded from the links below (install as described in [Download and Install](../getting_started/download_and_install.md)):

- [Windows](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer.exe)
- [OS X](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.dmg)
- [Linux](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-x86_64.AppImage) - Before running do the following:
  - `chmod +x QGroundControl.AppImage`
  - On the command prompt enter (one time only):
		- `sudo usermod -a -G dialout $USER`
    	- `sudo apt-get remove modemmanager -y`
  		- Logout and login again to enable the change to user permissions.
- [Android](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.apk)
- iOS currently unavailable
