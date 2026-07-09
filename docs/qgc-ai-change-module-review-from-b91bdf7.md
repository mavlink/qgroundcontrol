# QGC AI 修改模块化审查方案

日期：2026-07-08

## 目的

本文件用于审查 `b91bdf7cad67ba75f769ec985f51a03e1fb9ddff..HEAD` 之间由 AI 辅助生成的 QGroundControl 修改。

审查目标不是证明 AI 修改都错，也不是继续为已有修改找理由，而是判断这些修改在当前 QGC 项目中是否满足三个条件：

1. 服务于“让 QGC UI 更易用”的目标。
2. 没有引入不必要的飞控行为、状态同步、数据丢失或回归风险。
3. 后续维护者能理解、验证、局部修复或局部回退。

主审查方式采用“按修改涉及模块审查”。提交记录只作为追溯来源和回退粒度参考，不作为审查主线。

## 审查对象快照

基线：

```text
b91bdf7cad67ba75f769ec985f51a03e1fb9ddff
```

当前范围：

```text
b91bdf7cad67ba75f769ec985f51a03e1fb9ddff..HEAD
```

当前已识别提交：

```text
551339a99 修改产品图标
db6cd0bf8 补齐飞行界面中文翻译
9aed0bf30 同步规划页活动设备并调整工具栏
2454ac354 优化飞行态势和地图信息显示
1c25768f2 重整飞行底部操作托盘
da26c5db2 统一基础控件和返回导航
238865675 添加飞行视图与导航资源
93416e687 补齐中文翻译
b8ed47437 统一字体
eaeac2e3d 统一规划分析周边界面样式
24c348123 统一配置和设置页界面样式
d6d4f4437 重设计飞行主界面布局
8c29239ca 重构通用界面控件样式
d31972ad1 补充界面重设计记录和截图工具
1c14ad260 完善中文翻译文案
725a9eeb0 修改参数跑通流程
```

当前规模：

```text
206 files changed, 10589 insertions(+), 3908 deletions(-)
```

这个规模已经超过普通 UI 微调，应按模块做系统审查。

## 什么算问题

只有满足下面至少一条，才应记录为问题。不要因为“改动大”“风格和原版不同”“是 AI 写的”就直接判定为问题。

### P0：必须优先处理的问题

- 可能导致飞控命令误触发、发错对象、发错模式或发错车辆。
- 可能导致 Plan、任务、围栏、集结点等用户数据被静默覆盖或丢失。
- 可能导致连接、断连、解锁、模式切换、返航、降落、暂停等高风险路径行为改变且无清晰保护。
- 会造成启动崩溃、页面无法加载、QML 运行时硬错误或 C++ 编译失败。
- UI 显示与实际控制对象不一致，例如界面显示 A 机但操作落到 B 机。

### P1：应当修复或隔离的问题

- 为了 UI 易用性而修改了飞控、固件插件、车辆事实、单位设置等非 UI 行为，但没有明确必要性。
- 新布局在常见窗口尺寸下遮挡核心地图、视频、虚拟摇杆、告警或关键操作。
- 关键操作入口变得更难发现，或危险操作失去确认、状态提示、禁用条件。
- active vehicle、selected vehicle、manager vehicle、dirty plan 等状态边界不清。
- 共享控件改动影响全项目，但只服务于某个页面的局部视觉效果。

### P2：需要排期处理的问题

- 模块内形成难维护的大型 QML 文件，且职责混杂到难以局部修改。
- 同一视觉语义在多个文件中重复硬编码，后续调整需要多处同步。
- 大量固定宽高缺少溢出、隐藏、滚动或最小窗口策略。
- 毛玻璃、Canvas、ShaderEffectSource 等效果无边界使用，可能影响低性能机器或视频场景。
- 翻译、图标、状态颜色语义不一致，可能造成误读但不直接导致危险操作。

### P3：记录但不阻塞的问题

- 文案不够自然。
- 间距、圆角、色彩、图标尺寸存在局部不一致。
- 文档或脚本命名不够清晰。
- 可读性可以改善，但当前不会影响用户路径或维护边界。

