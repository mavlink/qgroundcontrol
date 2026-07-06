# QGC UI 系统级重构审计

日期：2026-07-04

## 结论

当前不能继续按单点补丁推进。仓库 `src` 下共扫描到 421 个 QML 文件，前端主要分布如下：

| 模块 | QML 数量 | 说明 |
| --- | ---: | --- |
| `QmlControls` | 150 | 共享控件、Plan View、参数编辑器、任务编辑器、弹窗、菜单、表单 |
| `AutoPilotPlugins` | 74 | PX4/APM/Common 车辆配置页 |
| `FlightDisplay` | 59 | Fly View、guided action、底部车辆条、预飞检查、视频、警告 |
| `UI` | 38 | 主窗口、顶部工具入口、应用设置页 |
| `FlightMap` | 29 | 地图、地图对象、罗盘/姿态/相机控件 |
| `Viewer3D` | 22 | 3D 查看器和模型 |
| `UTMSP` | 14 | 空域服务状态、通知、地图可视化 |
| `FactSystem` | 13 | Fact 表单控件 |
| `Vehicle` | 10 | Vehicle Setup 外层和 Joystick/Firmware/Summary 页面 |
| `AnalyzeView` | 6 | Analyze Tools 各页面 |
| `FirmwarePlugin` | 3 | PX4 指示器/状态内容 |
| `FirstRunPromptDialogs` | 2 | 首次启动提示 |
| `Comms` | 1 | AirLink 设置 |

系统割裂的主要原因不是某几个颜色没改，而是有三套结构并存：

1. Fly View 已经有新的半透明浮窗语言。
2. Tool Drawer / App Settings / Analyze / Setup / Plan 仍大量使用旧的 `qgcPal.window`、`qgcPal.windowShade`、实体 divider。
3. 共享控件只做了部分调整，导致进入其他页面后旧按钮、旧卡片、旧弹窗继续出现。

## 重构原则

- 不改业务后端，不改飞控逻辑，不写死车辆/固件页面。
- 优先改共享控件和容器，少逐页硬改。
- 主背景使用中性黑灰、半透明、可读；主题色只用于 active/primary/关键状态，不用于分割线。
- 分割线统一使用低透明中性灰，尽量减少显性线条。
- 圆角统一小圆角，不做卡片套卡片。
- Fly View 的地图上浮窗可以使用毛玻璃；全屏工具页不一定要毛玻璃，但背景、按钮、分组必须和主风格一致。
- Analyze / Setup / Settings / Plan 这类全屏页面应有统一页面壳层，否则用户切页面会立刻觉得是两个系统。

## 2026-07-05 进展

- `FlyViewTopRightPanel.qml` 右侧中部仪表不再直接嵌旧 `FlyViewInstrumentPanel`，改为读取当前车辆真实 `heading/roll/pitch` Fact 的中性灰自绘姿态/航向盘。
- Fly View 顶栏、底栏、右侧车辆菜单、左侧工具按钮按下态中残留的偏青 hover/pressed 背景，已收回到中性黑灰。
- `FlightModeIndicator.qml`、`MultiVehicleList.qml`、`TelemetryValuesBar.qml` 中旧 `groupBorder` 和偏青实底已弱化，避免在抽屉/旧入口中露出明显割裂。
- 已构建通过：`.\tools\build-qgc-windows.ps1 -Action build`。
- 已最大化截图验证右侧面板打开状态：`build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-right-instrument-neutral-open.png`。

## P0：先收统一壳层和共享控件

### 1. 主窗口壳层

文件：

- `src/UI/MainWindow.qml`

覆盖：

- `showPlanView`
- `showFlyView`
- `showAnalyzeTool`
- `showVehicleConfig`
- `showSettingsTool`
- `toolDrawer`
- `toolDrawerToolbar`
- `criticalVehicleMessagePopup`
- `indicatorDrawer`
- `windowedAnalyzePage`

问题：

- `toolDrawer` 仍是整屏实心 `qgcPal.window`。
- `toolDrawerToolbar` 仍是旧式顶部条。
- `indicatorDrawer` 仍是单层 opaque/opacity 旧弹窗结构。
- 严重告警弹窗有独立黄色样式，后续需要保留警告语义但统一圆角、边框、阴影。

