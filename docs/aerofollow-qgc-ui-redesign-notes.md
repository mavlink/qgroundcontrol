# AeroFollow QGC UI 重构记录

截至：2026-07-02

## 任务范围

目标是对 `C:\Users\arkvt\Desktop\AeroFollow\qgroundcontrol` 中的 QGroundControl Fly View UI 做整体重构，而不是局部美化。

构建脚本：

```powershell
C:\Users\arkvt\Desktop\AeroFollow\qgroundcontrol\tools\build-qgc-windows.ps1
```

默认构建目录来自脚本：

```text
build\qgc-v5.0.8-ui
```

## 用户明确要求

- 默认交互和文档使用中文。
- UI 要作为一个整体重构，不能只改圆角、透明度、间距或单个按钮。
- 配色、整体风格、图标语言、按钮状态、信息层级需要统一。
- 桌面端和平板端使用同一套设计系统。
- 不要设计成“桌面一套、平板一套”。平板尺寸比例和 PC 类似，只做同一套 UI 下的自然响应式适配。
- 地图/视频主画布仍然是第一优先级，控制面板不能无意义遮挡画面。
- 需要更易用：飞手应能快速扫读飞行状态、车辆状态、关键遥测和常用动作。

## 原始截图观察

用户提供的原始主页截图显示当前 Fly View 存在这些问题：

- 顶栏信息拥挤，Logo、飞行状态、模式、卫星、电量、飞机名和品牌图混在同一条黑色栏中，层级不清。
- 左侧动作栏是旧式深色块，图标和文字比例不统一，动作分组不明确。
- 右侧多机面板视觉上像孤立浮层，车辆卡片、选择按钮、动作按钮的状态和密度不统一。
- 右下指南针/仪表和右侧面板、地图标尺等控件风格不一致。
- 颜色体系混杂：黑、灰、绿、紫色 Logo、ArduPilot 品牌图、红色航迹/箭头之间缺少统一规范。
- 图标大小、线宽、填充风格不统一，部分中英文文本在控件中显得拥挤。
- 当前界面更像多个历史控件叠加，不像一个完整的操作台。

## 统一设计方向

设计方向应偏向“航空作业控制台”，不是营销页、仪表盘炫技页或卡片堆叠页。

核心原则：

- 地图/视频全屏主画布保持最大面积。
- 顶栏只承载全局状态、连接状态、当前模式和关键摘要。
- 左侧工具轨承载高频飞行动作，动作按钮必须清晰、稳定、易触控。
- 右侧区域承载多机/选中车辆状态，强调当前车辆和可执行动作。
- 底部区域承载遥测扫读，不应变成很厚的装饰栏。
- 正常状态使用绿色/青绿色，警告使用琥珀色，危险使用红色。
- 深色半透明面板覆盖地图时，需要清晰边界、足够对比度和一致的圆角。
- 平板端沿用同一套控件、颜色、图标和层级，只通过最大宽度、最小触控尺寸、间距约束自然适配。

## 设计基准图

已生成一张 Fly View 重构概念图，作为风格参考，不作为必须逐像素复刻的最终 UI。

生成图路径：

```text
C:\Users\arkvt\.codex\generated_images\019f222a-4591-7902-b6be-9d6642df155a\ig_0faf61b318eef7c8016a46302c67e481999ed6c3cf0b3a1a2a.png
```

概念图要点：

- 深色半透明顶栏。
- 左侧统一工具轨。
- 地图主画布干净，任务/状态信息贴边展示。
- 右侧车辆/仪表/动作区域统一为操作面板。
- 底部车辆或遥测条与右侧面板风格一致。

注意：用户随后强调“原始主页截图如上，基本上需要完全重构”，所以实现不应只套概念图局部样式，而应做统一系统。

## 已定位源码结构

核心 Fly View：

```text
src\FlightDisplay\FlyView.qml
src\FlightDisplay\FlyViewWidgetLayer.qml
src\FlightDisplay\FlyViewToolStrip.qml
src\FlightDisplay\FlyViewToolStripActionList.qml
src\FlightDisplay\FlyViewTopRightPanel.qml
src\FlightDisplay\FlyViewBottomRightRowLayout.qml
src\FlightDisplay\FlyViewInstrumentPanel.qml
src\FlightDisplay\TelemetryValuesBar.qml
src\FlightDisplay\MultiVehicleList.qml
```

通用 QML 控件：

```text
src\QmlControls\ToolStrip.qml
src\QmlControls\ToolStripHoverButton.qml
src\QmlControls\FlyViewToolBar.qml
src\QmlControls\FlyViewToolBarIndicators.qml
src\QmlControls\QGCButton.qml
src\QmlControls\QGCToolBarButton.qml
src\QmlControls\MainStatusIndicator.qml
src\QmlControls\QGCPalette.cc
src\QmlControls\QGCPalette.h
```