## 什么不算问题

以下情况不能单独作为问题，除非能进一步证明它造成了上面的 P0/P1/P2 风险。

- 文件改动行数多。
- UI 风格不再像原版 QGC。
- 旧控件被替换成新控件。
- 为了触控目标、扫读效率或状态分组而引入最小宽度、最小高度。
- 为了中文用户降低理解成本而采用意译，而不是逐字直译。
- 为了避免误操作，将入口重新分组、移动或收纳。
- 为了使 Fly/Plan/Setup/Analyze 风格统一而修改共享控件。
- 为了显示友好，将飞控内部模式名映射成更易读文本，只要实际写回值仍保持原始控制值。
- 为了多机易用性，点击载具时同步 active vehicle，只要界面清楚提示且不会静默覆盖用户未保存工作。

## 审查结论类型

每个模块或问题最终只给下面四种处理建议之一。

| 结论 | 含义 |
| --- | --- |
| 保留 | 目标清楚、行为合理、风险可接受，只需常规验证。 |
| 小修 | 方向正确，但有局部 bug、溢出、文案、条件判断或可维护性问题。 |
| 重构 | 目标有价值，但当前实现职责混杂、难验证或后续维护成本过高。 |
| 回退 | 目标不清、收益不足、风险高，或明显偏离 UI 易用性边界。 |

不要用“看起来不优雅”作为回退理由。回退必须说明具体用户风险、行为风险或维护风险。

## 模块审查顺序

### 0. 范围门禁：非 UI 行为改动

代表文件：

- `src/FirmwarePlugin/APM/APMFirmwarePlugin.*`
- `src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.*`
- `src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.cc`
- `src/MissionManager/PlanMasterController.*`
- `src/Settings/UnitsSettings.cc`
- `src/Vehicle/FactGroups/VehicleFactGroup.h`
- `src/QGCApplication.cc`
- `src/VideoManager/VideoReceiver/QtMultimedia/UVCReceiver.cc`

审查问题：

- 这个改动是否真的支撑 UI 易用性？
- 是否改变飞行模式列表、车辆状态、单位、初始化、视频行为或任务同步语义？
- 是否有旧逻辑保护被移除？
- 是否能被 UI 层替代实现？

可接受例子：

- 为了 Plan 页进入时同步当前 active vehicle，且 dirty plan 有保护。
- 为了暴露 UI 所需的只读状态，且不改变飞控命令语义。

应记录为问题的例子：

- UI 改造顺手改变固件模式列表或飞控能力判断。
- 为了显示方便而改变真实 Fact 值或单位换算。
- 自动同步任务时绕过 dirty plan 提示。

### 1. 全局设计系统与共享控件

代表文件：

- `src/QmlControls/QGCButton.qml`
- `src/QmlControls/QGCCheckBox.qml`
- `src/QmlControls/QGCComboBox.qml`
- `src/QmlControls/QGCMenu*.qml`
- `src/QmlControls/QGCPalette.cc`
- `src/QmlControls/ScreenTools.*`
- `src/QmlControls/GlassBackdrop.qml`
- `src/QmlControls/ToolStrip*.qml`
- `src/QmlControls/QGCToolPageFrame.qml`

审查问题：

- 共享控件是否仍保持原组件语义？
- 是否为了一个页面的视觉效果影响所有页面？
- hover、pressed、disabled、checked、warning、danger 状态是否仍可区分？
- `GlassBackdrop` 是否有降级或边界，不会到处制造性能风险？

可接受例子：

- 统一按钮视觉，但保留 enabled、checked、hover、pressed 的语义。
- 新增通用页面壳层，减少逐页硬改。

应记录为问题的例子：

- 全局按钮禁用态不明显，导致用户分不清能否点击。
- 全局文本大小改变导致参数页、设置页大量溢出。

### 2. 主窗口与导航

代表文件：

- `src/UI/MainWindow.qml`
- `src/QmlControls/NavigationBackButton.qml`
- `src/QmlControls/PlanViewToolBar.qml`
- `src/QmlControls/SetupPage.qml`
- `src/QmlControls/AnalyzePage.qml`

