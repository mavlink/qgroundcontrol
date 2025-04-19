# Fly View

The Fly View is used to command and monitor the vehicle.

## Overview

![비행 뷰](../../../assets/fly/fly_view_overview.jpg)

- **[Toolbar](fly_view_toolbar.md):** The toolbar is at the top of the screen. It provides controls to select views, show flight status and mode as well as the status of the main components of the vehicle.
- **[Vehicle Actions](fly_tools.md):** Allows you command the vehicle to take a specific action.
- **[Instrument Panel](instrument_panel.md):** A widget that displays vehicle telemetry.
- **[Attitude/Compass](hud.md):** A widget that provides virtual horizon and heading information.
- **[Camera Tools](camera_tools.md)**: A widget for switching between still and video modes, starting/stopping capture, and controlling camera settings.
- **[Video](video.md):** Display the video from the vehicle. Allows you to toggle between video or map as the main display.
- **Map:** Displays the positions of all connected vehicles and the mission for the current vehicle.
  - You can drag the map to move it around (the map automatically re-centres on the vehicle after a certain amount of time).
  - You can zoom the map in and out using the zoom buttons, mouse wheel, track pad or pinch on a tablet.
  - 비행 후에는 지도를 클릭하여 [이동](#goto) 또는 [궤도](#orbit) 위치를 설정할 수 있습니다.

There are a number of other elements that are not displayed by default and are only displayed in certain conditions or for certain vehicle types.

## 액션/태스크

다음 섹션에서는 비행 화면에서 일반적인 작업 수행 방법을 설명합니다.

:::info
Many of the available options depend on both the vehicle type and its current state.
:::

### Actions associated with a map position (#map_actions)

There are a number of actions which can be taken which are associated with a specific position on the map. To use these actions:

1. Click on the map at a specific position
2. A popup will display showing you the list of available actions
3. Select the action you want
4. Confirm the action

Examples of map position actions are Go To Location, Orbit and so forth.

### 일시 중지

이륙, 착륙, RTL, 임무 실행 및 위치 궤도를 포함한 대부분의 작업을 일시 중지할 수 있습니다. 일시 정지 시 기체의 동작은 기체 유형에 따라 달라집니다. 일반적으로 멀티콥터는 호버링을 하고, 고정익은 선회 비행을합니다.

:::info
_위치 이동_ 작업은 일시 중지가 불가능 합니다.
:::

일시 중지하려면:

1. _비행 도구_에서 **일시중지** 버튼을 클릭합니다.
2. 선택적으로 오른쪽 수직 슬라이더를 사용하여 새 고도를 설정합니다.
3. 슬라이더를 사용하여 일시 중지를 확인합니다.

### 미션

#### 미션 시작 {#start_mission}

기체는 착륙후 미션를 시작할 수 있습니다(미션 시작 확인 슬라이더는 대부분 기본적으로 표시됩니다).

착륙후 임무를 시작하려면:

1. _비행 도구_에서 **액션** 버튼을 클릭합니다.

2. 대화 상자에서 _미션 시작_ 작업을 선택합니다.

   ![미션 액션 시작](../../../assets/fly/start_mission_action.jpg)

   (확인 슬라이더를 표시하기 위하여)

3. 확인 슬라이더가 나타나면 드래그하여 미션을 시작합니다.

   ![미션 시작](../../../assets/fly/start_mission.jpg)

#### 미션 지속 {#continue_mission}

비행 중일 때 _다음_ 웨이포인트에서 미션을 _지속_할 수 있습니다(이륙 후에 _미션 지속_ 확인 슬라이더가 기본적으로 표시되는 경우가 많습니다).

:::info
미션 지속과 [미션 재개](#resume_mission)는 차이가 있습니다.
지속은 일시 중지되었거나 이륙했기 때문에, 이륙 미션 명령을 놓친 미션를 다시 시작시에 사용합니다.
미션 재개는 착륙지 복귀를 사용하였거나 미션 중간 착륙시 사용합니다(예: 배터리 교체). 그런 다음 다음 미션 항목을 지속할 수 있습니다(즉, 미션에서 지속하는 것이 아니라 미션에 있었던 위치로 이동합니다).
:::

현재 임무를 계속할 수 있습니다(이미 임무에 참여하지 않는 한!):

1. _비행 도구_에서 **액션** 버튼을 클릭합니다.

2. 대화 상자에서 _임무 지속_ 작업을 선택합니다.

   ![임무 계속/고도 변경 작업](../../../assets/fly/continue_mission_change_altitude_action.jpg)

3. 확인 슬라이더를 끌어 임무를 지속하십시오.

   ![미션 계속](../../../assets/fly/continue_mission.jpg)

#### 임무 재개 {#resume_mission}

_임무 재개_는 임무 수행중 [착륙지 복귀/복귀](#rtl)나 [착륙](#land)을 수행 후 임무를 재개합니다 (예를 들어 배터리 교체).

:::info
배터리를 교체하는 경우에는 배터리를 분리한 후 기체에서 QGroundControl를 분리하지 **마십시오**.
새 배터리를 교체하면 _QGroundControl_에서 차량을 다시 감지하고 자동으로 연결을 복원합니다.
:::

착륙 후에는 _비행 계획 완료_ 대화상자가 표시되며, 이 대화상자에서는 계획을 기체에서 제거하거나, 기체에 그대로 두거나, 또는 통과한 마지막 웨이포인트에서 임무를 재개합니다.

![임무 재개](../../../assets/fly/resume_mission.jpg)

임무 재개를 선택하면 _QGroundControl_에서 임무를 재구성하고 차량에 업로드합니다.
그런 다음 _임무 시작_ 슬라이더를 사용하여 임무를 계속 수행합니다.

아래 이미지는 위의 귀환 이후 재건된 임무를 나타냅니다.

![재건 임무 재개](../../../assets/fly/resume_mission_rebuilt.jpg)

:::info
임무는 임무의 다음 단계에 영향을 미치는 마지막 웨이포인트에 여러 항목이 있을 수 있으므로, 기체가 실행한 마지막 미션 항목에서 단순히 재개할 수 없습니다(예: 속도 명령 또는 카메라 제어 명령).
대신 _QGroundControl_은 비행한 마지막 임무 항목부터 시작하여 임무를 재구성하고 자동으로 임무 앞에 관련 명령을 추가합니다.
:::

#### 착륙 후 임무 프롬프트 제거 {#resume_mission_prompt}

임무가 완료후 기체가 착륙한 다음 시동이 해제되면 기체의 임무를 제거하라는 메시지가 표시됩니다.
이는 이전의 임무가 의도치 않게 기체에 저장되어, 예기치 않은 동작을 초래하는 문제를 방지합니다.

### 비디오 출력 {#video_switcher}

비디오 스트리밍이 활성화되면, _QGroundControl_은 지도 왼쪽 하단의 "비디오 전환기 창"에 현재 선택된 기체의 비디오 스트림을 표시합니다.
아무 곳이나 스위처를 눌러 _동영상_ 및 _지도_를 전경으로 전환할 수 있습니다(아래 이미지에서 동영상은 전경에 표시됨).

![비디오 스트림 녹화](../../../assets/fly/video_record.jpg)

:::info
비디오 스트리밍은 [애플리케이션 설정 > 일반 탭 > 비디오](../settings_view/general.md#video)에서 활성화하거나 설정할 수 있습니다.
:::

스위처의 컨트롤을 사용하여, 비디오 디스플레이를 추가로 설정할 수 있습니다.

![Video Pop](../../../assets/fly/video_pop.jpg)

- 오른쪽 상단 모서리에 있는 아이콘을 끌어 스위처의 크기를 조정합니다.
- 왼쪽 하단의 토글 아이콘을 눌러 스위처를 숨깁니다.
- 왼쪽 상단 모서리에 있는 아이콘을 눌러서 비디오 스위처 창을 분리합니다.
  분리된 창을 닫으면 스위처가 QGroundControl의 비행 화면으로 다시 전환됩니다.

### 동영상 녹화

카메라와 기체에서 지원하는 경우 _QGroundControl_에서 카메라 자체의 동영상 녹화를 시작하거나 중지할 수 있습니다. _QGroundControl_은 또한 비디오 스트림을 녹화하고 로컬에 저장할 수 있습니다.

:::tip
카메라에 저장된 동영상의 품질은 훨씬 높을 수 있지만, 지상국의 녹화 용량은 훨씬 더 클 수 있습니다.
:::

#### 비디오 스트림 녹화(GCS에서)

비디오 스트림 녹화는 [비디오 스트림 도구 페이지](#video_instrument_page)에서 제어합니다.
새 비디오 녹화를 시작하려면, 빨간색 원을 누르십시오(원을 누를 때마다 새 비디오 파일이 생성됨).

![비디오 스트림 녹화](../../../assets/fly/video_record.jpg)

비디오 스트림 녹화는 [애플리케이션 설정 > 일반 탭](../settings_view/general.md)에서 설정합니다.

- [동영상 녹화](../settings_view/general.md#video-recording) - 녹화 파일 형식 및 저장 제한을 지정합니다.

  **참고** 동영상은 기본적으로 Matroska 형식(.mkv)으로 저장됩니다.
  이 형식은 오류 발생으로 인한 손상에 대해 상대적으로 강건합니다.
  :::

- [Miscellaneous](../settings_view/general.md#miscellaneous) - 스트리밍된 동영상은 **애플리케이션 로드/저장 경로** 아래에 저장됩니다.

:::tip
저장된 동영상에는 동영상 스트림 자체만 포함됩니다.
QGroundControl 애플리케이션을 포함하여 비디오를 녹화하려면 별도의 화면 녹화 프로그램을 사용하여야 합니다.
:::

#### 카메라 비디오 녹화

[카메라 기기 페이지](#camera_instrument_page)를 사용하여 _카메라 자체에서_ 동영상 녹화를 시작하거나 중지합니다.
먼저 비디오 모드로 전환한 다음, 빨간색 버튼을 클릭하여 녹화를 시작합니다.

![계기 페이지 - 카메라 MAVLink 설정](../../../assets/fly/instrument_page_camera_mavlink.jpg)
