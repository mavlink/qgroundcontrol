# QGroundControl Development Guide

Ground Control Station for MAVLink-enabled UAVs supporting PX4 and ArduPilot.

## Quick Start for AI Assistants

**First-time here?** Start with these critical files:
1. **This file** - Architecture patterns and coding standards
2. `.github/CONTRIBUTING.md` - Contribution workflow
3. `src/FactSystem/Fact.h` - The most important pattern in QGC
4. `src/Vehicle/Vehicle.h` - Core vehicle model

**Most Critical Pattern**: The **Fact System** handles ALL vehicle parameters. Never create custom parameter storage - always use Facts.

**Golden Rule**: Multi-vehicle support means ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`.

## Tech Stack
- **C++20** with **Qt 6.10.0** (QtQml, QtQuick)
- **Build**: CMake 3.25+, Ninja
- **Protocol**: MAVLink 2.0
- **Platforms**: Windows, macOS, Linux, Android, iOS

## Critical Architecture Patterns

### 1. Fact System (Parameter Management)
**Most important pattern in QGC** - All vehicle parameters use this.

```cpp
// Access parameters (always null-check!)
Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (param && param->validate(newValue, false).isEmpty()) {
    param->setCookedValue(newValue);  // Use cookedValue for UI (with units)
    // param->rawValue() for MAVLink/storage
}

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

**FirmwarePlugin** - Firmware behavior (flight modes, capabilities)
```cpp
virtual QList<FlightMode> flightModes() override;
virtual bool setFlightMode(const QString& mode) override;
```

**AutoPilotPlugin** - Vehicle setup UI (returns `VehicleComponent` list)

**VehicleComponent** - Individual setup items (Radio, Sensors, Safety)
```cpp
virtual QString name() const override;
virtual bool setupComplete() const override;
virtual QUrl setupSource() const override;  // QML UI
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
// Access: SettingsManager::instance()->mySettings()->settingName()
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

## Coding Standards

**Naming:**
- Classes/Enums: `PascalCase`
- Methods/properties: `camelCase`
- Private members: `_leadingUnderscore`
- Files: `ClassName.h`, `ClassName.cc`

**Logging:**
```cpp
Q_DECLARE_LOGGING_CATEGORY(MyLog)
QGC_LOGGING_CATEGORY(MyLog, "qgc.component.name")
qCDebug(MyLog) << "Message:" << value;
```

**Defensive Coding (Critical!):**
```cpp
void method(Vehicle* vehicle) {
    if (!vehicle) {
        qCWarning(Log) << "Invalid vehicle";
        return;  // Early return on invalid state
    }
    // Always use braces
    if (condition) {
        doSomething();
    }
}
```

**Code Style Tools:**
- Use `.clang-format`, `.clang-tidy`, `.editorconfig` configured in repo root
- Keep comments minimal - code should be self-documenting
- Do NOT create verbose file headers or unnecessary documentation files
- Do NOT add README files unless explicitly requested

**Security & Dependencies:**
- Never commit secrets, API keys, or credentials
- Validate all external inputs (MAVLink messages, file uploads, user input)
- Use Qt's built-in sanitization for SQL and string operations
- When adding dependencies, check for known vulnerabilities
- Prefer Qt's built-in functionality over external libraries

## Common Pitfalls (DO NOT!)

1. ❌ Assume single vehicle - Always null-check `activeVehicle()`
2. ❌ Access Facts before `parametersReady` signal
3. ❌ Hardcode parameter names - Use Fact System
4. ❌ Bypass FirmwarePlugin for firmware behavior
5. ❌ Use `Q_ASSERT` in production - Use defensive checks
6. ❌ Ignore platform differences - Check `#ifdef Q_OS_*`
7. ❌ Mix cookedValue/rawValue without conversion
8. ❌ Create custom parameter storage - Use Fact System
9. ❌ Access FactGroups before `telemetryAvailable`
10. ❌ Forget to emit property signals (breaks QML bindings)

## Build Commands
```bash
git submodule update --init --recursive
~/Qt/6.10.0/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/Debug/QGroundControl --unittest  # Run tests
```

**Key CMake Options** (cmake/CustomOptions.cmake):
- `QGC_STABLE_BUILD` - Release vs daily
- `QGC_BUILD_TESTING` - Unit tests
- `QGC_ENABLE_BLUETOOTH` - Bluetooth support
- `QGC_DISABLE_APM_PLUGIN` / `QGC_DISABLE_PX4_PLUGIN`

## Testing

### Running Tests
```bash
# Run all unit tests
./build/Debug/QGroundControl --unittest

# Run specific test
./build/Debug/QGroundControl --unittest:<TestClassName>

# Run with verbose output
./build/Debug/QGroundControl --unittest --logging:full
```

### Test Structure
- Tests mirror `src/` structure in `test/` directory
- Use `UnitTest` base class from Qt Test framework
- Mock vehicle connections when testing vehicle-dependent code
- Always test with null vehicle checks