审查问题：

- Fly、Plan、Setup、Analyze、Settings 的入口和返回是否符合用户预期？
- `allowViewSwitch()` 等原有防误切换逻辑是否保留？
- 返回按钮是否只改变导航，不混入业务操作？

可接受例子：

- 新增统一返回按钮，使工具页返回路径更清楚。

应记录为问题的例子：

- 返回按钮绕过未保存提示。
- 抽屉或工具页关闭后状态残留，导致下次进入错页。

### 3. Fly 页顶部状态栏

代表文件：

- `src/QmlControls/FlyViewToolBar.qml`
- `src/QmlControls/MainStatusIndicator.qml`
- `src/QmlControls/FlyViewToolBarIndicators.qml`

审查问题：

- 顶部是否只承载全局状态和关键遥测摘要？
- 电池、GPS、高度、速度、飞行时间是否在无数据、断连、多电池情况下仍清楚？
- 小窗口下是否有裁切、隐藏或优先级策略？
- 设置、消息、Analyze、Vehicle Config 入口是否仍可发现？

可接受例子：

- 把电池/GPS摘要前置，减少飞行中查找成本。

应记录为问题的例子：

- 顶栏在 1366x768 下挤压到关键按钮不可见。
- 电池告警颜色和正常状态颜色区分不明显。

### 4. Fly 页右上飞行态势面板

代表文件：

- `src/FlightDisplay/FlyViewTopRightPanel.qml`

审查问题：

- 面板是否帮助飞手快速扫读，而不是堆满指标？
- 姿态、航向、高度、速度、模式、车辆名、电池是否有清楚层级？
- 面板最小尺寸是否会遮挡地图和底部操作托盘？
- 视频全屏、无 active vehicle、多机场景是否有正确隐藏或降级？

可接受例子：

- 用一个态势面板集中替代分散的小控件，只要布局稳定且不遮挡核心操作。

应记录为问题的例子：

- 固定宽高导致小窗口下覆盖地图主要区域。
- 指标过密，飞行中无法快速区分主次。

### 5. Fly 页底部操作托盘与模式显示

代表文件：

- `src/FlightDisplay/FlyViewBottomRightRowLayout.qml`
- `src/FlightDisplay/FlyViewToolStripActionList.qml`
- `src/FlightDisplay/FlightModeDisplay.qml`
- `src/FlightDisplay/GuidedAction*.qml`
- `src/FlightDisplay/FlyViewAdditionalActions*.qml`

审查问题：

- 高频动作是否更近，低频入口是否不干扰？
- 非飞控入口和飞控动作是否明确分组？
- 危险动作是否仍保留确认、禁用条件和状态反馈？
- 模式显示名是否和真实写回值分离？
- overflow 滚动是否可发现，触控尺寸是否稳定？

可接受例子：

- 把 `Plan`、检查单、3D 视图作为非操作入口，和起飞/降落/返航分开。
- QuadPlane 模式显示更友好，但点击时仍写回原始 `vehicle.flightMode`。

应记录为问题的例子：

- 显示“继续”但实际触发的不是继续任务。
- 模式菜单排序或翻译让用户无法区分固定翼/VTOL语义。
- 小窗口下托盘滚动后关键危险动作不可见且无提示。

### 6. 多机与地图交互

代表文件：

- `src/FlightDisplay/MultiVehicleList.qml`
- `src/FlightMap/MapItems/VehicleMapItem.qml`
- `src/FlightDisplay/FlyViewWidgetLayer.qml`
- `src/FlightMap/FlightMap.qml`

审查问题：

- 用户是否能明确知道当前 active vehicle 是哪架？
- 地图点击、侧边列表点击、右上面板选择是否一致？
- 信息卡是否显示关键状态，而不是遮挡地图或造成误读？
- 多机场景下锁定、解锁、模式、任务操作是否不会落到错误车辆？

可接受例子：

- 点击地图载具时同步 active vehicle 并展开信息卡，减少选错对象的概率。

