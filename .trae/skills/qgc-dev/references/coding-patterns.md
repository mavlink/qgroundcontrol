# 代码骨架与常用模式

直接仿照这些形状写代码；更完整的规范见 [CODING_STYLE.md](../../../../CODING_STYLE.md) 与
`tools/coding-style/` 下的 CodingStyle.h/.cc/.qml 示例。

## C++ 类骨架

```cpp
// MyComponent.h
#pragma once

#include <QtCore/QObject>

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(MyComponentLog)

class MyComponent : public QObject
{
    Q_OBJECT
    QML_UNCREATABLE("Created by Vehicle")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(Vehicle* vehicle READ vehicle CONSTANT)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    explicit MyComponent(Vehicle* vehicle, QObject* parent = nullptr);

    [[nodiscard]] Vehicle* vehicle() const { return _vehicle; }
    [[nodiscard]] bool active() const { return _active; }
    void setActive(bool active);

signals:
    void activeChanged(bool active);

private:
    void _handleTelemetry();

    Vehicle* _vehicle = nullptr;
    bool _active = false;
};
```

```cpp
// MyComponent.cc
#include "MyComponent.h"

#include "Vehicle.h"

QGC_LOGGING_CATEGORY(MyComponentLog, "qgc.mycomponent")

MyComponent::MyComponent(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
    // 防御式：vehicle 可能为空（多机切换/断链）
    if (!_vehicle) {
        qCWarning(MyComponentLog) << "Created without vehicle";
        return;
    }
}

void MyComponent::setActive(bool active)
{
    if (_active != active) {
        _active = active;
        emit activeChanged(_active);
    }
}
```

规则要点：4 空格、列宽 120、include 顺序（系统 → Qt 全路径 → 项目）、成员 `_` 前缀、
禁 `Q_ASSERT`、禁裸 `qDebug()`；`qCDebug` 消息不带函数名前缀，多值换行对齐。

## 载具空判与参数（Fact）

```cpp
Vehicle* vehicle = activeVehicle(); // 或 MultiVehicleManager::instance()->activeVehicle()
if (!vehicle) {
    qCWarning(MyComponentLog) << "No active vehicle";
    return;
}
if (!vehicle->parameterManager()->parametersReady()) {
    // 参数未同步完，等待 parametersReady 信号，不要立即读
    return;
}
Fact* fact = vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "PARAM_NAME");
if (fact) {
    fact->setCookedValue(newValue); // cooked=工程/显示单位，raw=协议裸值
}
```

```qml
// QML 中同样必须判空
property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
enabled: vehicle && vehicle.armed
text: vehicle ? vehicle.mav1RateFact.rawValue : ""
```

## 固件差异：走 FirmwarePlugin，不写 if (px4)

```cpp
// 正确：面向插件接口
vehicle->firmwarePlugin()->guidedFlightMode();

// 错误：业务代码里判断固件
if (vehicle->firmwareType() == MAV_AUTOPILOT_PX4) { /* ... */ }
```

新增固件行为的步骤：

1. 在 `src/FirmwarePlugin/FirmwarePlugin.h` 加虚函数（给安全默认实现）。
2. `src/FirmwarePlugin/PX4/` 与 `src/FirmwarePlugin/APM/` 分别 override。
3. 调用方一律 `vehicle->firmwarePlugin()->...()`。
4. 若涉及设置页 UI，在 `src/AutoPilotPlugins/PX4|APM` 的 `VehicleComponent` 体系中扩展。

## QML 组件骨架（Qt6）

```qml
import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

QGCButton {
    id: root

    // 1. 公开属性 2. _ 私有属性 3. signal 4. function 5. 子项 6. Connections 7. onCompleted
    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _canAct: vehicle && vehicle.armed

    text: qsTr("Takeoff")
    enabled: _canAct

    onClicked: console.log("clicked") // 用户可见文字一律 qsTr()

    Connections {
        target: root.vehicle
        function onArmedChanged() { }   // Qt6 函数语法；禁止 onArmedChanged: {}
    }
}
```

- 尺寸：`ScreenTools.defaultFontPixelHeight/Width`；颜色：`QGCPalette`；优先 QGC 控件。
- 文件必须登记到模块的 `qt_add_qml_module(... QML_FILES ...)`。

## CMake 登记模式

