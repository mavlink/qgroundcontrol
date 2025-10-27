# QGroundControl Development Guidelines

## Project Overview
QGroundControl (QGC) is an intuitive and powerful Ground Control Station (GCS) for UAVs. It provides comprehensive flight control and mission planning for MAVLink-enabled drones, with full support for PX4 and ArduPilot platforms.

## Technology Stack
- **Language**: C++ (Qt framework) and QML for UI
- **Build System**: CMake (minimum version 3.25)
- **Communication Protocol**: MAVLink
- **Supported Platforms**: Desktop (Windows, macOS, Linux), Mobile (Android, iOS)
- **Qt Integration**: QtQmlIntegration (QML_ELEMENT, QML_SINGLETON, QML_UNCREATABLE)
- **Architecture**: Manager singletons, Plugin system, Fact System for parameters

## Code Organization
- `src/` - Main source code directory
  - `Vehicle/` - Vehicle management and communication
  - `MissionManager/` - Mission planning and execution
  - `FlightDisplay/` - Flight control UI
  - `QmlControls/` - Reusable QML components
  - `FirmwarePlugin/` - PX4 and ArduPilot firmware interfaces
  - `AutoPilotPlugins/` - Vehicle-specific configuration
  - `Settings/` - Application settings
  - `Camera/` - Camera control
  - `Comms/` - Communication links
  - `FactSystem/` - Parameter management (Fact, FactGroup, FactMetaData)
  - `API/` - Public API
  - `Utilities/` - Helper classes (JsonHelper, QmlObjectListModel, etc.)

## Core Architectural Patterns

### 1. Fact System (Parameter Management)
The **Fact System** is QGC's type-safe parameter management framework. Use it for all parameters.

**Key Classes:**
- `Fact` - Single parameter with value, validation, and metadata
- `FactGroup` - Hierarchical grouping of Facts (e.g., `VehicleGPSFactGroup`, `VehicleBatteryFactGroup`)
- `FactMetaData` - Metadata loaded from JSON (units, min/max, enums, descriptions)

**Pattern:**
```cpp
// Access via ParameterManager
Fact* fact = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (fact) {
    fact->setCookedValue(newValue);  // With unit conversion
    QString error = fact->validate(value, false);  // Validation
}

// Expose to QML
Q_PROPERTY(Fact* myParam READ myParam CONSTANT)
Fact* myParam() { return _vehicle->parameterManager()->getParameter(-1, "MY_PARAM"); }
```

**Important:**
- Always use `cookedValue` for display (with unit conversions)
- Use `rawValue` for MAVLink/internal storage
- Facts auto-notify on value changes
- FactGroups handle MAVLink message parsing via `handleMessage()`

### 2. Plugin Architecture

**Three plugin types:**

**A. FirmwarePlugin** - Firmware-specific behavior (PX4, ArduPilot)
- Override virtual methods: `flightModes()`, `setFlightMode()`, `isCapable()`, `parameterMetaDataFiles()`
- Registered via `FirmwarePluginFactory`

**B. AutoPilotPlugin** - Vehicle configuration components
- Returns list of `VehicleComponent` instances
- Handles setup completion tracking

**C. VehicleComponent** - Individual setup items (Radio, Sensors, Safety)
```cpp
class MyVehicleComponent : public VehicleComponent {
    virtual QString name() const override;
    virtual bool setupComplete() const override;
    virtual QUrl setupSource() const override;  // QML UI
    virtual QStringList setupCompleteChangedTriggerList() const override;  // Monitored params
};
```

### 3. Manager Pattern (Singletons)
QGC uses singleton managers for cross-cutting concerns:
- `MultiVehicleManager` - Active vehicles
- `LinkManager` - Communication links
- `SettingsManager` - Application settings
- `ParameterManager` - Per-vehicle parameter caching
- `QGCLoggingCategoryManager` - Runtime logging config

**Pattern:**
```cpp
class MyManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    static MyManager* instance();
    static void deleteInstance();
private:
    static MyManager* _instance;
};
```

### 4. QML/C++ Integration

**Registration:**
```cpp
Q_OBJECT
QML_ELEMENT           // Creatable in QML
QML_SINGLETON         // Singleton instance
QML_UNCREATABLE("")   // Abstract/C++-only
```

**Properties and Methods:**
```cpp
Q_PROPERTY(Type name READ getter WRITE setter NOTIFY signal)
Q_INVOKABLE void methodName();  // Callable from QML
Q_ENUM(EnumType)                // Expose enum to QML
```

**Controllers** - Bridge QML UI to C++ logic:
- Inherit from `FactPanelController` or `QObject`
- Provide Q_PROPERTIES for data binding
- Q_INVOKABLE methods for user actions
- Emit signals for async updates