建议：

- 抽一个通用全屏工具页壳层，比如 `QGCToolPageFrame.qml`，Analyze / Setup / Settings 共用。
- `toolDrawerToolbar` 改为统一顶部退出条，不直接复用 Fly View 顶栏。
- `indicatorDrawer` 改为统一弹出层样式，保留 `showIndicatorDrawer()` 逻辑。

### 2. 基础 Palette

文件：

- `src/QmlControls/QGCPalette.cc`

当前关键暗色：

- `windowShadeLight`: `#26292d / #2b2f34`
- `windowShade`: `#181a1d / #1d2024`
- `windowShadeDark`: `#101214 / #121416`
- `button`: `#17191c / #202327`
- `buttonHighlight`: `#25282c / #2d3237`
- `primaryButton`: `#00afff`
- `toolbarBackground`: `#0b0c0d`
- `groupBorder`: `#454b52 / #4c535a`

问题：

- 主色已经偏中性，但 `primaryButton` 还是高饱和蓝，后续如果到处用会变成刺眼主题色。
- `groupBorder` 是实色灰，用作大面积 divider 时仍显脏。
- QML 里很多地方直接用 `windowShade` 做实体底，透明/毛玻璃不会自动传导。

建议：

- 保留 `primaryButton` 只给主操作。
- 增加或复用更低存在感的边界色，不让 `groupBorder` 直接当页面大分割线。
- 如果需要半透明页面背景，不能只靠 `QGCPalette.cc`，还要改容器背景。

### 3. 共享按钮与菜单

文件：

- `src/QmlControls/QGCButton.qml`
- `src/QmlControls/QGCToolBarButton.qml`
- `src/QmlControls/SettingsButton.qml`
- `src/QmlControls/ConfigButton.qml`
- `src/QmlControls/SubMenuButton.qml`
- `src/QmlControls/QGCMenu.qml`
- `src/QmlControls/QGCMenuItem.qml`
- `src/QmlControls/QGCMenuSeparator.qml`

问题：

- `SubMenuButton` 使用 `qgcPal.windowShade` 实底，是 Analyze 左侧旧感的主要来源。
- `SettingsButton/ConfigButton` 风格接近，但 hover/checked 仍偏旧。
- `QGCMenu.qml` 目前几乎是空壳，菜单风格主要来自 Qt 默认/子项，容易割裂。
- `QGCToolBarButton` 已改过，但它主要服务顶部入口，不覆盖设置页侧栏。

建议：

- 统一侧边导航按钮：Analyze 的 `SubMenuButton`、Settings 的 `SettingsButton`、Setup 的 `ConfigButton` 使用同一套底色/选中态/图标尺寸/左侧 active 标识。
- 菜单和下拉弹层补统一背景、圆角、低透明边框。

### 4. 表单控件

文件：

- `src/QmlControls/QGCTextField.qml`
- `src/QmlControls/QGCComboBox.qml`
- `src/QmlControls/QGCCheckBox.qml`
- `src/QmlControls/QGCSwitch.qml`
- `src/QmlControls/QGCSlider.qml`
- `src/FactSystem/FactControls/*`

问题：

- 参数页、设置页、固件配置页大量依赖这些控件。
- `QGCCheckBox` 当前白底黑勾，在暗色系统里突兀。
- `QGCSlider` 填充使用 `qgcPal.colorBlue`，可能和新主题冲突。
- `QGCComboBox` popup 背景仍是 `qgcPal.window`，边框较硬。

建议：

- 先改共享控件，再看具体页面。
- 表单输入应统一为深灰半透明/中性边框/清晰文本。
- check/switch/slider 的 active 色可以用主题色，但别把主题色用于分割线。

### 5. 分组、弹窗、抽屉

文件：

- `src/QmlControls/SettingsGroupLayout.qml`
- `src/QmlControls/QGCGroupBox.qml`
- `src/QmlControls/QGCPopupDialog.qml`
- `src/QmlControls/QGCSimpleMessageDialog.qml`
- `src/QmlControls/ToolIndicatorPage.qml`
- `src/QmlControls/QGCFileDialog.qml`

问题：