把 C++ 文件加进**已有模块**（仿 `src/Gimbal/CMakeLists.txt`）：

```cmake
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MyComponent.cc
        MyComponent.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
```

视图/控件模块使用独立 static lib + QML 模块（仿 `src/FlyView/CMakeLists.txt`）：

```cmake
qt_add_library(MyViewModule STATIC)

qt_add_qml_module(MyViewModule
    URI QGroundControl.MyView
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        MyPage.qml
    NO_PLUGIN
)

qt_add_resources(MyViewModule myview_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES "${CMAKE_SOURCE_DIR}/resources/some_icon.svg"
)
```

新建模块还要在 `src/CMakeLists.txt` 加 `add_subdirectory(MyView)`。
JSON 元数据（如 FactGroup 定义）用 `qt_add_resources(... PREFIX "/json/Vehicle" FILES XxxFact.json)` 方式挂载。

## 单元测试骨架

```cpp
// test/MyArea/MyComponentTest.h
#pragma once

#include "UnitTest.h"

class MyComponentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _defaultState();
    void _handlesMissingVehicle();
};
```

```cpp
// test/MyArea/MyComponentTest.cc
#include "MyComponentTest.h"

#include "MyComponent.h"

UT_REGISTER_TEST(MyComponentTest, TestLabel::Unit)

void MyComponentTest::init()
{
    UnitTest::init();
}

void MyComponentTest::_handlesMissingVehicle()
{
    MyComponent component(nullptr);
    QVERIFY(!component.active());
}
```

```cmake
# test/MyArea/CMakeLists.txt
add_qgc_test(MyComponentTest LABELS Unit)
# 需要 MockLink 连车：add_qgc_test(FooTest LABELS Unit Vehicle)
# 慢测试：add_qgc_test(FooTest LABELS Slow TIMEOUT 300)
# 独占资源：add_qgc_test(FooTest LABELS Integration SERIAL)
```

- 基类按需求选最窄的：`UnitTest` → `VehicleTest`（有连车）→ `ParameterTest`/`MissionTest`；
  APM 风味用 `VehicleTestAPM`。`UT_REGISTER_TEST` 自注册，无中央列表。
- 测试方法名 `_` 前缀；一测一行为；排列组合用 `_data()` + `QFETCH` 数据驱动，不复制粘贴。

### 异步等待（禁止 QTest::qWait 固定延时）

```cpp
QVERIFY_SIGNAL_WAIT(spy, TestTimeout::shortMs());
QVERIFY_SIGNAL_COUNT_WAIT(spy, 3, TestTimeout::shortMs());
QVERIFY_TRUE_WAIT(controller->ready(), TestTimeout::mediumMs());
QVERIFY(UnitTest::waitForCondition([&]() { return obj.done(); },
                                   TestTimeout::shortMs(),
                                   QStringLiteral("obj.done()")));
```

### 严格日志模式下的断言/抑制

```cpp
// 首选：断言日志确实发出
expectLogMessage("qgc.mavlink.libevents",
                 QtWarningMsg,
                 QRegularExpression(QStringLiteral("Flight mode group not set")));
triggerCode();
verifyExpectedLogMessage(); // 漏掉这行 cleanup() 会失败

// 兜底：环境相关噪声的持久抑制（不断言发出）
ignoreLogMessage("qt.bluetooth", QtWarningMsg,
                 QRegularExpression(QStringLiteral("Cannot find a compatible running Bluez")));
```

### MultiSignalSpy

```cpp
MultiSignalSpy spy;
spy.init(&obj);
QVERIFY(spy.expect("valueChanged").once());
QVERIFY(spy.waitForSignal("readyChanged", TestTimeout::mediumMs()));
QVERIFY(spy.expect("errorOccurred").never());
int v = spy.argument<int>("valueChanged");
```

## 提交信息（Conventional Commits）

```text
feat(FactSystem): add bulk parameter refresh job
fix(Vehicle): guard null activeVehicle in telemetry handler
test(GPS): cover NTRIP source table parsing
refactor(Comms): extract UDP IO device helper
chore(ci): bump Windows Qt to 6.11.1
```

触发发版：`feat` / `fix` / `perf` / `revert`；不触发：`docs` / `style` / `chore` / `refactor` / `test` / `build` / `ci`。
范围（括号内）用模块名。未经用户明确要求，不要代为执行 git commit。