应记录为问题的例子：

- 信息卡 pin/unpin 状态和 active vehicle 状态相互误导。
- 侧边列表显示 A 为高亮，但底部操作实际作用于 B。

### 7. Plan 页同步与工具栏

代表文件：

- `src/QmlControls/PlanView.qml`
- `src/QmlControls/PlanToolBarIndicators.qml`
- `src/QmlControls/TerrainStatus.qml`
- `src/QmlControls/MissionItemEditor.qml`
- `src/MissionManager/PlanMasterController.*`

审查问题：

- 进入 Plan 页时同步 active vehicle 是否符合用户预期？
- dirty plan 是否不会被静默覆盖？
- 当前航点、距离、方位、高差、电池需求等指标是否清楚且不挤压编辑区？
- active vehicle 变化时，提示文案是否能让用户理解“保留当前计划”和“加载新飞机计划”的区别？

可接受例子：

- 如果当前 plan 已经属于 active vehicle 且 dirty，则保留未保存工作。

应记录为问题的例子：

- 切换 active vehicle 后自动加载新任务，导致本地未保存任务消失。
- 指标栏宽度过大，压缩地图或任务编辑区。

### 8. Setup、Analyze、参数和配置页面

代表文件：

- `src/AutoPilotPlugins/**`
- `src/QmlControls/ParameterEditor.qml`
- `src/QmlControls/ParameterDiffDialog.qml`
- `src/AnalyzeView/**`
- `src/UI/AppSettings/**`
- `src/Vehicle/VehicleSetup/**`

审查问题：

- 修改是否主要是统一页面壳层和控件风格？
- 是否改变参数写入、校准、固件升级、安全配置等操作流程？
- 旧页面中表单密度、错误提示、确认按钮是否仍可靠？

可接受例子：

- 页面视觉统一，但不改变参数读写流程。

应记录为问题的例子：

- 为了美化参数编辑器，弱化了修改状态或重启需求提示。
- 校准流程按钮顺序或确认语义被改乱。

### 9. 资源、品牌、图标

代表文件：

- `qgcresources.qrc`
- `resources/branding/**`
- `resources/flyview-*.svg`
- `resources/nav-back.svg`
- `resources/icons/**`
- `deploy/windows/**`

审查问题：

- qrc 注册是否完整？
- 图标语义是否和动作一致？
- 品牌图是否影响顶部状态栏空间？
- Windows 图标、firstframe、应用资源是否和产品命名一致？

可接受例子：

- 新增操作图标并集中注册，支持 Fly 页操作托盘。

应记录为问题的例子：

- 图标看起来像“暂停”，实际触发“继续”。
- 品牌图过大导致状态栏挤压。

### 10. 翻译、文档、工具脚本

代表文件：

- `translations/qgc_source_zh_CN.ts`
- `translations/qgc_zh_CN.ts`
- `translations/qgc_json_zh_CN.ts`
- `docs/**`
- `tools/capture-qgc-window.ps1`

审查问题：

- 中文是否降低理解成本，而不是改变操作含义？
- 危险动作、飞行模式、未保存提示是否准确？
- 文档是否记录真实设计约束，而不是为修改背书？
- 工具脚本是否只用于截图/验证，不改变构建或运行默认行为？

可接受例子：

- 对飞行界面高频动作做更自然的中文翻译。

应记录为问题的例子：

- 将具有安全语义的英文提示翻译成模糊或过度简化的中文。

## 审查流程

### 第一步：模块建账

为每个模块列出：

- 涉及文件。
- 涉及提交。
- 是否包含非 UI 行为改动。
- 预期用户收益。
- 必测用户路径。

### 第二步：静态审查

按模块执行：

```powershell
git -C qgroundcontrol diff b91bdf7cad67ba75f769ec985f51a03e1fb9ddff..HEAD -- <module-files>
```

静态审查只接受有证据的问题。证据可以是：

- 具体代码位置。
- 旧逻辑被删除或绕过。
- 状态来源不一致。
- 缺少空值、断连、无数据、多机、小窗口处理。
- 文案或图标语义与触发动作不一致。

