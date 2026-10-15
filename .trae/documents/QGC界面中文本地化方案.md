# QGC 界面中文本地化（汉化）方案

## 一、摘要

目标：把 QGroundControl（本地基准 `v5.1.4`，分支 `learn/qgc-v5.1.4`）的界面文本全部汉化为简体中文。

经确认的范围与方式：

- **范围**：仅补齐两条 `.ts` 翻译管道（不做源码改造、不做繁体、不改默认语言）。
- **分工**：由 AI 批量翻译并写入 `.ts`，用户按批次抽查验收。
- **覆盖风险**：已知 `.ts` 会被上游 Crowdin / lupdate 覆盖，接受该风险，直接改本地文件。

工作量：`translations/qgc_source_zh_CN.ts` 待译 2876 条 + `translations/qgc_json_zh_CN.ts` 待译 1575 条，**合计 4451 条**。

## 二、当前状态分析

### 2.1 两条独立管道（必须都补，缺一不可）

| 管道 | 文件 | 内容来源 | 运行期加载 |
| --- | --- | --- | --- |
| C++/QML | `translations/qgc_source_zh_CN.ts` | `src/` 下 `tr()` / `qsTr()` 字符串 | `:/i18n/qgc_source_*.qm` |
| JSON 元数据 | `translations/qgc_json_zh_CN.ts` | `src/**/*.json` 的 `translateKeys` | `:/i18n/qgc_json_*.qm` |