- `SettingsGroupLayout` 当前默认有外边框和行分割线，是设置页“旧表格感”的主要来源。
- `QGCPopupDialog` 是实心 `windowShade` 外框 + `window` 内容区。
- `ToolIndicatorPage` expanded 分割线使用 `groupBorder` 实线。
- `QGCGroupBox` 使用实体 `windowShade` 背景。

建议：

- `SettingsGroupLayout` 改为轻量分组：弱背景、弱边界、减少内部硬线。
- `QGCPopupDialog` 改为统一浮层：遮罩 + 小圆角 + 中性暗底 + 低透明边框。
- `ToolIndicatorPage` 保留 expanded 能力，但 divider 要极弱或改为空间分隔。

## P1：四个大页面容器统一

### 1. Application Settings

入口：

- `src/QmlControls/AppSettings.qml`
- `src/UI/AppSettings/SettingsPagesModel.qml`

页面：

- General
- Fly View
- Plan View
- Video
- Telemetry
- ADSB Server
- Comm Links
- Maps
- PX4 Log Transfer
- Remote ID
- Console
- Help
- Mock Link
- Debug
- Palette Test

问题：

- 外层仍是 `Rectangle { color: qgcPal.window }`。
- 左侧列表 + divider + 右侧内容结构和 Analyze/Setup 相似，但三套写法不统一。
- 大量页面使用 `SettingsGroupLayout`，所以应先改共享分组。

建议：

- 改 AppSettings 外层为统一工具页壳层。
- 左侧导航套统一 `SettingsButton` 风格。
- `SettingsGroupLayout` 改完后，General/Video/Map/Telemetry 等会自动跟随。

### 2. Analyze Tools

入口：

- `src/AnalyzeView/AnalyzeView.qml`
- `src/API/QGCCorePlugin.cc`

页面：

- `LogDownloadPage.qml`
- `GeoTagPage.qml`
- `MAVLinkConsolePage.qml`
- `MAVLinkInspectorPage.qml`
- `VibrationPage.qml`

问题：

- 外层和 AppSettings 类似，也是实心背景 + 左侧按钮 + divider。
- 页面内有不少手写 `Rectangle`、`red`、`windowShade`。
- MAVLink Console/Inspector 的列表、表格、文本区域需要专门处理。

建议：

- 先和 AppSettings 共用同一工具页壳层。
- 左侧 `SubMenuButton` 改为统一侧边导航。

## 当前推进记录

### 2026-07-04 续作

已继续从共享基座和高暴露配置页推进，而不是只改 Fly View 小局部：

- `SetupPage.qml` 增加统一的轻量标题面板、圆角、低透明中性边界，禁用遮罩改为中性暗遮罩。
- `ParameterEditor.qml` 重做参数页头部、左侧分类栏和参数表外层，统一半透明中性面板、圆角、弱边界，并保留原参数控制器、搜索、修改过滤、工具菜单和参数编辑弹窗逻辑。
- `ParameterDiffDialog.qml` 增加统一参数差异表容器和表头强调，不改变发送 diff 的业务逻辑。
- `QGCRadioButton.qml`、`FactValueSlider.qml`、`FactSliderPanel.qml`、`AxisMonitor.qml`、`RCChannelMonitor.qml`、`QGCTabBar.qml`、`ClickableColor.qml` 等共享控件清理旧白底/黑边/实体旧底。
- `PX4`、`APM`、`Common` 下的高暴露固件配置页清理 `qgcPal.window/windowShade/windowShadeDark` 旧实体底色，改成中性半透明面板；Airframe/Sub frame/Actuator/ESP8266 等关键卡片补圆角和弱边界。
- `OfflineMapEditor.qml`、`RemoteIDIndicatorPage.qml`、`InstrumentValueEditDialog.qml`、`MissionItemStatus.qml`、`TerrainStatus.qml`、`LogReplayStatusBar.qml`、`MAVLinkChart.qml` 等页面/弹层同步为当前中性黑灰风格。

验证：

- 已运行 `tools\build-qgc-windows.ps1 -Action build`，第二次构建通过。
- 第一次构建暴露出机械替换导致的 `Qt.rgba(... )Shade` QML 语法错误，已修复并重新构建通过。

汉化审计：