### 第三步：运行验证

至少验证这些路径：

- 1366x768 窗口。
- 小于 1366 宽度的缩小窗口。
- 最大化窗口。
- 视频全屏和退出全屏。
- 单机连接、断连、重连。
- 双机或多机切换 active vehicle。
- Fly 页切换模式。
- 起飞、降落、暂停、继续任务、返航动作入口。
- 地图点击载具和侧边列表点击载具。
- Plan 页存在 dirty plan 时切换 active vehicle。
- 中文界面长文本、长飞机名、长模式名。

### 第四步：给出处理建议

每个问题记录为：

```text
模块：
文件：
提交来源：
等级：
问题：
为什么这是问题：
为什么不是可接受代价：
用户影响：
建议处理：保留 / 小修 / 重构 / 回退
验证方式：
```

其中“为什么不是可接受代价”是必填项，用于防止把有用修改误判为问题。

## 并行审查分工

如果使用多个审查智能体或多人并行，建议按模块拆分，而不是按提交拆分。

### 审查者 A：非 UI 行为边界

负责：

- `FirmwarePlugin`
- `MissionManager`
- `Vehicle`
- `Settings`
- `QGCApplication`
- `VideoReceiver`

输出：

- 哪些行为改动与 UI 目标无关。
- 哪些行为改动虽非 UI 但合理支撑用户路径。
- P0/P1 候选问题。

### 审查者 B：Fly 页核心操作台

负责：

- 顶部状态栏。
- 右上态势面板。
- 底部操作托盘。
- 模式显示。
- Fly View 布局 insets。

输出：

- 高频飞行路径是否变短。
- 小窗口和视频场景风险。
- 危险动作和模式切换风险。

### 审查者 C：多机、地图、Plan

负责：

- 多机列表。
- 地图载具信息卡。
- active vehicle 同步。
- Plan 页 dirty plan 和工具栏。

输出：

- 是否存在操作对象错配。
- 是否存在任务数据覆盖风险。
- 多机用户路径是否一致。

### 审查者 D：低风险收尾

负责：

- 翻译。
- 图标。
- 品牌资源。
- 文档。
- 截图脚本。

输出：

- 文案误导。
- 图标语义不一致。
- 资源注册缺失。
- 文档与实现不一致。

## 审查输出格式

建议最终生成一份结果文档：

```text
docs/qgc-ai-change-module-review-results-from-b91bdf7.md
```

建议结构：

1. 总结。
2. P0/P1 问题列表。
3. 按模块审查结论。
4. 保留的有价值修改。
5. 小修清单。
6. 重构清单。
7. 回退候选。
8. 运行验证记录。

特别要求：

- “保留的有价值修改”必须单独列出，避免审查文档只记录负面问题。
- 每个回退候选必须说明为什么小修不够。
- 每个 P0/P1 必须给出复现路径或代码证据。
- 每个“AI 写得不优雅”的判断必须转化成具体维护风险，否则不记录为问题。

## 当前建议的第一轮审查顺序

1. 非 UI 行为边界。
2. Plan 页 active vehicle 和 dirty plan。
3. Fly 页底部操作托盘和模式显示。
4. Fly 页右上态势面板和顶部状态栏。
5. 多机与地图载具交互。
6. 全局共享控件。
7. Setup/Analyze/参数页面样式统一。
8. 翻译、资源、文档、脚本。

这个顺序优先排除“安全和数据风险”，再审“易用性和视觉实现”，最后审“低风险一致性”。

## 第一轮重点审查清单

下面这些不是最终结论，而是第一轮必须优先验证的高风险假设。只有验证后才能判定为“保留、小修、重构、回退”。

### 1. APM 飞行模式与 FOLLOW_TARGET

涉及文件：

- `src/FirmwarePlugin/APM/APMFirmwarePlugin.cc`
- `src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.cc`
- `src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.h`
- `src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.cc`

必须验证：