### 5. State Machine Framework
Use `QGCStateMachine` for complex workflows:
```cpp
QGCStateMachine machine("Workflow", vehicle);
auto* sendCmd = new SendMavlinkCommandState("SendCmd", &machine);
auto* waitMsg = new WaitForMavlinkMessageState("WaitResponse", &machine);
auto* processFunc = new FunctionState("Process", &machine, []() { return true; });
// Add states and transitions, then machine.start()
```

**State types:** `DelayState`, `FunctionState`, `SendMavlinkCommandState`, `WaitForMavlinkMessageState`, `ShowAppMessageState`

### 6. Settings Framework
Use `DEFINE_SETTINGFACT` macro for persistent settings:
```cpp
class MySettings : public SettingsGroup {
    DEFINE_SETTINGFACT(settingName)
};
// Metadata in JSON: MySettings.SettingsGroup.json
```

Access via `SettingsManager::instance()->mySettings()->settingName()`

## Coding Style

### C++ Guidelines
- **Follow the existing code style** in the file you're editing
- Look at surrounding code for naming conventions, formatting, and patterns
- Private members typically prefixed with underscore: `_privateVariable`
- **Use defensive coding**: Check for null pointers, validate inputs, handle error cases
- Avoid assumptions - validate preconditions explicitly
- Return early on invalid conditions rather than continuing with bad state
- Document non-obvious private methods in implementation files
- Keep Qt headers and QGC headers separate in includes
- Use `QGC_LOGGING_CATEGORY` for logging (registered with runtime config)

**Logging Pattern:**
```cpp
// Header: Q_DECLARE_LOGGING_CATEGORY(MyComponentLog)
// Impl: QGC_LOGGING_CATEGORY(MyComponentLog, "qgc.category.name")
qCDebug(MyComponentLog) << "Debug:" << value;
qCWarning(MyComponentLog) << "Warning:" << error;
```

### Example Defensive Coding
```cpp
void ClassName::_privateMethod(Vehicle* vehicle)
{
    if (!vehicle) {
        qCWarning(LogCategory) << "Invalid vehicle pointer";
        return;
    }
    
    if (_lastSeenComponent == -1) {
        _lastSeenComponent = component;
    } else if (component != _lastSeenComponent) {
        qCWarning(LogCategory) << "Component mismatch detected";
        return;
    }
}
```

### QML Guidelines
- **Match the style of the QML file you're editing**
- Keep QML files focused and modular
- Use appropriate QGC controls from `QmlControls/`

