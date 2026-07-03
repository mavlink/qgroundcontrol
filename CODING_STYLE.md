# QGroundControl Coding Style Guide

This document describes the coding conventions for QGroundControl. For the canonical AI-agent
workflow entry point (golden rules, build/test commands, project structure), see [AGENTS.md](AGENTS.md).

For complete worked examples, see the reference files:

- [CodingStyle.h](tools/coding-style/CodingStyle.h) - C++ header example
- [CodingStyle.cc](tools/coding-style/CodingStyle.cc) - C++ implementation example
- [CodingStyle.qml](tools/coding-style/CodingStyle.qml) - QML example

## Contents

- [General](#general)
- [Comments](#comments)
- [Naming Conventions](#naming-conventions)
- [C++ Style](#c-style)
  - [Headers](#headers)
  - [Class Declaration Order](#class-declaration-order)
  - [Modern C++ (C++20)](#modern-c-c20)
  - [Defensive Coding](#defensive-coding)
  - [Logging](#logging)
- [Qt6 / QML Integration](#qt6--qml-integration)
  - [Exposing C++ to QML](#exposing-c-to-qml)
  - [Signal Emission](#signal-emission)
  - [QML File Structure](#qml-file-structure)
  - [QML Guidelines](#qml-guidelines)
  - [Connections Syntax (Qt6)](#connections-syntax-qt6)
- [Common Pitfalls](#common-pitfalls)
  - [Examples](#examples)
- [Formatting Tools](#formatting-tools)
- [Additional Resources](#additional-resources)

## General

- **Indentation**: 4 spaces (no tabs)
- **Line endings**: LF (Unix-style)
- **File encoding**: UTF-8
- **Max line length**: 120 columns (enforced by `.clang-format`, `ColumnLimit: 120`)

## Comments

Write comments that earn their place. Prefer self-documenting code — clear names and small functions —
over narration.

- **Only when non-obvious.** Comment the *why* (intent, trade-offs, non-obvious constraints, links to
  a spec or bug), not the *what* the code already states. Delete comments that just restate the code.
- **Keep them concise.** A short phrase beats a paragraph. Update or remove comments when the code
  changes so they never go stale.
- **Doxygen `///`** for public class/API docs in headers (see [Headers](#headers)); skip redundant
  member-by-member narration.

## Naming Conventions

| Element | Convention | Example |
| --------- | ------------ | --------- |
| Classes | PascalCase | `VehicleManager` |
| Methods/Functions | camelCase | `getActiveVehicle()` |
| Variables | camelCase | `activeVehicle` |
| Private members | _leadingUnderscore | `_vehicleList` |
| Constants | UPPER_SNAKE_CASE | `MAX_RETRY_COUNT` |
| Enums | PascalCase (scoped) | `enum class FlightMode` |
| Files | ClassName.h/.cc | `Vehicle.h`, `Vehicle.cc` |

## C++ Style

### Headers

```cpp
#pragma once

// System headers first
#include <algorithm>
#include <span>

// Qt headers second (use full paths)
#include <QtCore/QObject>
#include <QtCore/QString>

// Project headers last
#include "Vehicle.h"
```

### Class Declaration Order

```cpp
class MyClass : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(...)

public:
    // Constructors/destructor
    // Enums
    // Public methods
    // Getters/setters

signals:
    // Signals

public slots:
    // Public slots (only if connected externally)

protected:
    // Protected members (only for base classes)

private slots:
    // Private slots

private:
    // Private methods (prefixed with _)
    // Private members (prefixed with _)
};
```

### Modern C++ (C++20)

QGroundControl uses C++20. Prefer modern features:

```cpp
// Use [[nodiscard]] for functions with important return values
[[nodiscard]] bool isValid() const;

// Use std::string_view for read-only string parameters (non-Qt code)
bool validate(std::string_view input);

// Use std::span instead of pointer + size
void processData(std::span<const int> data);

// Use ranges for cleaner algorithms
auto filtered = data | std::views::filter([](int n) { return n > 0; });

// Use designated initializers for structs
Config config{.timeout = 30, .retries = 3};

// Use constexpr for compile-time constants
static constexpr int MaxRetries = 5;
```

### Defensive Coding

```cpp
// Always null-check pointers
Vehicle* vehicle = _manager->activeVehicle();
if (!vehicle) {
    qCWarning(MyLog) << "No active vehicle";
    return;
}

// Validate inputs early
if (param.isEmpty()) {
    return;
}

// Avoid Q_ASSERT in production - use defensive checks instead
// Q_ASSERT is removed in release builds
```

### Logging

- `qCDebug` - general logging, only displayed when the category is turned on. Use this instead of `qCInfo`.
- `qCWarning` - handled-but-unusual error flows (e.g. vehicle failed to respond to a request). Always displayed.
- `qCCritical` - indicates a coding error (e.g. a `Fact` using an unsupported type). Always displayed, and fails
  unit tests when hit.
- Never use uncategorized logging (`qDebug()`).
- Don't prefix messages with the function/method name - the logging formatter already includes it.
- When logging multiple values, stream each on its own line, aligned under the first operand.

```cpp
// Declare in header
Q_DECLARE_LOGGING_CATEGORY(MyComponentLog)

// Define in source (use QGC macro for runtime configuration)
QGC_LOGGING_CATEGORY(MyComponentLog, "qgc.component.name")

qCDebug(MyComponentLog) << "download(): fromURI:" << fromURI; // Wrong - redundant function name prefix
qCDebug(MyComponentLog) << "fromURI:" << fromURI;             // Correct

qCDebug(MyComponentLog) << "fromCompId:" << fromCompId
                        << "fromURI:" << fromURI
                        << "toDir:" << toDir
                        << "fileName:" << fileName;

qCWarning(MyComponentLog) << "Warning message";
qCCritical(MyComponentLog) << "Internal Error: ...";

qDebug() << "..."; // Wrong - never use uncategorized logging
```

## Qt6 / QML Integration

### Exposing C++ to QML

```cpp
class MyClass : public QObject
{
    Q_OBJECT
    QML_ELEMENT                    // For QML-creatable types
    QML_UNCREATABLE("C++ only")    // For C++-only instantiation
    QML_SINGLETON                  // For singletons

    Q_MOC_INCLUDE("Vehicle.h")     // For forward-declared types in Q_PROPERTY

    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(Vehicle* vehicle READ vehicle CONSTANT)
};
```

### Signal Emission

```cpp
void MyClass::setValue(int newValue)
{
    if (_value != newValue) {
        _value = newValue;
        emit valueChanged(_value);  // Always emit when property changes
    }
}
```

### QML File Structure

```qml
import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    // 1. Property bindings (width, height, anchors)
    width: ScreenTools.defaultFontPixelHeight * 10

    // 2. Public properties
    property int myProperty: 0

    // 3. Private properties (underscore prefix)
    readonly property bool _isValid: myProperty > 0

    // 4. Signals
    signal clicked()

    // 5. Functions
    function doSomething() { }

    // 6. Visual children
    QGCButton {
        text: qsTr("Click Me")
        onClicked: root.clicked()
    }

    // 7. Connections (use function syntax)
    Connections {
        target: someObject
        function onSignalName() { }  // NOT: onSignalName: { }
    }

    // 8. Component.onCompleted
    Component.onCompleted: { }
}
```

### QML Guidelines

- **No hardcoded sizes**: Use `ScreenTools.defaultFontPixelHeight/Width`
- **No hardcoded colors**: Use `QGCPalette` for theming
- **Use QGC controls**: `QGCButton`, `QGCLabel`, `QGCTextField`, etc.
- **Translations**: Wrap user-visible strings with `qsTr()`
- **Null checks**: Always check `_activeVehicle` before use

### Connections Syntax (Qt6)

```qml
// CORRECT - Qt6 function syntax
Connections {
    target: vehicle
    function onArmedChanged() {
        console.log("Armed state changed")
    }
}

// DEPRECATED - Old Qt5 syntax (triggers pre-commit warning)
Connections {
    target: vehicle
    onArmedChanged: { }  // Don't use this
}
```

## Common Pitfalls

1. **Assuming single vehicle** - Always null-check `activeVehicle()`
2. **Accessing Facts before ready** - Wait for `parametersReady` signal
3. **Bypassing FirmwarePlugin** - Use plugin for firmware-specific behavior
4. **Using Q_ASSERT in production** - Use defensive checks instead
5. **Mixing cookedValue/rawValue** - Understand the difference
6. **Hardcoded QML sizes/colors** - Use ScreenTools and QGCPalette

### Examples

```cpp
// Always null-check vehicle
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) return;

// Access parameters via Fact System
Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (param) param->setCookedValue(newValue);
```

```qml
// QML vehicle access
property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
enabled: vehicle && vehicle.armed
```

## Formatting Tools

Formatting and static analysis are enforced via [.pre-commit-config.yaml](.pre-commit-config.yaml):

- `.clang-format` - C++ formatting
- `.clang-tidy` - C++ static analysis
- `.qmlformat.ini` - QML formatting
- `.qmllint.ini` - QML linting
- `.editorconfig` - Editor settings

Run them with `just lint` (fast gate) or `pre-commit run --all-files` (full sweep); see
[tools/README.md](tools/README.md) for all commands.

## Additional Resources

- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [QGroundControl Dev Guide](https://dev.qgroundcontrol.com/)
- [MAVLink Protocol](https://mavlink.io/)
