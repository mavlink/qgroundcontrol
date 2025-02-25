# 일일 빌드

_QGroundControl_의 일일 빌드에는 [새로운 기능](../releases/daily_build_new_features.md)들을 제공합니다.

:::warning
일일 빌드는 안정적인 빌드보다 테스트가 부족합니다.
Use at your own risk!
:::

아래 링크에서 다운로드할 수 있습니다([다운로드 및 설치](../getting_started/download_and_install.md)에 설명된 대로 설치).

- [윈도우](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer.exe)
- [OS X](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.dmg)
- [Linux](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-x86_64.AppImage) - Before running do the following:
  - `chmod +x QGroundControl.AppImage`
  - On the command prompt enter (one time only):
    \- `sudo usermod -a -G dialout $USER`
    \- `sudo apt-get remove modemmanager -y`
    \- Logout and login again to enable the change to user permissions.
- [Android](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.apk)
- iOS에서는 현재 사용 불가합니다.
