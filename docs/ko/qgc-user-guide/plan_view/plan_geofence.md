# 계획 뷰 - 지오펜스

GeoFences를 사용하면 비행이 가능한 가상 영역 또는 비행이 _허용되지 않는_ 가상 영역을 만들 수 있습니다.
또한 허용된 지역을 벗어나 비행하는 경우 취해야 할 조치를 설정할 수 있습니다.

:::info
_QGroundControl_ will not display the GeoFence options if they are not supported by the connected vehicle firmware.
:::

## Create a GeoFence

지오펜스를 생성하려면:

1. 계획 뷰로 이동

2. Select the **GeoFence** layer using the [Layer Switcher](plan_view.md#layer_switcher) in the top-right area of the map (or expand the **GeoFence** section in the [Plan Editor Panel](plan_view.md#plan_editor_panel))

3. Insert a circular or polygon region by pressing the **Circular Fence** or **Polygon Fence** button in the GeoFence section.
   새로운 지역이 지도와 버튼 아래의 관련 울타리 목록에 추가됩니다.

:::tip
::: tip
버튼을 여러 번 눌러 여러 영역을 만들 수 있으므로 복잡한 지오펜스 정의를 생성할 수 있습니다.
:::

- 원형 영역:

  - 지도의 중앙 점을 드래그하여 지역을 이동
  - 원의 가장자리에 있는 점을 드래그하여 원의 크기를 조정합니다(또는 펜스 패널에서 반경 값을 변경할 수 있음).

- :::

  - 채워진 점을 드래그하여 정점 이동
  - 채워진 정점 사이의 선에서 "채워지지 않은" 점을 클릭하여 새 정점을 만듭니다.

1. 기본적으로 새 지역은 _포함_ 영역으로 생성됩니다(차량은 지역 내에 있어야 함).
   울타리 패널에서 연결된 _포함_ 확인란을 선택 취소하여 차량이 이동할 수 없는 제외 구역으로 변경합니다.

Depending on the firmware, the GeoFence section may also show fence parameters (e.g. breach action) and a **Breach Return Point** that the vehicle will fly to if it breaches the fence.

## 지오펜스 편집/삭제

You can select a GeoFence region to edit by selecting its _Edit_ radio button in the GeoFence section.
그런 다음 이전 섹션에서 설명한 대로 지도에서 지역을 편집할 수 있습니다.

지역은 연결된 **Del** 버튼을 눌러 삭제할 수 있습니다.

## 지오펜스 업로드

The GeoFence is uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