- `translations/qgc_zh_CN.ts` 当前 3160 条消息，未完成或空翻译 3160 条。
- `translations/qgc_json_zh_CN.ts` 当前 796 条消息，未完成或空翻译 796 条。
- 后续汉化应走现有 Qt Linguist `.ts` 资源机制；不要在 QML 中硬编码中文。
- Console/Inspector 保持信息密度，不做过度卡片化。

### 3. Vehicle Configuration

入口：

- `src/Vehicle/VehicleSetup/SetupView.qml`

动态来源：

- `activeVehicle.autopilotPlugin.vehicleComponents`

固定入口：

- Summary
- Optical Flow
- Joystick / Buttons
- Parameters
- Firmware

动态固件组件：

- PX4/APM/Common 的 Airframe、Sensors、Radio、Flight Modes、Power、Safety、Motors、Actuators、PID Tuning、Camera、Follow、Lights、Sub Frame、WiFi Bridge、Syslink 等。

问题：

- 外层仍是旧式实心背景 + divider。
- 空状态/缺参数状态用 `windowShade` 实底。
- 动态组件很多，不能逐个硬改导航。

建议：

- `SetupView.qml` 用同一工具页壳层。
- `ConfigButton` 与 `SettingsButton/SubMenuButton` 统一。
- `SetupPage.qml` 作为所有固件配置页的内层基座先改，能覆盖大部分页面。

### 4. Plan View

入口：

- `src/QmlControls/PlanView.qml`
- `src/QmlControls/PlanViewToolBar.qml`
- `src/QmlControls/PlanEditToolbar.qml`

子模块：

- Mission
- GeoFence
- Rally Points
- Survey
- Structure Scan
- Corridor Scan
- Simple Item Editor
- Mission Settings
- FW/VTOL Landing Pattern
- KML/SHP 导入

问题：

- Plan 顶栏仍是旧 `toolbarBackground`，并有黑色底部分割线。
- 左侧/右侧规划工具、任务编辑器和地图控件混杂，改动风险高。
- 这里同时承载地图交互和编辑表单，不能简单套 Fly View 的浮窗。

建议：

- 先改 Plan 顶栏和编辑工具条，不动 mission 逻辑。
- 任务编辑器优先从 `MissionItemEditor`、`SimpleItemEditor`、`MissionSettingsEditor`、`TransectStyleComplexItemEditor` 这几个共享编辑器下手。
- 地图 item 的白色/黑色 marker 暂时不要先动，避免影响可见性。

## P2：业务子模块逐步统一

### Fly View 子弹窗和辅助面板

文件：

- `src/FlightDisplay/PreFlight*.qml`
- `src/FlightDisplay/*Checklist.qml`
- `src/FlightDisplay/FlyViewAdditionalActions*.qml`
- `src/FlightDisplay/VehicleWarnings.qml`
- `src/FlightDisplay/GuidedAction*.qml`
- `src/FlightDisplay/OnScreenGimbalController.qml`
- `src/FlightDisplay/VirtualJoystick.qml`

建议：

- 主页面已接近目标，下一步要同步 checklist、Additional Actions、Guided confirm 弹窗。
- 不改 guided controller，只改承载 UI。

### 地图和仪表控件

文件：

- `src/FlightMap/FlightMap.qml`
- `src/FlightMap/MapItems/*`
- `src/FlightMap/Widgets/*`

建议：

- 罗盘/姿态已改过一部分，但 `HorizontalCompassAttitude`、`VerticalCompassAttitude`、`PhotoVideoControl`、地图比例尺仍需统一。
- Mission/Polygon/Polyline marker 以可见性优先，不能为了统一主题导致地图上看不清。

### 参数系统

文件：

- `src/QmlControls/ParameterEditor.qml`
- `src/QmlControls/ParameterEditorDialog.qml`
- `src/QmlControls/ParameterDiffDialog.qml`
- `src/FactSystem/FactControls/*`

建议：

- 参数表要保持高密度，不要做大卡片。
- 重点改表格 header、搜索、行选中、编辑弹窗、diff 弹窗。

### 视频、相机、云台

文件：

- `src/FlightDisplay/QGCVideoBackground.qml`
- `src/FlightDisplay/FlyViewVideo.qml`
- `src/FlightMap/Widgets/PhotoVideoControl.qml`
- `src/UI/AppSettings/VideoSettings.qml`
- `src/UI/toolbar/GimbalIndicator.qml`

