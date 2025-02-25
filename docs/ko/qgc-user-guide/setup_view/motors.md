# 모터 설정

모터 설정은 개별 모터와 서보를 테스트합니다(예: 모터가 올바른 방향으로 회전 여부 확인).

:::tip
이 지침은 PX4 및 ArduPilot의 대부분의 기체 유형에 적용됩니다.
기체별 설명서는 하위 주제로 제공됩니다(예: [모터 설정(ArduSub)](../setup_view/motors_ardusub.md)).
:::

![모터 테스트](../../../assets/setup/Motors.png)

## 테스트 절차

모터를 테스트하려면:

1. 프로펠러를 분리하십시오.

   > **경고** 모터를 작동하기 전에 프로펠러를 제거하여야 합니다!
   > :::

2. (_PX4 전용_) 안전 스위치 활성화(장착된 경우)

3. Slide the switch to enable motor slider and buttons (labeled: _Propellers are removed - Enable slider and motors_).

4. Adjust the slider to select the power for the motors.

5. Press the button corresponding to the motor and confirm it spin in the correct direction.

   ::: info
   The motors will automatically stop spinning after 3 seconds.
   You can also stop the motor by pressing the 'Stop' button.
   If no motors turn, raise the “Throttle %” and try again.
   :::

## 추가 정보

- [기본 설정 > 모터 설정](http://docs.px4.io/master/en/config/motors.html) (_PX4 사용자 가이드_) - 여기에는 추가 PX4 관련 정보를 설명합니다.
- [ESCS and Motors](https://ardupilot.org/copter/docs/connect-escs-and-motors.html#motor-order-diagrams) - This is the Motor order diagrams for all frames
