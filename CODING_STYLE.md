# QGroundControl Coding Style Guide

This document describes the coding conventions for QGroundControl. For complete examples, see the reference files:

- [CodingStyle.h](tools/coding-style/CodingStyle.h) - C++ header example
- [CodingStyle.cc](tools/coding-style/CodingStyle.cc) - C++ implementation example
- [CodingStyle.qml](tools/coding-style/CodingStyle.qml) - QML example

## General

- **Indentation**: 4 spaces (no tabs)
- **Line endings**: LF (Unix-style)
- **File encoding**: UTF-8
- **Max line length**: No hard limit, use judgment

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
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

```cpp
// Declare in header
Q_DECLARE_LOGGING_CATEGORY(MyComponentLog)

// Define in source (use QGC macro for runtime configuration)
QGC_LOGGING_CATEGORY(MyComponentLog, "Component.Name")

// Use categorized logging
qCDebug(MyComponentLog) << "Debug message";
qCWarning(MyComponentLog) << "Warning message";
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

## QML Style

### File Structure

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

## Formatting Tools

The repository includes configuration for automatic formatting:

- `.clang-format` - C++ formatting
- `.clang-tidy` - C++ static analysis
- `.qmlformat.ini` - QML formatting
- `.qmllint.ini` - QML linting
- `.editorconfig` - Editor settings

Run pre-commit checks:

```bash
pre-commit run --all-files
```

## Additional Resources

- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [QGroundControl Dev Guide](https://dev.qgroundcontrol.com/)
- [MAVLink Protocol](https://mavlink.io/)