- `FOLLOW_TARGET = 90` 是否是目标 AeroFollow 固件真实支持的 ArduPlane flight mode。
- 本地或目标参数枚举中是否存在对应 `FLTMODE` 值。
- 删除或隐藏 `AUTOLAND = 26` 是否会破坏现有运维或应急路径。
- Plane/Rover 模式下拉收窄是否是产品决策，而不是 AI 为了简化 UI 误删能力。

问题判定：

- 如果目标固件不支持 mode 90，而 UI 允许发送 mode 90，这是 P0/P1 问题。
- 如果模式收窄是明确产品决策，且应急/维护入口仍有替代路径，这不是问题。

### 2. QuadPlane 起飞能力与高度参数

涉及文件：

- `src/FirmwarePlugin/APM/APMFirmwarePlugin.cc`

必须验证：

- `Q_ENABLE > 0` 的 ArduPlane 是否应走 `Guided + Arm + MAV_CMD_NAV_TAKEOFF`。
- 未解锁、已解锁、GPS 不可用、参数未加载四种状态下是否都有清楚失败路径。
- QuadPlane 最小起飞高度是否取到正确参数；不能因为 `vehicle->vtol()` 判断不一致而误取 `PILOT_TKOFF_ALT` 或默认 3.048m。

问题判定：

- 如果 UI 显示可起飞但飞控拒绝或走错起飞模式，这是 P0/P1 问题。
- 如果 AeroFollow 固件确实定义这条起飞路径，且失败状态可理解，这是合理支撑，不应因为“改了非 UI 行为”直接回退。

### 3. Plan View 进入时自动同步

涉及文件：

- `src/MissionManager/PlanMasterController.cc`
- `src/MissionManager/PlanMasterController.h`
- `src/QmlControls/PlanView.qml`
- `src/UI/MainWindow.qml`

必须验证：

- 离线创建或加载计划，保存后回 Fly，再进 Plan，计划是否被清空。
- clean 但未上传的计划，在 active vehicle 存在时是否被当前车辆计划静默覆盖。
- dirty plan 是否确实保留，并且提示文案让用户理解“保留当前计划”和“加载新飞机计划”的区别。

问题判定：

- 无提示清空或替换用户当前计划是 P0/P1 问题。
- 进入 Plan 页自动跟随 active vehicle 本身不是问题；只有在覆盖用户工作或目标不清时才是问题。

### 4. Plan 地图点击载具是否意外改变计划上下文

涉及文件：

- `src/FlightMap/MapItems/VehicleMapItem.qml`
- `src/QmlControls/PlanView.qml`
- `src/MissionManager/PlanMasterController.cc`

必须验证：

- 在 Plan 页点击另一架飞机的信息卡，是否会切换 active vehicle。
- 切换 active vehicle 后，Plan controller 的 manager vehicle、当前计划内容、上传目标是否随之改变。
- 用户只是想看地图上另一架飞机信息时，是否会触发计划保留/丢弃提示或自动重载。

问题判定：

- 如果地图信息查看导致计划上下文被静默切换，这是 P1 问题。
- 如果产品设计就是“点飞机即切当前操作对象”，且界面清楚提示当前对象变化，这可以接受。

### 5. 多机选择语义和批量动作

涉及文件：

- `src/FlightDisplay/MultiVehicleList.qml`
- `src/FlightDisplay/FlyViewWidgetLayer.qml`
- `src/FlightDisplay/FlyViewTopRightPanel.qml`

必须验证：

- 基线中的多机选择、Select All、Deselect All、多机 Arm/Disarm/Start/Pause 是否仍是产品需要。
- 当前 UI 是否把“选择多机”替换成“切 active vehicle/打开详情面板”。
- `selectedVehicles` 相关逻辑是否还有有效入口。

问题判定：

- 如果批量控制是产品需求而入口消失，这是功能回退，不是视觉问题。
- 如果 AeroFollow 明确只保留单 active vehicle 操作，移除批量入口可以接受，但需要文档化。

### 6. 安全动作可用性是否被局部重写

涉及文件：

- `src/FlightDisplay/MultiVehicleList.qml`
- `src/FlightDisplay/FlyViewToolStripActionList.qml`
- `src/FlightDisplay/GuidedAction*.qml`

