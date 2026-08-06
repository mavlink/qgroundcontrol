# 3D 视图

:::warning
The entire 3D View feature is a work in progress. You may hit various issues while using it.
:::

:::info
The 3D View is not available on Android, as rendering is not yet reliable across Android GPUs and drivers.
:::

3D视图用于视觉和监测载具、环境以及计划在3D中执行的任务。 [飞行视图](../fly_view/fly_view.md) 中的大部分功能在3D视图中同样可用。

您可以使用它：

- 从OpenStreetMap 网站(.osm 文件)下载的任何感兴趣区域导入和显示3D映射图。
- 在 3D 中显示车辆和它的任务。
- 以及[飞行视图](../fly_view/fly_view.md)的大多数功能，包括：
  - Run an automated [pre-flight checklist](../fly_view/hud.md#preflight_checklist).
  - 把载具解锁(或检查它为什么不会解锁)。
  - Control missions: [start](../fly_view/hud.md#start_mission), [continue](../fly_view/hud.md#continue_mission), [pause](../fly_view/hud.md#pause), and [resume](../fly_view/hud.md#resume_mission).
  - Guide the vehicle to [arm](../fly_view/hud.md#arm)/[disarm](../fly_view/hud.md#disarm)/[emergency stop](../fly_view/hud.md#emergency_stop), [takeoff](../fly_view/hud.md#takeoff)/[land](../fly_view/hud.md#land), [change altitude](../fly_view/hud.md#change_altitude), and [return/RTL](../fly_view/hud.md#rtl).
  - 在地图视图和视频视图之间切换(如果可用)
  - 显示当前载具的视频、飞行任务、遥测和其他信息，同时在已连接的载具之间切换。

# 界面概述

The main elements of the 3D View are the same as the [Fly View](../fly_view/fly_view.md), with an added 3D environment.

**Enabling the 3D View:** The 3D View is disabled by default. To enable it, go to **Settings > Fly View**, and under the **3D View** settings group, toggle the **Enabled** switch.

To open the 3D View, when you are in the [Fly View](../fly_view/fly_view.md), select the 3D View icon from the toolbar on the left.

## View Controls

The camera controls work like Gazebo: gestures are anchored to the ground point under the cursor, so the spot you grab stays under the cursor while you pan, orbit, or zoom.

- **鼠标：**
  - **Pan**: Press and hold the left mouse button, then move the cursor. The ground point you grabbed follows the cursor.
  - **Orbit (rotate)**: Press and hold the middle mouse button (scroll wheel), then move the cursor. The view rotates around the ground point under the cursor: dragging left/right changes heading (a full window width of drag is one full revolution), dragging up/down changes the viewing angle between top-down and near-horizontal.
  - **Orbit (trackpad-friendly)**: Hold **Shift** and drag with the left button. This is useful on trackpads without a middle button.
  - **Zoom**: Use the mouse wheel or trackpad scroll, or press and hold the right mouse button and drag up (zoom in) or down (zoom out). Zooming moves toward or away from the ground point under the cursor, keeping it fixed on screen.

- **触摸屏:**
  - **Pan**: Tap and move with a single finger.
  - **Zoom**: Pinch with two fingers, moving them together or apart.
  - **Rotate**: Twist with two fingers. The view rotates around the ground point under the gesture, following your fingers.
  - **Tilt**: Swipe up or down with two fingers held together. Swiping up tilts toward the horizon, swiping down toward a top-down view.

A **scale bar** is shown in the same position as in the Fly View (top of the view, next to the tool strip) and shows the ground distance covered on screen. In a perspective view the scale varies across the screen; the value shown is exact for the ground plane through the screen center.

若要在三维查看器中可视化特定区域的3D地图，您必须下载。 从 [OpenStreetMap](https://www.openstreetmap.org/#map=16/47.3964/8.5498)网站上的sm 文件，然后通过 **3D View** 的设置导入。 更多关于**3D View** 设置的详细信息可在下一节找到。

If no OSM file is loaded, buildings are not shown, but the 3D View still displays the vehicle over a map-tile ground patch centered on the vehicle's home position, viewed from the side by default.

# 设置

您可以在 **应用程序设置** ->**Fly View** 标签在 **3D View** 设置组中更改3D 视图的设置。
以下属性可以在 3D 视图设置组中修改：

- **启用**：要启用或禁用3D视图。
- **3D地图文件**：在QGC中可视化感兴趣区域的.osm文件的路径。 可以点击 **选择文件** 按钮上传.osm 文件。 要清除以前加载的 .osm 文件的3D 视图，您可以点击**清除** 按钮。
- **建筑物平均楼层高度**：此参数决定建筑物每层的高度，因为在.osm文件中，建筑物高度有时是按楼层来指定的。
- **载具高度偏差**：这是指载具及其任务的高度相对于地面的偏差。 在飞行器飞控估算的高度存在偏差的情况下，这很有用，因为3D视图目前使用的是相对高度。
