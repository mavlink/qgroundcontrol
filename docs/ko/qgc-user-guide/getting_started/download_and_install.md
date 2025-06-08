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

_QGroundControl_ can be installed on 64 bit versions of Windows 10 (1809 or later) or Windows 11:

1. [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe)을 다운로드합니다.
2. 다운로드한 설치 파일을 더블 클릭하여 프로그램을 실행합니다.

:::info
윈도우용 설치 프로그램은 **QGroundControl**, **GPU 호환 모드** 및 **GPU 안전 모드**의 3가지 바로 가기를 생성합니다.
시작 문제나 비디오 렌더링 문제가 발생하지 않으면, 첫 번째 바로 가기를 사용하십시오.
자세한 내용은 [QGroundControl 설정 문제 해결 > 윈도우: UI 렌더링/비디오 드라이버 문제](../troubleshooting/qgc_setup.md#opengl_troubleshooting)를 참고하십시오.
:::

## Mac OS X {#macOS}

_QGroundControl_ can be installed on macOS 12 (Monterey) or later:

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg)를 다운로드합니다.
2. 다운로드한 dmg 파일을 더블 클릭하여 마운트하여, _QGroundControl_ 애플리케이션을 _Application_ 폴더로 드래그합니다.

::: info
QGroundControl continues to not be signed. You will not to allow permission for it to install based on you macOS version.
::

## 우분투 리눅스 {#ubuntu}

_QGroundControl_ can be installed/run on Ubuntu LTS 22.04 (and later):

Ubuntu comes with a serial modem manager that interferes with any robotics related use of a serial port (or USB serial).
_QGroundControl_을 설치 전에 모뎀 관리자를 제거하고, 직렬 포트 접근 권한을 부여합니다.
동영상 스트리밍을 지원하려면 _GStreamer_을 설치합니다.

_QGroundControl_을 처음 설치하기 전에:

1. 쉘 프롬프트에서 다음 명령어들을 실행합니다:
   ```sh
   sudo usermod -a -G dialout $USER
   sudo apt-get remove modemmanager -y
   sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
   sudo apt install libfuse2 -y
   sudo apt install libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev -y
   ```
   <!-- Note, remove install of libqt5gui5 https://github.com/mavlink/qgroundcontrol/issues/10176 fixed -->
2. 사용자 권한을 변경하려면 로그아웃 후 다시 로그인하여야 합니다.

&nbsp; _QGroundControl_을 설치하려면:

1. Download [QGroundControl-x86_64.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-x86_64.AppImage).
2. Install (and run) using the terminal commands:
   ```sh
   chmod +x ./QGroundControl-x86_64.AppImage
   ./QGroundControl-x86_64.AppImage  (or double click)
   ```

## 안드로이드 {#android}

_QGroundControl_ can be installed/run on Android 9 or later:

- [Android 32/64 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl.apk)

## Old Stable Releases

구 버전의 안정 배포판은 <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>를 참고하십시오.

## 일일 빌드

여기에서 [일일 빌드](../releases/daily_builds.md)를 다운로드 할 수 있습니다.