依据：[QGCApplication.cc](file:///c:/Users/Admin/Desktop/QGC/src/QGCApplication.cc#L207-L216) 分别装载两个 translator；[CMakeLists.txt](file:///c:/Users/Admin/Desktop/QGC/CMakeLists.txt#L369-L389) 用 `qgc_*.ts` glob 生成 `.qm`。

**关键认知：设置页、参数说明、MAVLink 指令名、枚举选项等大量界面文本在 JSON 里，不在 QML 里。**设置页 QML 是构建期由 `*.SettingsUI.json` 生成的产物（[src/AppSettings/CMakeLists.txt](file:///c:/Users/Admin/Desktop/QGC/src/AppSettings/CMakeLists.txt#L19-L46)），所以"汉化设置界面"主要靠 json 管道。

### 2.2 现状统计（只读实测）

| 文件 | 行数 | `<context>` | `<message>` | 已译 | `type="unfinished"` |
| --- | --- | --- | --- | --- | --- |
| `translations/qgc_source_zh_CN.ts` | 22043 | 410 | 4046 | 1170 | **2876** |
| `translations/qgc_json_zh_CN.ts` | 11368 | 87 | 1821 | 246 | **1575** |

- 无复数形式（`<numerusform>` 命中 0 处），全部为 1:1 替换，无需处理单复数分支。
- 未完成条目形态：`<translation type="unfinished">Roll</translation>`（回填了英文原文，故界面显示英文）。
- `zh_TW` 两份文件完成度均为 0，且 `_rgLanguageInfo` 中无 `ChineseTaiwan` 项 → 本次不涉及。

### 2.3 条目格式（编辑对象）

```xml
    <message>
      <location filename="../src/AutoPilotPlugins/APM/APMAdvancedTuningCopterComponent.qml" line="27"/>
      <source>Roll</source>
      <translation type="unfinished">Roll</translation>
    </message>
```

改动仅两处：把 `<translation>` 的文本换成中文，**并去掉 `type="unfinished"` 属性**。
`<source>` / `<location>` / `<extracomment>` 一律不动（`<source>` 是查表键，改了会导致翻译失效）。

### 2.4 必须原样保留的内容（实测存在）

- **占位符**：`%1`、`%2`、`%n` 等（source 管道中大量存在）。
- **HTML 转义实体**：如 `&lt;b&gt;...&lt;/b&gt;`、`&lt;br /&gt;`、`&quot;`、含 URL 的 `&lt;a href=...&gt;`（实例：`.../APMSensorsComponent.qml`、`.../PX4 TuningComponent` 相关条目）。
- **XML 实体**：译文中的 `&` `<` `>` 必须写作 `&amp;` `&lt;` `&gt;`；引号按需 `&quot;`。
- 协议/单位/品牌等：`MAV_CMD_*`、`SYS_STATUS`、`m/s`、`°`、`PX4`、`ArduPilot`、`RTK`、`NTRIP`、`ADSB`、`ESC`、`RC1_MIN` 等。

### 2.5 行尾与格式约束（pre-commit 相关）

[.pre-commit-config.yaml](file:///c:/Users/Admin/Desktop/QGC/.pre-commit-config.yaml#L18-L34) 生效规则：

- `check-xml`（**不排除 translations/**）→ 编辑后必须仍是合法 XML，这是最廉价的验证手段。
- `end-of-file-fixer` 与 `trailing-whitespace` 均排除 `translations/`；但 `mixed-line-ending --fix=lf` **未排除 `.ts`** → 文件必须使用 **LF** 行尾。当前文件为 LF，编辑时须保持。

### 2.6 语言生效路径

- 界面入口：Settings → General → Language 下拉，定义于 [General.SettingsUI.json](file:///c:/Users/Admin/Desktop/QGC/src/AppSettings/pages/General.SettingsUI.json#L11-L15)。
- 中文选项来自 [AppSettings.cc](file:///c:/Users/Admin/Desktop/QGC/src/Settings/AppSettings.cc#L41) 的 `中文 (Chinese)`，属 release 语言（可选）。
- `QLocale::Chinese` 对应文件名 `zh_CN`，因此会加载 `qgc_source_zh_CN.qm` 与 `qgc_json_zh_CN.qm`。
- **切换语言后 JSON 侧需重启**（`qgcRebootRequired: true`，见 [App.SettingsGroup.json](file:///c:/Users/Admin/Desktop/QGC/src/Settings/App.SettingsGroup.json#L343-L351)），因为 JSON 字符串在加载时一次性翻译并固化进 `FactMetaData`。
- `en_US` 不加载任何翻译器，英文即源码原串（[QGCApplication.cc](file:///c:/Users/Admin/Desktop/QGC/src/QGCApplication.cc#L200)）。

## 三、术语表（强制统一，逐批复用）

### 3.1 核心术语

| 英文 | 统一译法 |
| --- | --- |
| Vehicle | 载具 |
| Ground Control Station / GCS | 地面站 |
| Mission | 任务 |
| Waypoint | 航点 |
| Home / Home Position | 返航点 |
| Takeoff / Land | 起飞 / 降落 |
| RTL (Return to Launch) | 返航 |
| Arm / Disarm | 解锁 / 上锁 |
| Flight Mode | 飞行模式 |
| Parameter | 参数 |
| Firmware | 固件 |
| Telemetry | 遥测 |
| Calibration | 校准 |
| Failsafe | 失效保护 |
| Geofence | 地理围栏 |
| Rally Point | 集结点 |
| Survey | 测绘 |
| Corridor Scan | 走廊扫描 |
| Structure Scan | 结构扫描 |
| Checklist | 检查单 |
| Offline Maps | 离线地图 |
| AutoConnect | 自动连接 |
| Joystick | 摇杆 |
| Gimbal | 云台 |
| Camera | 相机 |
| Video Stream | 视频流 |
| Trigger | 触发 |
| Airframe | 机架 |
| Sensor | 传感器 |
| Compass / Magnetometer | 罗盘 / 磁力计 |
| Accelerometer | 加速度计 |
| Gyro | 陀螺仪 |
| Barometer | 气压计 |
| Battery | 电池 |
| Motor | 电机 |
| Servo | 舵机 |
| ESC | 电调 |
| Landing Gear | 起落架 |
| Payload | 载荷 |
| Roll / Pitch / Yaw | 横滚 / 俯仰 / 偏航 |
| Throttle | 油门 |
| Altitude | 高度 |
| Attitude | 姿态 |
| Heading | 航向 |
| Gain | 增益 |
| Rate | 角速率 |
| Hover | 悬停 |
| Loiter | 悬停/盘旋（按上下文，优先"悬停"） |
| Log / Logging | 日志 / 日志记录 |
| Analyze | 分析 |
| Setup | 设置（车辆配置语境）/ 设置向导 |

### 3.2 保留原文（不翻译）

`PX4`、`ArduPilot`、`MAVLink`、`MAV_CMD_*`、`SYS_STATUS` 等消息/命令名、位掩码名、参数名（`RC1_MIN`）、单位（`m`、`m/s`、`°`、`V`、`A`、`mAh`）、协议缩写（`RTK`、`NTRIP`、`ADSB`、`PID`、`RC`、`GPS`、`IMU`、`EO`、`IR`、`UDP`、`TCP`、`USB`、`ESC`）、品牌（`Mapbox`、`Bing`、`Esri`、`Google`、`OpenStreetMap`）、文件路径与 URL。

### 3.3 风格约定

- 用词简短，贴合按钮/标签宽度；避免过长（下拉、按钮易截断）。
- 使用中文全角标点；句末按原句风格决定是否加"。"，纯标签不加。
- 保留末尾空格与省略号形态（如 `" (Partial)"` 前置空格不可丢）。
- 保持大小写风格一致：句首大写→中文无此问题，但缩写大小写不可改。

## 四、执行方式

### 4.1 工具链（不新增仓库文件）

- 在系统临时目录建立工作区（如 `%TEMP%\qgc-i18n\`），存放：
  - `apply_batch.py`：读取批次 JSON，按 `(context, source)` 精确定位并替换 `<translation>` 内容、去掉 `type="unfinished"`；对每条校验"命中且仅命中 1 次"，输出 applied / missing 统计；缺项即报错退出。
  - `batchNN.json`：每批的 `[{context, source, translation}]` 翻译记忆，既是输入也是交付物（可用于日后导入 Crowdin）。
- 仓库内**只修改** `translations/qgc_source_zh_CN.ts` 与 `translations/qgc_json_zh_CN.ts` 两个文件。

### 4.2 编辑硬规则

1. 只替换 `<translation>` 文本；去掉 `type="unfinished"`；不碰 `<source>` / `<location>` / `<extracomment>`。
2. 保留所有 `%n` 占位符、`&lt;b&gt;` 等实体与 URL。
3. 保持 LF 行尾与 2 空格缩进层级不变。
4. **不运行** `tools/translations/qgc_lupdate.py`、不运行 `update_translations` / `*_lupdate` 目标（会让 lupdate 重写 `.ts`）。普通 `just build` 只触发 lrelease，是安全的。
5. 开工前先把两个 `.ts` 备份到临时工作区，作为意外覆盖时的回滚源。

### 4.3 批次划分

**source 管道（`qgc_source_zh_CN.ts`，按 context 名过滤定位）**

| 批次 | 范围（context 名/所在模块） | 目标 |
| --- | --- | --- |
| 1 | `AutoPilotPlugins/APM`：context 名以 `APM` 开头（约 50 个 context，覆盖文件开头至 `APMTuningComponent` 一带） | 约 1100 条 |
| 2 | `AutoPilotPlugins` 其余：`Actuator*`、`Airframe*`、`AutoPilotPlugin*`、`Autotune*`、`Common/*`、`PX4*` | 约 450 条 |
| 3 | 任务/规划：`Survey*`、`Corridor*`、`Structure*`、`Transect*`、`CameraCalc*`、`Camera*`、`RallyPoint*`、`BreachReturn*`、`*LandingPattern*`、`Mission*`、`PlanView*`、`SimpleItem*`、`Takeoff*`、`GeoFence*`、`KML*` | 约 400 条 |
| 4 | 飞行视图与通用控件：`FlyView*`、`QmlControls*`、`ScreenTools`、`MainWindow*`、`Toolbar*`、`*Indicator*`、`*Panel*`、`*Dialog*` | 约 350 条 |
| 5 | 设置与向导：`AppSettings*`、`*Settings*`、`Setup*`、`*ComponentSummary*`、`*ComponentController*` 中的界面串 | 约 300 条 |
| 6 | 收尾扫尾：其余全部上下文（`Vehicle*`、`FirmwarePlugin*`、`MAVLink*`、`AnalyzeView*`、`Video*`、`Camera*`、`Joystick*`、`ADSB*`、`Logging*`、`GPS*`、`Utilities*` 等） | 剩余全部，直至归零 |

注：每批开工前先用 Grep 统计该批 `type="unfinished"` 实际条数并记录，表中数字为估算依据，以实测为准。

**json 管道（`qgc_json_zh_CN.ts`，按 context = 文件名过滤定位）**

| 批次 | 范围（context 名即 JSON 文件名） | 目标 |
| --- | --- | --- |
| 7 | `MavCmdInfo*.json`（`MavCmdInfoCommon.json`、`MavCmdInfoFixedWing.json`、`MavCmdInfoMultiRotor.json`、`APM-MavCmdInfoCommon.json`） | 约 700 条 |
| 8 | `*.VehicleConfig.json`（APM/PX4 组件配置页标题、说明、enumStrings） | 约 350 条 |
| 9 | `*.SettingsGroup.json`（全部设置项 label/shortDesc/longDesc/enumStrings） | 约 400 条 |
| 10 | 其余：`*.FactMetaData.json`、`*.Facts.json`、`*Fact.json`、`*FactGroup.json` | 约 125 条 |

### 4.4 进度管理

- 每批开始时先用 Grep 记录该批 unfinished 条数（基线），完成后再次统计，差值即本批完成数。
- 每批完成后输出验收清单：批次号、涉及 context 数、完成条数、剩余总 unfinished 条数、抽查术语对照（约 20-30 条关键界面词）。
- 每批完成后询问是否 `git commit` 留档（**不自动提交**）。若留档，提交信息按 Conventional Commits，例如 `chore(i18n): translate APM component strings to zh_CN`。

## 五、假设与决策

| 项 | 决策 | 理由/影响 |
| --- | --- | --- |
| 不改 `type="unfinished"` 是否也能生效 | **必须去掉** | 标记为已完成才能确保 lrelease 收录；无论 lrelease 默认是否收录 unfinished，去掉属性都是正确做法。最终以构建后实际显示中文为准（见验收 S4）。 |
| `.ts` 被覆盖 | 接受 | 已确认；通过临时目录备份 + 分批 git 留档降低损失。 |
| `zh_TW`、硬编码文本、默认语言 | **不在本次范围** | 按用户选择。硬编码项会残留英文，清单见第七节。 |
| 术语最终解释权 | 以第三节术语表为准 | 用户抽查若提出更优译法，逐批回改并同步更新术语表。 |
| 翻译记忆 JSON | 存临时目录，不入库 | 避免新增无引用文件；如需长期保存可另行确认。 |

## 六、验收步骤

**S1 结构校验（每批必做，秒级）**

```
pre-commit run check-xml --files translations/qgc_source_zh_CN.ts translations/qgc_json_zh_CN.ts
```

必须通过。失败说明 XML 转义或标签被破坏。

**S2 完整性统计**

用 Grep 统计两个文件的 `type="unfinished"` 数量，目标均为 **0**。
同时校验 `<message>` 总数不变（4046 / 1821），防止误删条目。

**S3 占位符与实体守恒**

抽查含 `%1`、`%2`、`&lt;b&gt;` 的条目，确认译文占位符数量与 source 一致。

**S4 构建与运行期验证（关键）**

1. 构建：`just build`（或 Qt Creator 构建）。构建日志中应出现 lrelease 处理 `qgc_source_zh_CN.ts` / `qgc_json_zh_CN.ts`；确认 `build/i18n/`（`QM_OUTPUT_DIRECTORY`，多配置生成器下具体路径以构建后实际位置为准）下两个 `.qm` 时间戳被刷新。
2. 启动 QGroundControl → Settings → General → Language → 选择 `中文 (Chinese)` → 重启应用。
3. 抽查界面（每处至少确认 3 项）：
   - 主窗口菜单/工具栏/Toolbar 文案
   - Fly View：仪表标签、左侧动作按钮、飞行模式名
   - Plan View：任务项名称、测绘（Survey）参数面板、相机计算器
   - Vehicle Setup：机架、传感器校准、参数页、失效保护
   - Application Settings：General/Offline Maps/Logging 等设置项 label 与说明
   - 参数列表中的参数短描述（验证 json 管道生效）

**S5 回归**

- 确认切回 `System`/English 后界面恢复英文（验证未把英文串写进 source）。
- 运行 `pre-commit run --files translations/qgc_source_zh_CN.ts translations/qgc_json_zh_CN.ts`，确认 0 失败。
- 可选：`just test` 中与 ComponentInformation 翻译相关的用例（`test/Vehicle/ComponentInformation/`）。

## 七、本次不在范围内（已知会残留英文的位置）

按用户选择不做源码改造，以下文本不走翻译系统，即使 `.ts` 全译完仍显示英文，作为后续可选任务记录：

- QML 硬编码：`CameraCalcCamera.qml`（Landscape/Portrait）、[SyslinkComponent.qml](file:///c:/Users/Admin/Desktop/QGC/src/AutoPilotPlugins/Common/SyslinkComponent.qml#L68)、`QmlTest.qml`
- C++ 硬编码：[QGCCorePlugin.cc](file:///c:/Users/Admin/Desktop/QGC/src/API/QGCCorePlugin.cc#L217)（AirSpd/Thr）、[QGCMapEngineManager.cc](file:///c:/Users/Admin/Desktop/QGC/src/QtLocationPlugin/QGCMapEngineManager.cc#L112)（Various）
- 地图 provider 名称：`src/QtLocationPlugin/Providers/*.h`（Mapbox/Google/Generic 各 provider 显示名）
- 繁体中文 `zh_TW`（完成度 0，且界面无对应语言项）
- 默认语言仍为"跟随系统"（`default: 0`），首次启动非中文需手动选择并重启

如需覆盖以上内容，需另行立项（涉及 `qsTr()`/`tr()` 补全与代码改动）。
