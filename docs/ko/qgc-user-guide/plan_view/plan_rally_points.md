# 계획 뷰 - 랠리 포인트

랠리 포인트는 대체 착륙 또는 배회 장소입니다.
일반적으로 리턴/RTL 모드에서 홈 위치보다 더 안전하거나 더 편리한(예: 더 가까운) 목적지를 지정합니다.

:::info
_QGroundControl_ will not display the Rally Point options if they are not supported by the connected vehicle firmware.
:::

## 랠리 포인트 사용법

랠리 포인트를 생성하려면:

1. 계획 뷰로 이동합니다.
2. Select the **Rally Points** layer using the [Layer Switcher](plan_view.md#layer_switcher) in the top-right area of the map (or expand the **Rally Points** section in the [Plan Editor Panel](plan_view.md#plan_editor_panel))
3. 랠리 포인트를 지도에서 원하는 곳을 클릭합니다.
   - 각각에 대해 **R** 마커가 추가됩니다.
   - The currently active marker has a different color (green) and its editor is expanded in the _Rally Points_ section.
4. Make any rally point active by selecting it on the map or in the _Rally Points_ section:
   - Move the active rally point by either dragging it on the map or editing the position fields in its editor.
   - Delete a rally point by pressing the **X** delete button on its editor.

## 랠리 포인트 업로드

Rally points are uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