建议：

- 视频控件应使用低遮挡半透明背景。
- 拍照/录像按钮保留红色语义，但容器要跟新主题一致。

### Remote ID / UTMSP

文件：

- `src/UI/AppSettings/RemoteIDSettings.qml`
- `src/UI/toolbar/RemoteIDIndicator.qml`
- `src/UTMSP/*`

建议：

- 保留绿/红/黄状态语义。
- 状态条、通知滑出、地图多边形控件后续单独看。

### 3D Viewer

文件：

- `src/Viewer3D/Viewer3DQml/*`

建议：

- 优先级低于设置/规划/车辆配置。
- 只需要同步入口、进度条、外层背景，不要先动 3D 模型资源。

### 首次运行与文件对话框

文件：

- `src/FirstRunPromptDialogs/*`
- `src/QmlControls/QGCFileDialog.qml`

建议：

- 跟随 `QGCPopupDialog` 和表单控件同步。
- 文件对话框要注意 Windows 原生弹窗不完全可控。

## 推荐执行顺序

1. P0-1：改 `MainWindow.qml` 的 `toolDrawer`、`indicatorDrawer`、严重告警弹窗外观。
2. P0-2：统一 `SettingsButton`、`ConfigButton`、`SubMenuButton`。
3. P0-3：统一 `SettingsGroupLayout`、`QGCGroupBox`、`QGCPopupDialog`、`ToolIndicatorPage`。
4. P0-4：统一基础表单控件：TextField、ComboBox、CheckBox、Switch、Slider。
5. P1-1：改 `AppSettings.qml` 容器，抽出可复用工具页壳层。
6. P1-2：改 `AnalyzeView.qml` 容器，套同一壳层。
7. P1-3：改 `SetupView.qml` 和 `SetupPage.qml`，覆盖车辆配置体系。
8. P1-4：改 `PlanViewToolBar.qml`、`PlanEditToolbar.qml` 和 Plan 编辑器基座。
9. P2：再处理 checklist、视频、Remote ID、UTMSP、3D Viewer、个别固件页面。

## 验证策略

- P0 每完成一组共享控件构建一次。
- P1 每完成一个大容器构建一次，并至少截图验证 Fly / Plan / Analyze / Settings / Setup。
- 不需要每改一个按钮就截图。
- 视觉截图重点看 PC 最大化，不用小窗驱动布局。
- 不提交 commit。

## 2026-07-04 续推进记录

本轮已完成：

- 构建验证通过：`.\tools\build-qgc-windows.ps1 -Action build`。
- 截图脚本增强：启动提示的 OK 按钮识别从绿色扩展到当前蓝色主按钮，并在识别后追加 Enter 兜底。
- `AppSettings.qml`、`AnalyzeView.qml`、`SetupView.qml` 增加统一的弱页面底和侧栏底，减少纯黑空壳感。
- `SettingsPage.qml` 内容从居中孤岛改为靠左工作区布局，PC 端设置页更像工具界面。
- `VehicleSummary.qml` Summary 卡片增加圆角和裁剪，减弱旧方框感。
- `IntegratedCompassAttitude.qml` 默认罗盘底色、外圈和姿态弧改为更轻的中性灰，缓解 Fly View 右侧旧黑盘感。
- `PlanView.qml` 右侧编辑面板背景改为单层 `GlassBackdrop`，和 Fly View 浮层语言靠拢。

本轮截图：

- `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-pass-postpatch-right-open.png`
- `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-pass-postpatch-settings.png`
- `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-pass-postpatch-plan-clean2.png`

仍需继续：

- Plan View 左侧文件面板、右侧任务编辑器、底部 TerrainStatus 仍有旧线框/旧层级。
- Settings/Analyze/Setup 已统一底色，但页面密度和内容组织还不够像同一套产品。
- Fly View 右侧仪表盘虽然配色已轻一些，但形态仍偏旧，需要后续重画仪表结构。
- 汉化仍需走 `.ts/.qm` 翻译资源，不能继续在 QML 里硬编码中文动作文案。

## 2026-07-05 续推进记录

本轮已完成：

