# QGroundControl 시작하기

_QGroundControl_를 설치하고 실행하는 방법은 간단합니다.

1. 애플리케이션을 [다운로드](../getting_started/download_and_install.md)후에 설치합니다.
2. QGroundControl을 실행합니다.
3. 지상국에서 USB나 텔레메트리 또는 WiFi로 기체를 연결합니다. QGroundControl에서 기체를 자동으로 감지하여 연결합니다.

That's it! 비행 준비가 완료되면, _QGroundControl_에는 아래와 같은 [비행화면](../fly_view/fly_view.md)을 표시됩니다.

![](../../../assets/quickstart/fly_view_connected_vehicle.jpg)

QGroundControl에 자주 사용하는 것이 익숙해 질 수 있는 최선의 방법입니다.

- [도구 모음](../toolbar/toolbar.md)을 사용하여 아래의 기본 화면 간의 전환이 가능합니다.
  - [설정](../settings_view/settings_view.md): QGroundControl 애플리케이션을 설정합니다.
  - [설정](../setup_view/setup_view.md): 기체를 설정하고 튜닝합니다.
  - [계획](../plan_view/plan_view.md): 자율 비행 미션을 생성합니다.
  - [비행](../fly_view/fly_view.md): 스트리밍 비디오를 포함하여 비행중 기체를 모니터링합니다.
  - [분석] \*\* 분석 보기에 대한 설명이 누락되었습니다 \*\*
- 툴바에서 _상태 아이콘_을 클릭하여 연결된 기체의 상태를 확인할 수 있습니다.

While the UI is fairly intuitive, this documentation can also be referenced to find out more.

:::info
Make sure QGC has an internet connection when you connect a new vehicle. This will allow it to get the latest parameter and other metadata for the vehicle, along with [translations](../settings_view/general.md#miscellaneous).
:::
