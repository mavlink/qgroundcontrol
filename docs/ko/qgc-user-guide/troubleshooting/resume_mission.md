# 미션 재실행 실패

배터리 교체 후 임무 재개 과정은 QGroundControl에서 매우 복잡한 과정입니다.

가장 문제가 되는 두 가지 주요 문제는 다음과 같습니다.

- _임무 재개_ 대화상자가 표시되지 않고 임무 시작 슬라이더만 남습니다.
- _임무 재&#xAC1C;_&#xC5D0;서 생성된 새 임무는 웨이포인트 재생성과 카메라 명령과 관련하여 정확하지 않습니다.

:::warning
_QGroundControl_ 개발 팀이 이러한 문제를 디버그하려면 _임무 재&#xAC1C;_&#xC5D0; 입력된 모든 github 문제에 다음 정보가 **제공되어야 합니다**.
:::

## 임무 재개 대화창 또는 생성을 위한 일반적인 단계 {#common_steps}

두 가지 유형의 문제를 디버깅하려면 다음 단계가 필요합니다.

1. QGroundControl 다시 시작

2. Turn on [console logging](../troubleshooting/console_logging.md) with the logging category: `QMLControls.GuidedActionsController`.

3. [원격 분석 로깅](../settings_view/general.md#miscellaneous)을 활성화합니다(**설정 > 일반**).

4. 미션을 시작합니다.

5. 배터리 교체가 필요할 때까지 비행합니다.

   **Tip** 또는 미션 중간부터 수동으로 출발지를 복귀하여 문제를 재현할 수 있습니다(항상 문제가 재현되지는 않음).
   :::

6. Once the vehicle lands and disarms you should get the _Resume Mission_ dialog.

   ::: info
   If not there is a possible bug in QGC.
   :::

### 임무 재개 대화 문제

_임무 재개 대화창_ 문제의 경우 [위의 일반적인 단계](#common_steps)를 따른 후 다음을 수행하십시오.

1. Save the _App Log_ to a file.
2. Place the _App Log_, _Telemetry Log_ and _Plan File_ someplace which you can link to in the issue.
3. 세 파일 모두에 대한 세부 정보와 네트워크를 사용하여 문제를 만듭니다.

## 임무 재개 생성 문제

_임무 재개 생성_ 문제의 경우 [위의 일반적인 단계](#common_steps)를 따른 후 다음을 수행하십시오.

1. **임무 재개**를 클릭합니다.
2. 새로운 미션이 생성되어야 합니다.
3. [계획 화면](../plan_view/plan_view.md)으로 이동합니다.
4. _파일/동기화_ 메뉴에서 **다운로드**를 선택합니다.
5. _수정된 계&#xD68D;_&#xC744; 파일에 저장합니다.
6. Save the _App Log_ to a file.
7. Place the _App Log_, _Telemetry Log_, _Original Plan_ file and _Modified Plan_ file someplace which you can link to in the issue.
8. 4개의 모든 파일에 대한 세부 정보와 링크를 사용하여 이슈를 생성합니다.
