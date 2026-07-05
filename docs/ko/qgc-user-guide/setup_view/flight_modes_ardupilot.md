# ArduPilot 비행 모드 설정

_비행 모드_ 섹션에서는 RC 송신기의 특정 스위치/스위치 위치에 의해 트리거되는 비행 모드 및 기타 작업을 설정할 수 있습니다.

::: info
In order to set up flight modes you must already have

> - 비행 모드를 설정하려면 비행 모드를 설정하기 위해 [무전기를 구성](../setup_view/radio.md)해야 합니다.
> - [Setup the RC transmitter](flight_modes.md#transmitter-setup) (Flight Modes > Transmitter Setup)> :::

To access this section, select the **Gear** icon (Vehicle Configuration) in the top toolbar and then **Flight Modes** in the sidebar.

## 비행 모드 설정

ArduPilot에서 최대 6개의 다른 비행 모드를 송신기의 단일 채널에 할당할 수 있습니다(채널은 평면에서 선택 가능하지만 멀티콥터에서는 채널 5로 고정됨).
또한 ArduCopter를 사용하면 채널 7-12에 대한 추가 _채널 옵션_을 설정할 수 있습니다.
이를 통하여 스위치에 기능을 설정합니다(예: 카메라를 켜거나 실행으로 돌아가기).

비행 모드를 설정 방법:

1. RC 송신기를 켭니다.

2. Select the **Gear** icon (Vehicle Configuration) in the top toolbar and then **Flight Modes** in the sidebar.

   ::: info
   위 이미지는 ArduCopter의 비행 모드 설정 스크린샷입니다.
   :::

3. 드롭다운에서 최대 6개의 비행 모드를 선택합니다.

4. **ArduCopter만 해당:** 채널 7-12에 대해 추가 _채널 옵션_을 선택합니다.

5. **ArduPlane만 해당:** 드롭다운에서 모드 채널을 선택합니다.

6. 송신기의 각 모드 스위치를 차례로 선택하여 모드가 올바른 송신기 스위치에 매핑되는지 테스트하고, 선택한 비행 모드 활성화 여부를 확인합니다(활성 모드의 경우 _QGroundControl_에서 텍스트가 노란색으로 변경됩니다).

모든 값은 변경시에 자동으로 저장됩니다.

:::info
A typical setup uses a three position flight mode switch with an additional option of RTL being on a channel 7 switch.
You can also setup 6 flight modes using two switches plus mixing on your transmitter. Scroll down to the center section of this [page](http://ardupilot.org/copter/docs/common-rc-transmitter-flight-mode-configuration.html#common-rc-transmitter-flight-mode-configuration) for tutorials on how to do that.
:::

## See Also

- [ArduCopter 비행 모드](http://ardupilot.org/copter/docs/flight-modes.html)
- [ArduPlane 비행 모드](http://ardupilot.org/plane/docs/flight-modes.html)
- [ArduCopter > 보조 기능 스위치](https://ardupilot.org/copter/docs/channel-7-and-8-options.html#channel-7-and-8-options) - 채널 설정에 대한 추가 정보
