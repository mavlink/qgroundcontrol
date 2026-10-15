---
name: qgc-dev
description: 面向 QGroundControl 仓库的 C++20/Qt6/QML 开发指引，含架构铁律、build-test-lint 流程与改动清单。当在本仓库新增或修改 C++、QML、测试、CMake，或排查构建/测试失败时使用。不用于与本仓库无关的通用 Qt 问题。
---

# QGroundControl 开发指引

在本仓库做任何代码改动时按本文件执行。命令与规范以仓库内文件为最终事实来源，不要凭记忆猜测；遇到本文件未覆盖的问题，先查下方"权威文档"。

## 权威文档（先读，胜过猜测）

| 问题 | 仓库文件 |
| --- | --- |
| 架构铁律、构建命令、完成定义、提交规范 | [AGENTS.md](../../../AGENTS.md) |
| 命名/格式/C++20/QML/日志规范与示例 | [CODING_STYLE.md](../../../CODING_STYLE.md) |
| Fact System / 多机 / FirmwarePlugin 模式 | [.github/CONTRIBUTING.md](../../../.github/CONTRIBUTING.md#architecture-patterns) |
| 测试基类、标签、超时、日志断言 | [test/README.md](../../../test/README.md) |
| just/python 工具链全部命令 | [tools/README.md](../../../tools/README.md) |
| 模块地图与关键数据流 | [references/architecture.md](references/architecture.md) |
| 各平台（含 Windows）构建/测试命令 | [references/build-workflow.md](references/build-workflow.md) |
| 可复制的代码骨架 | [references/coding-patterns.md](references/coding-patterns.md) |

## 架构铁律（违反即被 pre-commit/CI 拒绝）

1. **Fact System**：载具参数一律走 `Fact`（`vehicle->parameterManager()->getParameter(...)`），禁止自建参数存储或缓存副本。访问前等 `parametersReady`，区分 `rawValue()`/`cookedValue()`。
2. **多机空判**：每次使用 `activeVehicle()`（QML 中 `QGroundControl.multiVehicleManager.activeVehicle`）后先判空再解引用。
3. **Firmware Plugin**：固件差异用 `vehicle->firmwarePlugin()`（实现放 `src/FirmwarePlugin/APM`、`src/FirmwarePlugin/PX4`），禁止 `if (px4)` / `if (apm)` 分支。
4. **QML 集成**：C++ 类型用 `QML_ELEMENT`/`QML_SINGLETON`/`QML_UNCREATABLE` 注册，状态通过 `Q_PROPERTY` 暴露；禁止把 C++ 对象塞进 context property。
5. **生产代码禁止 `Q_ASSERT`**：用 `qCWarning` + 提前 return 的防御式写法（`check-no-qassert`）。
6. **测试禁止 `QTest::ignoreMessage`**：用 `expectLogMessage()` + `verifyExpectedLogMessage()`（`check-no-qtest-ignore-message`）。
7. **测试禁止固定延时 `QTest::qWait(n)`**：用 `TestTimeout::shortMs()/mediumMs()/longMs()` 配合 `QVERIFY_*_WAIT`、`QSignalSpy::wait`、`waitForCondition`（`check-no-fixed-qwait`）。

## 标准开发循环

- **增量构建**：每改几个文件就构建一次，立刻修错，不要攒到最后。
- **紧测试循环**：只跑受影响测试 `ctest -R <名字>` 或 `QGroundControl --unittest:<TestClass>`；收尾再跑完整 `Unit` 标签。
- **与 CI 对齐**：本地用与 CI 相同的命令（见 [references/build-workflow.md](references/build-workflow.md)），不要自造命令行。
- Windows 环境用 `python`（非 `python3`）、PowerShell 语法；推荐 CMake preset（`cmake --preset Windows`），需先设 `QT_ROOT_DIR`。

```powershell
# Windows 推荐路径（Ninja Multi-Config，输出到 build/Windows）
cmake --preset Windows
cmake --build --preset Windows-debug
ctest --test-dir build/Windows -C Debug -L Unit --output-on-failure

# 跨平台 just 封装（Windows 上先设 QT_DIR 指向 Qt 安装目录）
just configure   # 首次：拉 submodule + CMake 配置（含 --testing）
just build       # 增量构建
just test        # ctest -L "Unit|Integration" -LE "Flaky|Network"
just lint        # pre-commit run --all-files
just check       # lint + test，提交前必过
```

## 改动手册（按任务类型）

### 1. 在已有模块新增/修改 C++ 类
- 文件放对应 `src/<Module>/`，命名 `ClassName.h` / `ClassName.cc`（PascalCase）。
- 把新文件加进该模块 [CMakeLists.txt](../../../src/Gimbal/CMakeLists.txt) 的 `target_sources(... PRIVATE ...)`，否则不会编译。
- 头文件 `#pragma once`；include 顺序：系统 → Qt（全路径 `#include <QtCore/...>`）→ 项目。
- 日志用分类 category（`QGC_LOGGING_CATEGORY`），禁止裸 `qDebug()`。

### 2. 新建 src 模块
1. 建 `src/<NewModule>/` 目录和 `CMakeLists.txt`（仿照 [src/Gimbal/CMakeLists.txt](../../../src/Gimbal/CMakeLists.txt)：`target_sources(${CMAKE_PROJECT_NAME} ...)` 或 `qt_add_library`）。
2. 在 [src/CMakeLists.txt](../../../src/CMakeLists.txt) 的 subdirectory 区 `add_subdirectory(<NewModule>)`。
3. 有 QML 则在模块内 `qt_add_qml_module`，URI 遵循 `QGroundControl.<NewModule>`。

### 3. 新增 QML 页面/组件
- 放入对应 UI 模块（FlyView/PlanView/QmlControls 等），并把文件名加入该模块 `qt_add_qml_module(... QML_FILES ...)`（仿 [src/FlyView/CMakeLists.txt](../../../src/FlyView/CMakeLists.txt)）。
- 尺寸用 `ScreenTools.defaultFontPixelHeight/Width`，颜色用 `QGCPalette`，优先复用 `QGCButton`/`QGCLabel` 等 QGC 控件；用户可见字符串必须 `qsTr()`。
- `Connections` 用 Qt6 函数语法 `function onXxx() {}`，禁止 `onXxx: {}` 旧语法。

### 4. 把 C++ 暴露给 QML
- 类声明加 `Q_OBJECT` + `QML_ELEMENT`（QML 可创建）或 `QML_UNCREATABLE("...")`（仅 C++ 实例化）/ `QML_SINGLETON`。
- 属性用 `Q_PROPERTY(... READ ... WRITE ... NOTIFY ...)`；setter 中值变化才 `emit`。
- 前向声明类型出现在 `Q_PROPERTY` 时加 `Q_MOC_INCLUDE("...")`。

### 5. 读写载具参数
```cpp
Vehicle* vehicle = _vehicle; // 或 activeVehicle()，用前必须判空
Fact* fact = vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "MAV_1_RATE");
if (fact) {
    fact->setCookedValue(newValue); // 显示/工程单位用 cooked；协议裸值用 raw
}
```
QML：`vehicle.mav1RateFact.rawValue`（Fact 已暴露为属性），不要自建参数副本。

### 6. 增加固件相关行为
在 `FirmwarePlugin` 接口上加虚函数，分别在 `APMFirmwarePlugin` / `PX4FirmwarePlugin` 实现，调用方写 `vehicle->firmwarePlugin()->xxx()`。载具设置页差异放 `src/AutoPilotPlugins/APM`、`src/AutoPilotPlugins/PX4`。

### 7. 新增持久化设置
继承 `SettingsGroup`（`src/Settings/`），配套 `*.SettingsGroup.json`，用 `SettingsFact`；仿 `NTRIPSettings` / `VideoSettings`。不要新造一套持久化。

### 8. 新增单元测试
- 在 `test/` 下镜像 `src/` 结构建 `<X>Test.h/.cc`，选最窄基类（`UnitTest` / `VehicleTest` / `ParameterTest` / `MissionTest` 等）。
- `UT_REGISTER_TEST(MyTest, TestLabel::Unit)` 自注册，无需改中央列表。
- 在该测试目录 `CMakeLists.txt` 写 `add_qgc_test(MyTest LABELS Unit)`（需要 MockLink 连车加 `Vehicle`，长跑加 `Slow TIMEOUT n`，独占资源加 `SERIAL`）。
- 异步等待用 `TestTimeout::*` + QGC 包装宏；严格日志模式下预期告警用 `expectLogMessage`。
- 完整骨架见 [references/coding-patterns.md](references/coding-patterns.md)。

## 风格速记

- 4 空格、LF、UTF-8、列宽 120；类 PascalCase、函数/变量 camelCase、成员 `_leadingUnderscore`、常量 UPPER_SNAKE、枚举 `enum class`。
- C++20：`[[nodiscard]]`、`std::span`、`std::string_view`、ranges、designated initializers、`constexpr`。
- 日志：`qCDebug`（默认不显示，首选）/ `qCWarning`（异常但已处理）/ `qCCritical`（代码错误，会导致单测失败）；消息不带函数名前缀。
- QML 文件内顺序：id/绑定 → 公开属性 → `_` 私有属性 → signal → function → 可视子项 → Connections → `Component.onCompleted`。
- 高频坑：未判空 vehicle、参数未 ready 就访问、绕过 FirmwarePlugin、混用 cooked/raw、QML 硬编码尺寸颜色。

## 完成定义

1. 增量构建 + `just build` 通过。
2. `just lint`（或 `pre-commit run --all-files`）通过；格式问题先 `just format-fix`。
3. 受影响测试 `ctest -R <name>` 通过，收尾跑 `ctest --output-on-failure -L Unit`。
4. 改动聚焦：不夹带无关修改、不留注释掉的代码和含糊 TODO。
5. 提交信息用 Conventional Commits：`feat(Vehicle): ...`、`fix(Comms): ...`、`test(...)`、`refactor(...)`、`chore(...)` 等；只有 `feat/fix/perf/revert` 触发发版。未经用户明确要求不要提交 commit。