### Adding New Tests
```cpp
class MyComponentTest : public UnitTest {
    Q_OBJECT
private slots:
    void init();    // Called before each test
    void cleanup(); // Called after each test
    void testMyFunction();
};
```

## Troubleshooting

### Build Issues
- **Qt not found**: Set `CMAKE_PREFIX_PATH` to Qt installation, or use qt-cmake
- **Submodule errors**: Run `git submodule update --init --recursive`
- **Missing dependencies**: Check platform-specific build instructions at https://dev.qgroundcontrol.com/
- **CMake cache issues**: Delete `build/` directory and reconfigure

### Runtime Issues
- **Crash on startup**: Check log files in `~/.local/share/QGroundControl/` (Linux/macOS) or `%LOCALAPPDATA%\QGroundControl` (Windows)
- **Vehicle not connecting**: Verify MAVLink protocol compatibility, check link configuration
- **Parameter load failures**: Ensure `parametersReady` signal before accessing Facts

### Development Environment
- **Qt Creator recommended**: Import CMakeLists.txt as project
- **clangd for VSCode**: Uses `.clangd` config in repo root
- **Pre-commit hooks**: Run `pre-commit install` to enable automatic formatting

## Performance Tips
- Batch updates: `fact->setSendValueChangedSignals(false)`
- Suppress live updates: `factGroup->setLiveUpdates(false)`
- Cache frequently accessed values
- MAVLink handled on separate thread
- Qt parent/child ownership for cleanup

## Common Tasks

### Working with Vehicle Parameters
```cpp
// 1. Get parameter (always null-check!)
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) return;

Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (!param) {
    qCWarning(Log) << "Parameter not found";
    return;
}

// 2. Validate before setting
QString error = param->validate(newValue, false);
if (!error.isEmpty()) {
    qCWarning(Log) << "Invalid value:" << error;
    return;
}

// 3. Set value (cookedValue for UI with units)
param->setCookedValue(newValue);

// 4. Listen to changes
connect(param, &Fact::valueChanged, this, [](QVariant value) {
    qCDebug(Log) << "Parameter changed:" << value;
});
```

### Creating a Settings Group
```cpp
// 1. Define in MySettings.h
class MySettings : public SettingsGroup {
    Q_OBJECT
public:
    DEFINE_SETTINGFACT(mySetting)  // Creates Fact with JSON metadata
};

// 2. Create MySettings.SettingsGroup.json with metadata
{
    "mySetting": {
        "shortDescription": "My setting",
        "type": "uint32",
        "default": 100,
        "min": 0,
        "max": 1000
    }
}

// 3. Access anywhere
int value = SettingsManager::instance()->mySettings()->mySetting()->rawValue().toInt();
```

### Adding a Vehicle Component
```cpp
// 1. Create MyComponent.h (subclass VehicleComponent)
class MyComponent : public VehicleComponent {
    Q_OBJECT
public:
    MyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    QString name() const override { return "My Component"; }
    QString description() const override { return "Component description"; }
    QString iconResource() const override { return "/qmlimages/MyComponentIcon.svg"; }
    bool requiresSetup() const override { return true; }
    bool setupComplete() const override { return _setupComplete; }
    QUrl setupSource() const override { return QUrl::fromUserInput("qrc:/qml/MyComponentSetup.qml"); }
};

// 2. Register in AutoPilotPlugin::getVehicleComponents()
```

### Handling MAVLink Messages
```cpp
// In a FactGroup or custom component
void MyFactGroup::handleMessage(Vehicle* vehicle, mavlink_message_t& message) {
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MY_MESSAGE: {
        mavlink_my_message_t msg;
        mavlink_msg_my_message_decode(&message, &msg);

        // Update Facts (thread-safe via Qt signals)
        myFact()->setRawValue(msg.value);
        break;
    }
    }
}
```

### Adding a QML UI Component
```qml
// 1. Create MyControl.qml
import QtQuick
import QGroundControl
import QGroundControl.Controls

QGCButton {
    text: "My Action"

    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle

    enabled: vehicle && vehicle.armed

    onClicked: {
        if (vehicle) {
            // Always null-check vehicle!
            vehicle.sendMavCommand(...)
        }
    }
}
```

## Essential Files to Read
1. `.github/CONTRIBUTING.md` - Contribution guidelines
2. `src/FactSystem/Fact.h` - Parameter system (CRITICAL!)
3. `src/Vehicle/Vehicle.h` - Vehicle model
4. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

## Resources
- **User Manual**: https://docs.qgroundcontrol.com/
- **Dev Guide**: https://dev.qgroundcontrol.com/
- **MAVLink**: https://mavlink.io/
- **Qt Docs**: https://doc.qt.io/qt-6/

## Memory & Threading
- Qt parent/child ownership (auto-cleanup)
- Use `deleteLater()` for objects with active signals
- `Qt::QueuedConnection` for cross-thread signals
- MAVLink on LinkManager thread, UI on main thread

---

**Key Principle**: Match the style of code you're editing. Use defensive coding, validate inputs, handle errors gracefully, and respect the Fact System architecture.
