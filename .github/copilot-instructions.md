# QGroundControl AI Assistant Guide

Quick reference for AI coding assistants. For complete documentation, see the linked files.

## Quick Start

**Critical files to understand:**
1. `src/FactSystem/Fact.h` - Parameter system (READ FIRST!)
2. `src/Vehicle/Vehicle.h` - Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

**Golden Rules:**
- **Fact System**: ALL vehicle parameters use the Fact System. Never create custom parameter storage.
- **Multi-Vehicle**: ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`.

## Related Documentation

| Document | Purpose |
|----------|---------|
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution workflow, PR process, testing |
| [CODING_STYLE.md](../CODING_STYLE.md) | Naming conventions, formatting, QML style |
| [tools/README.md](../tools/README.md) | Build commands, development scripts |
| [CLAUDE.md](../CLAUDE.md) | Quick build commands reference |

## Architecture Patterns

### 1. Fact System (Most Important!)

All vehicle parameters use this system. Never bypass it.

```cpp
// Access parameters (always null-check!)
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) return;

Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (!param) {
    qCWarning(Log) << "Parameter not found";
    return;
}

// Validate before setting
if (param->validate(newValue, false).isEmpty()) {
    param->setCookedValue(newValue);  // UI value (with units)
    // param->rawValue() for MAVLink/storage
}

// Listen to changes
connect(param, &Fact::valueChanged, this, [](QVariant value) {
    qCDebug(Log) << "Parameter changed:" << value;
});

// Expose to QML
Q_PROPERTY(Fact* myParam READ myParam CONSTANT)
```

**Key classes:**
- `Fact` - Single parameter with validation, units, metadata
- `FactGroup` - Hierarchical container (handles MAVLink via `handleMessage()`)
- `FactMetaData` - JSON-based metadata (min/max, enums, descriptions)

**Rules:**
- Wait for `parametersReady` signal before accessing
- Use `cookedValue` (display) vs `rawValue` (storage)
- Metadata in `*.FactMetaData.json` files

### 2. Plugin Architecture

Three types handle firmware customization:

```cpp
// FirmwarePlugin - Firmware behavior (flight modes, capabilities)
vehicle->firmwarePlugin()->flightModes();
vehicle->firmwarePlugin()->isCapable(capability);

// AutoPilotPlugin - Vehicle setup UI (returns VehicleComponent list)
// VehicleComponent - Individual setup items (Radio, Sensors, Safety)
class MyComponent : public VehicleComponent {
    QString name() const override { return "My Component"; }
    bool setupComplete() const override { return _setupComplete; }
    QUrl setupSource() const override { return QUrl("qrc:/qml/MySetup.qml"); }
};
```

### 3. Manager Singletons

```cpp
// Always null-check activeVehicle (multi-vehicle support)
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (vehicle) {
    vehicle->parameterManager()->getParameter(...);
}

// Other managers
SettingsManager::instance()->appSettings()->...
LinkManager::instance()->...
```

### 4. QML/C++ Integration

```cpp
Q_OBJECT
QML_ELEMENT           // Creatable in QML
QML_SINGLETON         // Singleton
QML_UNCREATABLE("")   // C++-only

Q_MOC_INCLUDE("Vehicle.h")  // For forward-declared types in Q_PROPERTY
Q_PROPERTY(Type name READ getter WRITE setter NOTIFY signal)
Q_INVOKABLE void method();
Q_ENUM(EnumType)
```

### 5. State Machines

For complex workflows (parameter loading, calibration):

```cpp
QGCStateMachine machine("Workflow", vehicle);
auto* sendCmd = new SendMavlinkCommandState("SendCmd", &machine);
auto* waitMsg = new WaitForMavlinkMessageState("WaitResponse", &machine);
sendCmd->addTransition(sendCmd, &SendMavlinkCommandState::done, waitMsg);
machine.start();
```

### 6. Settings Framework

```cpp
class MySettings : public SettingsGroup {
    DEFINE_SETTINGFACT(settingName)  // Creates Fact with JSON metadata
};

// Create MySettings.SettingsGroup.json with metadata
// Access: SettingsManager::instance()->mySettings()->settingName()
```

### 7. MAVLink Message Handling

```cpp
void MyFactGroup::handleMessage(Vehicle* vehicle, mavlink_message_t& message) {
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MY_MESSAGE: {
        mavlink_my_message_t msg;
        mavlink_msg_my_message_decode(&message, &msg);
        myFact()->setRawValue(msg.value);  // Thread-safe via Qt signals
        break;
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

## Common Pitfalls

1. Assume single vehicle - Always null-check `activeVehicle()`
2. Access Facts before `parametersReady` signal
3. Hardcode parameter names - Use Fact System
4. Bypass FirmwarePlugin for firmware behavior
5. Use `Q_ASSERT` in production - Use defensive checks (Q_ASSERT compiles out in release)
6. Ignore platform differences - Check `#ifdef Q_OS_*`
7. Mix cookedValue/rawValue without conversion
8. Create custom parameter storage - Use Fact System
9. Access FactGroups before `telemetryAvailable`
10. Forget to emit property signals (breaks QML bindings)

## Performance & Threading

- **Batch updates**: `fact->setSendValueChangedSignals(false)`
- **Suppress live updates**: `factGroup->setLiveUpdates(false)`
- **Memory**: Qt parent/child ownership for auto-cleanup
- **Threading**: MAVLink on LinkManager thread, UI on main thread
- **Cross-thread signals**: Use `Qt::QueuedConnection`
- **Cleanup**: Use `deleteLater()` for objects with active signals

## QML Quick Reference

```qml
import QtQuick
import QGroundControl
import QGroundControl.Controls

QGCButton {
    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle

    enabled: vehicle && vehicle.armed

    onClicked: {
        if (vehicle) {  // Always null-check!
            vehicle.sendMavCommand(...)
        }
    }
}

// Qt6 Connections syntax (required)
Connections {
    target: vehicle
    function onArmedChanged() { }  // NOT: onArmedChanged: { }
}
```

**QML Rules:**
- No hardcoded sizes - Use `ScreenTools.defaultFontPixelHeight/Width`
- No hardcoded colors - Use `QGCPalette`
- Use QGC controls: `QGCButton`, `QGCLabel`, `QGCTextField`
- Translations: Wrap strings with `qsTr()`

## Logging

```cpp
// Header
Q_DECLARE_LOGGING_CATEGORY(MyLog)

// Source (use QGC macro for runtime config)
QGC_LOGGING_CATEGORY(MyLog, "qgc.component.name")

qCDebug(MyLog) << "Debug message";
qCWarning(MyLog) << "Warning message";
qCCritical(MyLog) << "Critical error";
```

---

**Key Principle**: Match the style of code you're editing. Use defensive coding, validate inputs, handle errors gracefully, and respect the Fact System architecture.