QML 模块注册：

```text
src\FlightDisplay\CMakeLists.txt
src\QmlControls\CMakeLists.txt
```

Qt/QML 参考：

- 已通过 Context7 查询 Qt 6.8 官方文档。
- 重点参考 Qt Quick Layouts、anchors、states、property bindings、Control/Button styling。
- 修改时避免制造 binding loop，不混用同一方向上的 anchors 与 Layout 尺寸控制。

## 当前实现判断

第一阶段建议先重构 Fly View 外壳，不碰 MAVLink、任务规划、地图渲染、车辆控制后端：

- `QGCPalette`：统一新的深色操作台配色。
- `FlyViewToolBar`：重做顶部全局状态栏。
- `QGCToolBarButton` 和 `MainStatusIndicator`：统一顶栏按钮和状态胶囊。
- `ToolStrip` / `ToolStripHoverButton`：重做左侧工具轨样式、触控尺寸、选中/悬停状态。
- `FlyViewTopRightPanel` / `MultiVehicleList`：重做右侧多机面板和车辆卡片。
- `FlyViewBottomRightRowLayout` / `TelemetryValuesBar`：重做底部遥测扫读区域。

不建议第一阶段做的事：

- 不重写飞控业务逻辑。
- 不替换地图控件。
- 不引入 Web 前端框架。
- 不做桌面和平板两套 UI。
- 不把 QGC 变成营销式首页或通用数据看板。

## 统一视觉系统草案

颜色：

- 背景面板：接近黑色的冷中性深灰，带轻微透明感。
- 面板边界：低透明白色或青绿色选中边。
- 主文本：近白。
- 次级文本：中灰。
- 正常/Ready：绿色。
- 导航/强调：青绿色。
- 警告：琥珀色。
- 危险：红色。

形态：

- 面板圆角保持统一，约 8px 或按 `ScreenTools.defaultFontPixelWidth` 比例换算。
- 卡片、工具轨、顶栏都使用同一套边框/背景/高亮逻辑。
- 按钮不做过度装饰，优先图标 + 短文本。
- 地图覆盖面板应有稳定宽高，避免内容变化导致跳动。

信息层级：

- 一级：连接/飞行状态、当前模式、当前活动车辆。
- 二级：GPS、电池、关键遥测、通信/告警。
- 三级：多机选择、批量动作、相机/附加动作。
- 四级：设置/高级诊断入口。

图标：

- 保留现有 QGC 资源优先，统一颜色、大小和状态。
- 如果现有图标风格差异太大，再补小型 SVG 资源，但不在第一步大量替换。

## 平板适配原则

平板不另做一套视觉。

只允许这些响应式调整：

- 使用同一套颜色、图标、控件和状态。
- 保证最小触控尺寸接近 `ScreenTools.minTouchPixels`。
- 控制面板设置 `maximumWidth`，避免宽度挤占地图。
- 顶栏和底部栏使用横向滚动或截断，而不是换成完全不同布局。
- 右侧面板在窄宽度下控制高度和内部滚动，避免遮挡地图主目标。
- 所有文本保持同一字号比例，避免按视口宽度任意缩放字体。

## 验证方式

构建：

```powershell
.\tools\build-qgc-windows.ps1 -Action build -Config Debug
```

如需部署：

```powershell
.\tools\build-qgc-windows.ps1 -Action deploy -Config Debug
```

如需运行：

```powershell
.\tools\build-qgc-windows.ps1 -Action run -Config Debug
```

已有可执行文件位置：

```text
build\qgc-v5.0.8-ui\Debug\QGroundControl.exe
```

检查重点：

- QML 是否能编译。
- Fly View 是否能正常加载。
- 顶栏、左侧工具轨、右侧多机面板、底部遥测区视觉是否统一。
- 桌面和平板比例窗口下是否保持同一套设计。
- 关键动作按钮是否仍能触发原有 guided action。
- 多机选择、遥测栏、仪表面板是否仍能显示原有数据。
- 地图/视频主画布是否仍是视觉中心。

## 后续继续执行时的优先顺序

1. 先改通用视觉底座：`QGCPalette`、`QGCButton`、`QGCToolBarButton`。
2. 再改 Fly View 顶栏：`FlyViewToolBar`、`MainStatusIndicator`。
3. 再改左侧工具轨：`ToolStrip`、`ToolStripHoverButton`、必要时调整 `FlyViewToolStripActionList` 的文案和顺序。
4. 再改右侧多机面板：`FlyViewTopRightPanel`、`MultiVehicleList`。
5. 再改底部遥测和仪表外壳：`FlyViewBottomRightRowLayout`、`TelemetryValuesBar`、`FlyViewInstrumentPanel`。
6. 构建并根据错误修正 QML。
7. 启动应用，截图检查桌面和平板比例窗口。

