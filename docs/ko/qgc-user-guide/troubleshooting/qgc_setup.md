# QGroundControl 설정 문제 해결

호스트 컴퓨터에서 _QGroundControl_의 설정 및 설치와 관련된 문제 해결 방법을 설명합니다.

:::tip
_QGroundControl_을 **사용**하여 기체 연동시 발생하는 문제는 [QGroundControl 기체 연동 문제](../troubleshooting/qgc_usage.md)에서 설명합니다.
:::

## 64비트 윈도우: 예기치 않은 언어의 오디오

Windows 64비트 시스템에서 _QGroundControl_은 때때로 **제어판 > 음성**의 _텍스트 음성 변환_ 설정과 일치하지 않는 언어로 오디오/메시지를 재생할 수 있습니다(예: 영어 기계에서 독일어로 말하는 오디오).

이러한 현상은 64비트 Windows가 64비트 음성만 표시하는 반면에, _QGroundControl_은 32비트 응용 프로그램(Windows)이므로 32비트 음성만 실행하기 때문에 발생할 수 있습니다.

해결책은 시스템에 원하는 _32비트 음성_을 설정하는 것입니다.

1. 제어판 응용 프로그램을 실행합니다: **C:\Windows\SysWOW64\Speech\SpeechUX\sapi.cpl**.
2. 원하는 _음성 선택_을 한 다음 대화상자 하단의 **확인**을 클릭합니다.
   ![Windows 32비트 TTS(텍스트 음성 변환) 제어판](../../../assets/support/windows_text_to_speech.png)

:::info
Windows 음성 API에 대한 추가 정보는 [여기](https://www.webbie.org.uk/blog/microsoft-speech/)를 참고하십시오.
:::

## Windows: UI 렌더링/비디오 드라이버 문제 {#opengl_troubleshooting}

Windows에서 UI 렌더링 문제 또는 비디오 드라이버 충돌이 발생하는 경우에는 "비정상적" OpenGL 드라이버가 원인일 수 있습니다. _QGroundControl_은 "안전한" 동영상 모드에서 _QGroundControl_을 시작하는 데 사용할 수 있는 3가지 단축키를 제공합니다(순서대로 시도).

- **QGroundControl:** QGroundControl은 OpenGL 그래픽 드라이버를 직접 사용합니다.
- **GPU 호환성 모드:** QGC는 DirectX 위에 OpenGL을 구현하는 ANGLE 드라이버를 사용합니다.
- **GPU 안전 모드:** QGroundControl은 UI에 소프트웨어 래스터라이저를 사용합니다(매우 느림).

## Windows: WiFi 기체 연결 불량 {#waiting_for_connection}

Wi-Fi를 통해 기체 연결시 _QGroundControl_이 영구적으로 유지되는 경우(예: _기체 연결 대기_), 가능한 원인은 IP 트래픽이 방화벽 소프트웨어에 의해 차단되기 때문일 수 있습니다.

![연결을 기다리는 중](../../../assets/support/waiting_for_connection.jpg)

해결책은 방화벽을 통해 _QGroundControl_ 앱을 허용하는 것입니다.

:::info
연결을 허용하기 위해 네트워크 프로필을 공개에서 비공개로 간단히 전환할 수 있지만, PC가 네트워크에 노출되므로 주의하십시오.
:::

_Windows Defender_를 사용하는 경우:

- **시작** 표시줄에서 다음을 입력/선택합니다.
- 스크롤하여 _방화벽을 통해 앱 허용_ 옵션을 선택합니다.
- _QGroundControl_을 선택하고 _액세스_ 선택기를 **허용**으로 변경합니다.

  **Tip** 프로그램은 파일 이름이 아닌 설명의 알파벳 순서로 나열됩니다.
  _QGroundControl 개발 팀에서 제공하는 오픈 소스 지상 관제 앱_

## Ubuntu: 비디오 스트리밍 실패(Gstreamer 누락) {#missing_gstreamer}

Ubuntu에서 비디오 스트림을 보려면 _Gstreamer_ 구성요소를 설치하여야 합니다.
이것들이 설치되어 있지 않으면, _QGroundControl_이 gstreamer 노드를 생성할 수 없고 다음과 같은 에러가 발생합니다.

```sh
VideoReceiver::start()가 실패했습니다. gst_element_factory_make('avdec_h264') 오류
```

[Ubuntu용 다운로드/설치 지침](../getting_started/download_and_install.md#ubuntu)에는 _GStreamer_ 설정 정보가 포함되어 있습니다.

## Ubuntu 18.04: 듀얼 비디오 어댑터 시스템에서 비디오 스트리밍 실패 {#dual_vga}

![Ubuntu 18.04의 비디오](../../../assets/support/troubleshooting_dual_vga_driver.jpg)

Ubuntu 18.04의 GSteamer 버전에는 Intel 및 NVidia 비디오 디스플레이 어댑터가 모두 있는 시스템에서 VA API 기반 디코더(예: vaapih264dec 등)를 사용시 비디오가 표시되지 않는 버그가 있습니다. on systems that have both Intel and NVidia video display adapters.

:::info
더 일반적인 문제는 Intel 및 NVidia VGA가 있는 Ubuntu 18.04에서 발생하는 것으로 알려져 있지만, 모든 Linux 시스템 및 기타 유형의 (이중) VGA에서 발생할 수 있습니다.
:::

이 경우 _QGroundControl_ 실행하는 가장 쉬운 방법은 다음 명령어를 사용하여 시작하는 것입니다.

```
LIBVA_DRIVER_NAME=fakedriver ./QGroundControl
```

다른 대안은 VGA 중 하나를 비활성화하거나 VA API 구성 요소를 제거하거나 GStreamer 1.16으로 업그레이드하는 것입니다(Ubuntu 18.04에서는 이를 수행하는 쉬운 방법이 없습니다.

## Ubuntu 16.04: GLIBC_2.27을 찾을 수 없음 {#glibc_2_27}

사전 빌드된 QGroundControl 4.0(이상)용 AppImages는 Ubuntu 18.04 LTS(이상)에서만 실행 가능합니다.
Ubuntu 16.04에서는 실행되지 않습니다.

실행시 다음과 같은 에러가 발생합니다.

```sh
$ ./QGroundControl.AppImage
/tmp/.mount_i4hPuB/QGroundControl: /lib/x86_64-linux-gnu/libm.so.6: version `GLIBC_2.27' not found (required by /tmp/.mount_i4hPuB/QGroundControl)
```

Ubuntu 16.04를 사용해야 하는 경우의 한 가지 해결 방법은 비디오 라이브러리 없이 소스에서 빌드하는 것입니다.