- 清理 Fly View 底部多机动作的 QML 硬编码中文：
  - `qsTr("解锁")` -> `qsTr("Arm")`
  - `qsTr("上锁")` -> `qsTr("Disarm")`
  - `qsTr("开始")` -> `qsTr("Start")`
  - `qsTr("暂停")` -> `qsTr("Pause")`
- 已确认 QGC 实际加载源代码翻译资源：
  - `QGCApplication.cc` 加载 `qgc_source_*.qm`
  - `JsonHelper` 加载 `qgc_json_*.qm`
  - 不是 Fly View 文案的主要路径 `qgc_*.qm`
- 已在 `translations/qgc_source_zh_CN.ts` 为 `FlyViewBottomRightRowLayout` 增加上述 4 条中文翻译。
- 构建时 `qgc_source_zh_CN.qm` 已重新生成，完成翻译数增加。
- Plan View 旧层级继续收敛：
  - `TerrainStatus.qml`、`MissionItemStatus.qml` 改为背景自身半透明，取消整控件 opacity，降低边框和网格线存在感。
  - `DropPanel.qml`、`ToolStripDropPanel.qml` 降低实心黑底强度，减轻 Plan 左侧文件面板/弹出菜单压地图的问题。

本轮截图：

- `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-i18n-flyview-source-clean.png`
- `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-plan-panel-cleanup.png`

仍需继续：

- Fly View 顶栏、底栏、左栏已经能作为主风格基准，但右侧仪表结构还需要进一步重画，不能只停留在配色调整。
- Plan View 左侧创建计划/文件面板仍偏重，右侧任务编辑器还需要重排密度与层级。
- Settings/Analyze/Setup 还有较多英文缺口；后续汉化应优先补 `qgc_source_zh_CN.ts` 与 `qgc_json_zh_CN.ts` 的高暴露页面，不要硬编码 QML 中文。

## 2026-07-05 右侧仪表与汉化续推进

本轮已完成：

- `FlyViewTopRightPanel.qml` 中部不再嵌旧 `FlyViewInstrumentPanel`，改成读取当前车辆真实 `heading/roll/pitch` Fact 的自绘中性灰姿态/航向盘。
- Fly View 残留偏青交互色已收回到中性黑灰：
  - `FlyViewToolBar.qml`
  - `FlyViewBottomRightRowLayout.qml`
  - `FlyViewTopRightPanel.qml`
  - `ToolStripHoverButton.qml`
  - `FlightModeIndicator.qml`
  - `MultiVehicleList.qml`
  - `TelemetryValuesBar.qml`
- 车辆类型仍来自 QGC/MAVLink 的真实类型字符串，不覆盖用户自定义名；中文只通过翻译资源展示。
- `FlyViewTopRightPanel.qml`、`FlyViewBottomRightRowLayout.qml`、`FlyViewToolStripActionList.qml` 增加显示层 `flightModeDisplayText`，将实时 flight mode 做 `qsTr` 显示映射，但设置飞行模式时仍使用原始 `modelData`。
- `translations/qgc_source_zh_CN.ts` 补充 Fly View 顶栏、右侧面板、底部车辆条、模式名、MainStatusIndicator、MAVLink vehicle type 的高暴露中文翻译。
- `translations/qgc_zh_CN.ts` 补充部分重复来源，避免加载顺序导致高暴露入口回落英文。

验证：