必须验证：

- Arm、Disarm、Takeoff、Land、RTL、Pause、Continue、Gripper 是否仍走原有确认流程。
- 通信丢失、飞行中上锁、RC RSSI 不满足、checklist 未通过、force arm 等边界是否和原 GuidedAction 规则一致。
- 动作显示、禁用、隐藏策略是否会让用户误以为功能不存在。

问题判定：

- 绕过确认或把动作发给错误车辆是 P0。
- 禁用动作改为隐藏本身不是问题，但若降低安全动作可发现性或让状态不可解释，应记录为 P1/P2。

### 7. 飞行模式显示与实际模式值

涉及文件：

- `src/FlightDisplay/FlightModeDisplay.qml`
- `src/FlightDisplay/FlyViewToolStripActionList.qml`
- `src/FlightDisplay/FlyViewBottomRightRowLayout.qml`

必须验证：

- Manual、Stabilize、Auto、Guided、AutoTune、QuadPlane、VTOL/FW 等模式是否不会被显示成互相混淆的标签。
- 模式列表排序是否只是降低认知成本，而不是隐藏必要模式。
- 点击模式时写回的仍是原始 `modelData`，而不是友好显示文本。

问题判定：

- 友好显示不是问题。
- 不同真实模式显示成同一个不可区分标签，导致用户可能选错，是 P1。

### 8. 右上态势面板单位和数据源

涉及文件：

- `src/FlightDisplay/FlyViewTopRightPanel.qml`

必须验证：

- 高度、速度、航向刻度的数值和单位是否一致。
- 如果标题显示用户单位，例如 ft、kts，刻度数值是否也按用户单位转换。
- NaN、无 GPS、无电池、断连情况下是否显示可理解的占位。

问题判定：

- 单位标签和实际数值不一致是 P1。
- 固定网格和密集指标只要可读、不会误导，可以作为设计取舍接受。

### 9. 顶部栏、底部托盘和布局避让

涉及文件：

- `src/QmlControls/FlyViewToolBar.qml`
- `src/FlightDisplay/FlyViewBottomRightRowLayout.qml`
- `src/FlightDisplay/FlyViewWidgetLayer.qml`
- `src/FlightDisplay/FlyViewTopRightPanel.qml`

必须验证：

- 1366x768、窄屏、高 DPI、触摸设备下关键按钮是否可见可点。
- 参数下载进度、断开按钮、消息入口、电池/GPS、模式、返航、降落等关键入口是否被裁剪。
- 视频全屏、PiP、虚拟摇杆、地图比例尺是否和新面板互相遮挡。
- 底部托盘 overflow 是否可发现且可操作。

问题判定：

- 关键安全按钮不可达是 P1。
- 面板变大但仍保留地图主画布、关键入口可用，是可接受代价。

### 10. UVC 视频与飞行时间 Fact

涉及文件：

- `src/VideoManager/VideoReceiver/QtMultimedia/UVCReceiver.cc`
- `src/Vehicle/FactGroups/VehicleFactGroup.h`

必须验证：

- UVC 路径是否是当前产品实际运行路径。
- 改动后视频画面是否存在比例错误、黑屏、尺寸振荡。
- `flightTime` 暴露给 QML 后，在断连、无车辆、重连场景是否不会产生未定义属性或 NaN 文本。

问题判定：

- 原整数除法或变量遮蔽修正可以是合理非 UI 支撑。
- 视频链路实际黑屏或振荡才是问题。

## 第一轮不应优先投入的内容

除非运行验证发现回归，以下内容不应在第一轮消耗主要审查预算：

- `UnitsSettings.cc` 中单位字符串补翻译。
- `QGCApplication.cc` 中全局字体设置。
- `VehicleSetup/*.qml` 的纯视觉样式。
- `PhotoVideoControl.qml` 的纯布局样式。
- 文档、截图脚本、品牌图、普通图标资源。

这些内容仍要审，但应排在安全、数据、操作对象和关键布局之后。
