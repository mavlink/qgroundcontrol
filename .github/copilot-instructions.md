# QGroundControl - AI Assistant Guide

Ground Control Station for MAVLink-enabled UAVs supporting PX4 and ArduPilot.

## Quick Start

**Read these files first:**
1. **[CONTRIBUTING.md](CONTRIBUTING.md)** - Coding standards and architecture patterns
2. `src/FactSystem/Fact.h` - The most important pattern in QGC
3. `src/Vehicle/Vehicle.h` - Core vehicle model

## Tech Stack

- **C++20** with **Qt 6.10.1** (QtQml, QtQuick)
- **Build**: CMake 3.25+, Ninja
- **Protocol**: MAVLink 2.0
- **Platforms**: Windows, macOS, Linux, Android, iOS

## Build Commands

```bash
git submodule update --init --recursive
~/Qt/6.10.1/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/Debug/QGroundControl --unittest  # Run tests
```

## Critical Rules

### The Two Golden Rules

1. **Fact System**: ALL vehicle parameters use the Fact System. Never create custom parameter storage.
2. **Multi-Vehicle**: ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`.

### Common Pitfalls (DO NOT!)

1. ❌ Assume single vehicle - Always null-check `activeVehicle()`
2. ❌ Access Facts before `parametersReady` signal
3. ❌ Hardcode parameter names - Use Fact System
4. ❌ Bypass FirmwarePlugin for firmware behavior
5. ❌ Use `Q_ASSERT` in production - Use defensive checks
6. ❌ Ignore platform differences - Check `#ifdef Q_OS_*`
7. ❌ Mix cookedValue/rawValue without conversion
8. ❌ Create custom parameter storage - Use Fact System
9. ❌ Forget to emit property signals (breaks QML bindings)

## Code Examples

### Working with Vehicle Parameters

```cpp
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) return;

Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (!param) {
    qCWarning(Log) << "Parameter not found";
    return;
}

QString error = param->validate(newValue, false);
if (!error.isEmpty()) {
    qCWarning(Log) << "Invalid value:" << error;
    return;
}

param->setCookedValue(newValue);
```

### Creating a Settings Group

```cpp
// MySettings.h
class MySettings : public SettingsGroup {
    Q_OBJECT
public:
    DEFINE_SETTINGFACT(mySetting)
};

// MySettings.SettingsGroup.json
{
    "mySetting": {
        "shortDescription": "My setting",
        "type": "uint32",
        "default": 100,
        "min": 0,
        "max": 1000
    }
}

// Access anywhere
int value = SettingsManager::instance()->mySettings()->mySetting()->rawValue().toInt();
```

### Handling MAVLink Messages

```cpp
void MyFactGroup::handleMessage(Vehicle* vehicle, mavlink_message_t& message) {
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MY_MESSAGE: {
        mavlink_my_message_t msg;
        mavlink_msg_my_message_decode(&message, &msg);
        myFact()->setRawValue(msg.value);
        break;
    }
    }
}
```

### QML Component

```qml
import QtQuick
import QGroundControl
import QGroundControl.Controls

QGCButton {
    text: "My Action"
    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    enabled: vehicle && vehicle.armed

    onClicked: {
        if (vehicle) {
            vehicle.sendMavCommand(...)
        }
    }
}
```

## Code Structure

```
src/
├── Vehicle/            # Vehicle state/comms (Vehicle.h is key)
├── FactSystem/         # Parameter management (READ THIS FIRST!)
├── FirmwarePlugin/     # PX4/ArduPilot abstraction
├── AutoPilotPlugins/   # Vehicle setup UI
├── MissionManager/     # Mission planning
├── MAVLink/            # Protocol handling
├── Comms/              # Serial/UDP/TCP/Bluetooth
├── Settings/           # Persistent settings
├── UI/                 # QML UI (MainWindow.qml)
└── Utilities/          # StateMachine, helpers
```

## Style Reference

For complete coding standards, naming conventions, and architecture patterns, see **[CONTRIBUTING.md](CONTRIBUTING.md#coding-standards)**.

**Quick reference:**
- Classes/Enums: `PascalCase`
- Methods/properties: `camelCase`
- Private members: `_leadingUnderscore`
- Files: `ClassName.h`, `ClassName.cc`
- Use `.clang-format`, `.clang-tidy` from repo root
- Minimal comments - code should be self-documenting

## Resources

- **Dev Guide**: https://dev.qgroundcontrol.com/
- **User Manual**: https://docs.qgroundcontrol.com/
- **MAVLink**: https://mavlink.io/
- **Qt Docs**: https://doc.qt.io/qt-6/

---

**Key Principle**: Match the style of code you're editing. Use defensive coding, validate inputs, handle errors gracefully, and respect the Fact System architecture.
