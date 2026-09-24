# QGroundControl 简体中文界面汉化计划

## 概要

以当前 `learn/qgc-v5.1.4` 分支为基线，分批完成 QGC 自有界面的简体中文汉化，并在 Windows 上逐批实测。保留现有语言选择机制：用户在应用设置中选择“中文”，重启后完整生效；不改变其他语言和系统默认选项。已完成的中文译文保留并审校，未完成的译文逐条结合上下文翻译，不以未经校对的机翻作为完成标准。

## 当前状态分析

- `src/Settings/AppSettings.cc` 已将中文列为正式语言；`src/AppSettings/pages/General.SettingsUI.json` 已提供 `appSettings.qLocaleLanguage` 选择框；`src/Settings/App.SettingsGroup.json` 标记语言设置重启生效。无需新建语言切换功能。
- `src/QGCApplication.cc` 已在启动时安装 Qt、C++/QML、JSON 三类翻译，并对运行中的 QML 发出重新翻译请求。JSON 元数据需重启后重载。
- `translations/qgc_source_zh_CN.ts` 约 4,047 个消息，其中约 2,877 个标记为 `unfinished`；`translations/qgc_json_zh_CN.ts` 约 1,821 个消息，其中约 1,575 个标记为 `unfinished`。当前基线合计约 5,868 项、约 4,452 项未完成。以上为初步文本计数，正式基线应按 XML 有效消息重新统计。
- `tools/translations/qgc_lupdate.py` 从 `src/` 提取 C++/QML 与 JSON 文本到 `translations/qgc.ts`、`translations/qgc-json.ts`；`crowdin.yml` 将它们映射到上述中文翻译文件；`CMakeLists.txt` 用 `qt_add_translations` 打包 `.qm`。
- 工作区现有用户文件 `tools/translations/README.zh-CN.md`，不是本任务的翻译输入，保持不动。此计划阶段不修改任何项目源码或翻译文件。

## 拟议改动

### 1. 建立覆盖基线与术语规范

- 为 `tools/translations/` 增加只读的简体中文 TS 审核脚本（如 `check_zh_cn.py`），为 `tools/tests/` 增加对应测试。用 XML 解析 `translations/qgc_source_zh_CN.ts` 与 `translations/qgc_json_zh_CN.ts`，按有效消息输出各文件及上下文的总量、未完成、空译文、疑似未翻译等计数；检查 `%1`、`%L1`、`%n` 等 Qt 占位符和复数形式，并明确允许保留的标识符/品牌/缩写。CI 验证只检查，不自动改写译文。
- 在 `tools/translations/` 增加简体中文术语和人工复核说明，记录飞行安全、任务规划、航点、返航、解锁、固件和参数等术语的统一译法，以及不可翻译的参数名、MAVLink 标识符、单位、文件路径和品牌名。译文不得改变安全提示的条件、否定、数值、占位符及富文本结构。
- 先检查源模板与中文 TS 的对应关系；若需运行 `python3 tools/translations/qgc_lupdate.py` 刷新模板，应单独审查其生成差异，避免把无关的批量位置或伪本地化变化混入翻译提交。

### 2. 分阶段补齐并审校译文

- 在 `translations/qgc_source_zh_CN.ts` 中按上下文分批处理：应用导航/通用控件与设置；飞行视图与安全警报；规划/任务；飞控设置、固件与车辆页面；相机、分析与其他 QGC 自有页面。每批建议约 100–200 条，逐条核对源文、上下文和运行效果；已完成译文只修正错译、不一致或过时内容。
- 在 `translations/qgc_json_zh_CN.ts` 中分批处理设置项、Fact 元数据、任务命令说明、参数展示文案等；确认 JSON 原始标识符保持不变，只翻译 `translateKeys` 所定义的用户可见内容。必要时调整 `src/` 下具体 JSON 的可翻译键声明，使漏掉的用户可见字段进入既有提取链，但不翻译内部键值。
- 每批更新计数报告并复核术语、占位符、快捷键、换行、HTML/富文本。达到一个模块的 0 个应译 `unfinished` 后再进行该模块 Windows 界面走查，然后处理下一模块；最终两份 TS 的应译有效条目均达到 100%。

### 3. 排查翻译系统未覆盖的自有文案

- 对 `src/` 的 QML/C++ 用户可见字符串及 JSON 元数据做静态排查，再用 Windows 运行版分别走查首页、应用设置、飞行、规划、分析、PX4/ArduPilot 车辆设置与错误/空状态；记录出现英文的路径、截图、来源。
- 对确认是 QGC 自有且缺少翻译标记的文字，按现有规范在原文件改用 `qsTr()`、`tr()`，或在 JSON 中声明 `translateKeys`；重提取模板后补入中文 TS。仅修复实际用户可见遗漏，不对协议标识符或内部调试文本做机械替换。
- 飞控通过 MAVLink 实时发送的文本、设备提供的相机定义、地图/网站内容、系统与第三方窗口不在此次译文 100% 指标内；记录来源并保持原样，不通过按关键字强行替换设备消息。

## 假设与决策

- 目标用户为简体中文的 Windows QGC 使用者；先在 Windows 实测，保留翻译文件跨平台可用性，不承诺本轮对其他平台逐一实测。
- “全部汉化”指所有可由本仓库控制、面向用户显示的 QGC 文案；产品名、协议名、参数 ID、单位等必要专名允许保留原文，并在术语说明中列出。
- 保留现有英文源文本、原有翻译、其他语言与用户语言设置；不强制默认简中。若用户在应用中切换语言，按现有提示重启后验收 JSON 文字。
- 以分批交付和逐条校对为约束：大约 4,452 项未完成译文不是一个小补丁；进度报告需区分“已处理批次”和“全部完成”，不得因完成首批就宣称整体汉化完成。
- Crowdin 是仓库既有同步链路；本地修改中文 TS 后若未来由 Crowdin 同步覆盖，应将最终译文同步到对应 Crowdin 项目，或明确维护本地分支的冲突处理流程。本轮不自动上传、推送或创建 PR。

## 验证步骤与验收标准

1. 每批运行 TS 审核脚本和其单元测试；检查 XML 可解析、无空的应译文本、占位符和复数形式匹配、标记状态与计数正确；有意保留的专名有记录。
2. 使用现有 `CMakeLists.txt` 翻译构建路径增量构建（`just build`，若 Windows 未安装 `just` 则用仓库 `justfile` 对应的 `cmake --build build --config Debug --parallel ...`），确认 `.qm` 生成及资源加载；执行 `just lint` 或对应 pre-commit 检查，以及相关工具测试、`ctest` 对应标签测试。不要在验证时悄悄修改源模板。
3. 在 Windows 应用设置中手动选“中文”，重启并核对菜单、设置、飞行、规划、分析、PX4/ArduPilot 设置等页面；测试安全警报、空状态、错误对话框、动态数值、参数说明和任务命令说明，确认无 QGC 自有英文残留、中文不截断、按钮和快捷键可用。
4. 最终重新生成覆盖报告：两份中文 TS 中所有应译有效消息均有经审校译文且不再标记 `unfinished`；静态排查和人工走查发现的 QGC 自有英文文案已修复或有具体排除原因。记录 Windows 构建/测试结果及外部文本的边界；其他平台仅做不破坏构建/资源的静态检查，不作为实测结论。