## Build Instructions
1. CMake 3.25 or higher required
2. Default build type is Release
3. Custom builds supported via `custom/` directory
4. See [Developer Guide](https://dev.qgroundcontrol.com/en/getting_started/) for detailed build instructions

## MAVLink Protocol
- QGC communicates with vehicles using the MAVLink protocol
- Support for multiple concurrent vehicle connections
- Handle MAVLink messages in appropriate Vehicle or FirmwarePlugin classes

## Testing
- Test files located in `test/` directory
- Always test changes with both PX4 and ArduPilot if applicable
- Test on multiple platforms when possible

## Key Concepts
- **Vehicle**: Represents a connected drone/UAV (extends `VehicleFactGroup`)
  - Contains managers: `ParameterManager`, `MissionManager`, `GeoFenceManager`, `RallyPointManager`
  - Contains plugins: `AutoPilotPlugin`, `FirmwarePlugin`
  - Contains FactGroups: GPS, battery, attitude, terrain, wind, etc.
- **Fact System**: Type-safe parameter management with metadata-driven validation
  - `Fact` - Single parameter with cooked/raw values and change signals
  - `FactGroup` - Hierarchical container for related Facts
  - `FactMetaData` - JSON-based metadata (units, ranges, enums)
- **Mission Items**: Individual mission commands (waypoints, takeoff, land, etc.)
- **Firmware Plugin**: Abstraction layer for different autopilot firmware (PX4, ArduPilot)
- **Communication Links**: Serial, UDP, TCP connections to vehicles via `LinkManager`
- **QmlObjectListModel**: Standard list model for QML with dirty tracking

## Common Utility Classes
- `JsonHelper` - JSON parsing and validation utilities
- `QmlObjectListModel` - QAbstractListModel for QML with count/dirty properties
- `QGCMapPolygon` / `QGCMapCircle` - Geofence/survey geometry
- `FileHelper` / `QGCTemporaryFile` - File operations
- `QGCFileDownload` - HTTP file downloads
- `ShapeFileHelper` - Shapefile reading

## Documentation
- User Manual: https://docs.qgroundcontrol.com/en/
- Developer Guide: https://dev.qgroundcontrol.com/en/
- Contributing Guide: .github/CONTRIBUTING.md

## Important Files
- `CMakeLists.txt` - Main build configuration
- `.github/CONTRIBUTING.md` - Contribution guidelines
- `CHANGELOG.md` - Release notes and version history

## When Making Changes
1. **Match the style of the code you're editing** - look at surrounding files
2. Use defensive coding - validate inputs, check for null, handle errors gracefully
3. Test with real hardware/SITL when possible
4. Consider impact on both PX4 and ArduPilot users
5. Update documentation if adding new features
6. Keep changes focused and atomic
7. Add appropriate logging using Q_LOGGING_CATEGORY
8. Use Qt's signal/slot mechanism for loose coupling
9. Respect the existing architecture and patterns

## Common Pitfalls to Avoid
- **Don't** break MAVLink protocol compatibility
- **Don't** assume single vehicle operation (support multiple vehicles via `MultiVehicleManager`)
- **Don't** hardcode values that should be configurable (use Settings or Facts)
- **Don't** ignore platform differences (desktop vs mobile, check with `#ifdef`)
- **Don't** forget to handle cleanup in destructors (Qt parent/child ownership)
- **Don't** assume pointers are valid - always check before dereferencing
- **Don't** use `Q_ASSERT` in production code - use defensive checks with logging instead
- **Don't** access Fact values before `parametersReady` is true
- **Don't** modify parameters without validation via `Fact::validate()`
- **Don't** create custom parameter storage - use the Fact System
- **Don't** bypass FirmwarePlugin for firmware-specific operations
- **Don't** use raw parameter names without checking `parameterExists()`
- **Don't** forget to emit property change signals (breaks QML bindings)
- **Don't** use direct method calls where signals/slots are more appropriate
- **Don't** access FactGroups before `telemetryAvailable` is true
- **Don't** mix cooked and raw values without proper translation

## Implementation Patterns

### Adding a New Parameter
1. Ensure parameter exists in firmware metadata JSON
2. Access via `vehicle->parameterManager()->getParameter(componentId, "PARAM_NAME")`
3. Expose to QML via Q_PROPERTY returning `Fact*`
4. Use `Fact::validate()` before setting new values
5. Listen to `valueChanged()` signal for updates

### Creating a New FactGroup
```cpp
class MyFactGroup : public FactGroup {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(Fact* param1 READ param1 CONSTANT)

public:
    MyFactGroup(QObject* parent = nullptr) : FactGroup(0, parent) {
        _addFact(_param1Fact = new Fact(1, "PARAM1", FactMetaData::valueTypeInt32, this));
    }
    Fact* param1() const { return _param1Fact; }

    virtual void handleMessage(Vehicle*, const mavlink_message_t& msg) override {
        // Parse MAVLink and update facts
    }
};
```

### Extending a Plugin
1. Create subclass of `FirmwarePlugin` or `AutoPilotPlugin`
2. Override virtual methods for custom behavior
3. Register in appropriate factory
4. Test with specific firmware type

### Creating a Controller for QML
```cpp
class MyController : public FactPanelController {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    Q_INVOKABLE void performAction() {
        // Validate, perform operation, emit signals
    }

signals:
    void statusChanged();
};
```

### Working with QmlObjectListModel
```cpp
QmlObjectListModel* model = new QmlObjectListModel(this);
model->append(new MyObject(this));
model->setDirty(true);  // Trigger dirtyChanged signal
connect(model, &QmlObjectListModel::countChanged, this, &MyClass::onCountChanged);
```

## Best Practices Summary

### Memory Management
- Rely on Qt parent/child ownership (objects deleted with parent)
- Use `deleteLater()` for objects with active signals
- Disconnect signals before manual deletion if not parent/child
- Smart pointers (`QSharedPointer`) for shared ownership

### Thread Safety
- Qt objects tied to creation thread
- Use `Qt::QueuedConnection` for cross-thread signals
- MAVLink messages processed on LinkManager thread
- UI updates must happen on main thread

### Signal/Slot Usage
- Prefer signals over direct calls for decoupling
- Use typed signals with parameters for data passing
- `connect()` with lambda for inline handlers
- Use `QueuedConnection` when thread boundaries involved

### Performance Considerations
- Batch Fact updates with `setSendValueChangedSignals(false)`
- Use `setLiveUpdates(false)` on FactGroups during bulk updates
- Avoid expensive operations in Q_PROPERTY getters
- Cache frequently accessed values

## Resources
- Official Website: http://qgroundcontrol.com
- GitHub: https://github.com/mavlink/qgroundcontrol
- Support: https://docs.qgroundcontrol.com/en/Support/Support.html
- MAVLink: https://mavlink.io/
- Qt Documentation: https://doc.qt.io/
