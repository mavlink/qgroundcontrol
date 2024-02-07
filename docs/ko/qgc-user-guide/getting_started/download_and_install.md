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

_QGroundControl_은 64비트 버전 윈도우 운영체제를 지원합니다.

1. [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe)을 다운로드합니다.
2. 다운로드한 설치 파일을 더블 클릭하여 프로그램을 실행합니다.

:::info
윈도우용 설치 프로그램은 **QGroundControl**, **GPU 호환 모드** 및 **GPU 안전 모드**의 3가지 바로 가기를 생성합니다.
시작 문제나 비디오 렌더링 문제가 발생하지 않으면, 첫 번째 바로 가기를 사용하십시오.
자세한 내용은 [QGroundControl 설정 문제 해결 > 윈도우: UI 렌더링/비디오 드라이버 문제](../troubleshooting/qgc_setup.md#opengl_troubleshooting)를 참고하십시오.
:::

:::info
_QGroundControl_ 4.0 버전 이상의 사전 빌드 버전은 64비트 전용입니다.
32비트 버전은 수동으로 빌드할 수 있습니다(개발팀에서는 지원하지 않음).
:::

## Mac OS X {#macOS}

_QGroundControl_은 MacOS 10.11 이상에서 설치할 수 있습니다.

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg)를 다운로드합니다.
2. 다운로드한 dmg 파일을 더블 클릭하여 마운트하여, _QGroundControl_ 애플리케이션을 _Application_ 폴더로 드래그합니다.

::: info
QGroundControl continues to not be signed which causes problem on Catalina. To open QGC app for the first time:

- QGroundControl 앱 아이콘을 마우스 오른쪽 버튼으로 클릭한 다음에 메뉴에서 열기를 선택합니다. 취소 옵션만 제공됩니다. 취소를 선택합니다.
- QGroundControl 앱 아이콘을 마우스 오른쪽 버튼으로 클릭한 다음에 메뉴에서 열기를 선택합니다. 이번에는 열기 옵션이 표시됩니다.
  :::

## 우분투 리눅스 {#ubuntu}

_QGroundControl_은 Ubuntu LTS 20.04 이상의 버전에 설치됩니다.

Ubuntu comes with a serial modem manager that interferes with any robotics related use of a serial port (or USB serial).
_QGroundControl_을 설치 전에 모뎀 관리자를 제거하고, 직렬 포트 접근 권한을 부여합니다.
동영상 스트리밍을 지원하려면 _GStreamer_을 설치합니다.

_QGroundControl_을 처음 설치하기 전에:

1. 쉘 프롬프트에서 다음 명령어들을 실행합니다:
   ```sh
   sudo usermod -a -G dialout $USER
   sudo apt-get remove modemmanager -y
   sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
   sudo apt install libqt5gui5 -y
   sudo apt install libfuse2 -y
   ```
   <!-- Note, remove install of libqt5gui5 https://github.com/mavlink/qgroundcontrol/issues/10176 fixed -->
2. 사용자 권한을 변경하려면 로그아웃 후 다시 로그인하여야 합니다.

&nbsp; _QGroundControl_을 설치하려면:

1. [QGroundControl.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.AppImage)를 다운로드합니다.
2. Install (and run) using the terminal commands:
   ```sh
   터미널 명령을 사용하여 설치:
      sh
      chmod +x ./QGroundControl.AppImage
      ./QGroundControl.AppImage (or double click)
   ```

듀얼 어댑터가 있는 Ubuntu 18.04 시스템에는 [동영상 스트리밍 문제](../troubleshooting/qgc_setup.md#dual_vga)가 있습니다.
:::

:::info
_QGroundControl_ 4.0의 사전 빌드된 버전은 Ubuntu 16.04에서 실행 불가능합니다.
Ubuntu 16.04에서 이 버전을 실행하려면, [소스에서 비디오 라이브러리를 제거한 다음에 QGroundControl을 빌드](https://dev.qgroundcontrol.com/en/getting_started/)하여야 합니다.
:::

## 안드로이드 {#android}

Google Play 스토어에서 _QGroundControl_을 일시적으로 사용할 수 없습니다. 아래의 링크들에서 수동으로 설치할 수 있습니다.

- [Android 32 비트 APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl32.apk)
- [Android 64 비트 APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl64.apk)

## Old Stable Releases

구 버전의 안정 배포판은 <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>를 참고하십시오.

## 일일 빌드

여기에서 [일일 빌드](../releases/daily_builds.md)를 다운로드 할 수 있습니다.
