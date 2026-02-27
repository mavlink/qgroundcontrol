# 다운로드 및 설치

다양한 운영체제에서 실행 가능한 QGroundControl [최신 안정판](../releases/release_notes.md)를 다운로드할 수 있습니다.

:::tip
설치 후 QGroundControl이 정상적으로 실행되지 않으면, [QGC 설정 문제 해결](../troubleshooting/qgc_setup.md)편을 참고하여 문제를 해결할 수 있습니다.
:::

## 시스템 요구 사항

QGroundControl는 최신 컴퓨터나 모바일 장치에서 실행 가능합니다. 시스템 환경과 사용 가능한 시스템 리소스 상태에 따라 성능이 차이가 날 수 있습니다.
More capable hardware will provide a better experience.
최소 8Gb 메모리, SSD 다스크, Nvidia 또는 AMD 그래픽 및 i5 이상의 CPU가 장착된 컴퓨터를 사용하는 것이 좋습니다.

최신 버전의 운영 체제에서 최적의 사용과 호환성을 발휘할 수 있습니다.

## 윈도우 {#windows}

Supported versions: Windows 10 (1809 or later), Windows 11:

1. [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe)을 다운로드합니다.
2. 다운로드한 설치 파일을 더블 클릭하여 프로그램을 실행합니다.

:::info
윈도우용 설치 프로그램은 **QGroundControl**, **GPU 호환 모드** 및 **GPU 안전 모드**의 3가지 바로 가기를 생성합니다.
시작 문제나 비디오 렌더링 문제가 발생하지 않으면, 첫 번째 바로 가기를 사용하십시오.
자세한 내용은 [QGroundControl 설정 문제 해결 > 윈도우: UI 렌더링/비디오 드라이버 문제](../troubleshooting/qgc_setup.md#opengl_troubleshooting)를 참고하십시오.
:::

## Mac OS {#macOS}

Supported versions: macOS 12 (Monterey) or later:

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg)를 다운로드합니다.
2. 다운로드한 dmg 파일을 더블 클릭하여 마운트하여, _QGroundControl_ 애플리케이션을 _Application_ 폴더로 드래그합니다.

:::info
QGroundControl continues to not be signed. You will not to allow permission for it to install based on your macOS version.
:::

## 우분투 리눅스 {#ubuntu}

Supported versions: Ubuntu 22.04, 24.04:

Ubuntu comes with a serial modem manager that interferes with any robotics related use of a serial port (or USB serial).
_QGroundControl_을 설치 전에 모뎀 관리자를 제거하고, 직렬 포트 접근 권한을 부여합니다.
동영상 스트리밍을 지원하려면 _GStreamer_을 설치합니다.

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

## 안드로이드 {#android}

Supported versions: Android 9 to 15 (arm 32/64):

- [Android 32/64 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl.apk)

## Old Stable Releases

구 버전의 안정 배포판은 <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>를 참고하십시오.

## 일일 빌드

여기에서 [일일 빌드](../releases/daily_builds.md)를 다운로드 할 수 있습니다.