- 构建通过：`.\tools\build-qgc-windows.ps1 -Action build`。
- `qgc_source_zh_CN.qm` 已重新生成，完成翻译数增加到 2009。
- 最大化截图验证右侧仪表与汉化：
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-right-instrument-neutral-open.png`
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-i18n-right-panel-open.png`
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-i18n-flightmode-open.png`

仍需继续：

- 顶栏 `MainStatusIndicator` 已补翻译资源，但最后一张截图是在补 `Ready/MR(vtol)` 翻译前截的；后续可在下一批截图里顺带确认。
- Plan View 左侧创建/文件面板、任务编辑器和底部状态条还需要继续统一密度与层级。
- Settings/Analyze/Setup/固件配置页仍有大量旧布局和英文缺口，需要按页面继续补 `.ts/.qm`，不要硬编码中文。

## 2026-07-05 系统页与高暴露汉化批次

本轮已完成：

- Plan View 右侧编辑器继续减层级，多个任务/围栏/集结点/测绘编辑器的子背景改为透明，保留右侧大面板的单层 `GlassBackdrop` 作为主要承载，避免小面板重复叠黑底。
- Setup 相关提示中原先拼接的英文裸字符串改为 `qsTr(...)`，继续走 QGC 翻译资源，不在 QML 里硬编码中文。
- `UnitsSettings.cc` 的速度单位枚举从硬编码英文改为 `UnitsSettings::tr(...)`，设置页速度单位现在可显示为“米/秒”等中文。
- `APMPowerComponentSummary.qml` 对电池监测枚举里的 `Disabled/Enabled` 增加显示层翻译，不改参数原始值和飞控逻辑。
- `translations/qgc_source_zh_CN.ts`、`translations/qgc_zh_CN.ts`、`translations/qgc_json_zh_CN.ts` 补了一批高暴露中文：
  - Settings 侧栏、General/Units、按钮、弹窗基础按钮。
  - Analyze 页面按钮/表头/入口。
  - Setup summary、单位、禁用/启用状态。

验证：

- `git diff --check` 通过，仅剩工作区已有 LF/CRLF 提示。
- 三个中文 TS 文件均可被 XML 解析。
- 构建通过：`.\tools\build-qgc-windows.ps1 -Action build`。
- 最新构建生成：
  - `qgc_source_zh_CN.qm`: 3477 条翻译，2179 finished。
  - `qgc_zh_CN.qm`: 3161 条翻译，205 finished。
- 本轮最大化截图：
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-batch-20260705-settings-units-zh.png`
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-batch-20260705-analyze.png`
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-batch-20260705-setup-disabled-zh3.png`

仍需继续：

- Setup summary 里仍有来自飞控参数枚举或 MAVLink/ArduPilot 元数据的英文，如 `Manual`、`RTL`、`Primary, External`、`Analog Voltage and Current`，不能粗暴翻译原始参数值，后续需要做安全的显示层映射。
- Settings 具体子页、参数编辑器、固件升级页、Plan View 文件/创建任务弹层仍有英文和旧密度，需要继续按页面收敛。
- Fly View 首页当前可作为主风格基准，但仍需继续对右侧仪表、顶部信息密度和左侧长文本截断做精修。

## 2026-07-06 Plan View 与毛玻璃中性化续推进

本轮已完成：

- 将 `GlassBackdrop` 默认和 Fly View/Plan View 显式使用的背景采样饱和度下调，避免半透明毛玻璃在绿地/道路地图上被采样成明显青绿或橄榄偏色。
- `PlanView.qml` 右侧任务编辑面板改为更中性的黑灰玻璃底；`TerrainStatus.qml` 降低底部高度曲线面板背景、边框和网格线强度，减少底部旧式实底层级感。
- `PlanView.qml` 新增计划模板名称的显示层翻译函数，只改可见文案，不改 `PlanCreator`/复杂任务项原始名称和创建逻辑。
- 补充 Plan View 高暴露汉化：`Exit Plan`、`Mission Start`、`All Altitudes`、`Mixed Modes`、`Initial Waypoint Alt`、`Powered by %1`、`Dist prev WP:`、`Empty Plan`、`Survey`、`Corridor Scan`、`Structure Scan` 等。
- 截图脚本继续增强启动弹窗清理逻辑，增加常见 OK 按钮兜底点击，减少自动截图被连接提示弹窗遮挡的概率。

验证：

- XML 解析通过：`qgc_source_zh_CN.ts`、`qgc_zh_CN.ts`。
- `git diff --check` 通过，仅剩工作区已有 LF/CRLF 提示。
- 构建通过：`.\tools\build-qgc-windows.ps1 -Action build`。
- 最大化截图验证：
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-20260706-fly-dialog-clean2.png`
  - `build\qgc-v5.0.8-ui\Debug\qgc-ui-goal-20260706-plan-plancreator-display.png`

仍需继续：

- Plan View 的创建计划/文件面板仍有黑底层级，下一批应继续减少缩略图卡片和分组分割线的旧式边界感。
- Analyze/Settings/Setup 的子页面还需要继续补高暴露英文和局部旧控件密度。
- 汉化仍应优先走 `qsTr`、`.ts`、`.qm` 和显示层映射，不能为了中文覆盖飞控参数、任务类型或固件返回的原始值。
