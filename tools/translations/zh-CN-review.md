# 简体中文译文审校约定

## 范围与进度

本次只翻译 QGC 自有界面中的可见文字：C++/QML 文案见
`translations/qgc_source_zh_CN.ts`，设置、任务命令等 JSON 文案见
`translations/qgc_json_zh_CN.ts`。设备经 MAVLink 发送的原始消息、设备提供的相机定义、
地图和网站内容、操作系统及第三方控件内容不计入本次完成率。

在仓库根目录执行 `python tools/translations/check_zh_cn.py` 查看每个上下文的待译数量。
`--strict` 用于最终验收，在任何有效词条未完成、空译或占位符不一致时失败。
`unchanged` 仅是人工复核线索：参数 ID、产品名、缩写和单位可能需要保留原样。

## 术语

| 英文 | 建议译法 | 注意 |
| --- | --- | --- |
| Vehicle | 飞行器 | 如果上下文明确指其他载具，按实际对象翻译。 |
| Flight mode | 飞行模式 | 模式专名以固件实际显示和已有译文为准。 |
| Arm / Disarm | 解锁 / 上锁 | 不得误译为起飞 / 降落。 |
| Return to launch (RTL) | 返航 | 保留需要显示的 `RTL` 缩写。 |
| Takeoff / Land | 起飞 / 降落 | 确认动作与状态、命令与结果的区别。 |
| Mission | 任务 | 与“计划”页面及命令区分。 |
| Plan | 规划 / 计划 | 页面标题优先用“规划”，视句意确定。 |
| Waypoint | 航点 | 不改动 MAVLink 命令标识符。 |
| Failsafe | 失效保护 | 根据条件和动作校对安全提示。 |
| Firmware | 固件 | 固件名称保持专名。 |
| Parameter | 参数 | 参数 ID 原样保留。 |

## 逐条审校

1. 根据 `<context>`、`<location>`、`<extracomment>` 阅读源码或 JSON 后再译；先检查
   `unfinished`，已完成译文仅修正错译、过时用词或不一致。
2. 保留 `%1`、`%L1`、`%n` 等占位符的数量、类型和含义；保留富文本标签、换行、
   菜单助记符、单位和快捷键含义。将英文危险提示中的否定、前提和数值与中文逐项核对。
3. 仅保留必要的专名、协议、参数 ID 和计量单位，例如 `QGC`、`QGroundControl`、
   `MAVLink`、`PX4`、`ArduPilot`、`GPS`、`RTL`、`mAh`、`MAV_CMD_NAV_WAYPOINT`。
   其他与源文相同的拉丁字母文本需逐条判断，不能直接按“英文未翻译”批量替换。
4. 译文审校通过后移除对应的 `type="unfinished"`；不得靠删除标记或复制英文凑完成率。
   每处理约 100–200 条更新覆盖统计，在 Windows 选择中文并重启，对照实际页面验收。
5. 如发现没有进入 `.ts` 的 QGC 自有可见文字，在原 QML/C++ 中使用 `qsTr()`/`tr()`，
   或为 JSON 声明合适的 `translateKeys`；源模板和译文分别审查，避免覆盖 Crowdin 更新。
