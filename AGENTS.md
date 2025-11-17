# QGroundControl Quick Reference for AI Assistants

**Ground Control Station** for UAVs using MAVLink protocol. **C++20/Qt 6.10.0** with QML UI.

## 🔑 Most Critical Architecture Pattern

**Fact System** - QGC's type-safe parameter management. Every vehicle parameter uses it.

```cpp
// ALWAYS use this pattern for parameters
Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (param) {
    param->setCookedValue(newValue);  // cookedValue = UI (with units)
    // param->rawValue() = MAVLink/storage
}
```

**Rules:**
- Wait for `parametersReady` signal before accessing
- Use cookedValue (display) vs rawValue (storage)
- Never create custom parameter storage

## 🏗️ Key Patterns

1. **Plugins**: FirmwarePlugin (PX4/ArduPilot behavior), AutoPilotPlugin (setup UI), VehicleComponent (individual items)
2. **Managers**: Singleton pattern - `MultiVehicleManager::instance()->activeVehicle()` (always null-check!)
3. **QML Integration**: `QML_ELEMENT`, `Q_PROPERTY`, `Q_INVOKABLE`
4. **State Machines**: Use `QGCStateMachine` for complex workflows (calibration, parameter loading)

## 📂 Code Structure

```
src/
├── FactSystem/         # Parameter management (READ FIRST!)
├── Vehicle/            # Vehicle state (Vehicle.h is critical)
├── FirmwarePlugin/     # PX4/ArduPilot abstraction
├── AutoPilotPlugins/   # Vehicle setup UI
├── MissionManager/     # Mission planning
├── Comms/              # Serial/UDP/TCP/Bluetooth links
```

## ⚡ Quick Build

```bash
git submodule update --init --recursive
~/Qt/6.10.0/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/Debug/QGroundControl --unittest  # Run tests
```

## ❌ Common Mistakes (DO NOT!)

1. Assume single vehicle (always null-check `activeVehicle()`)
2. Access Facts before `parametersReady`
3. Use `Q_ASSERT` in production code
4. Bypass FirmwarePlugin for firmware-specific behavior
5. Mix cookedValue/rawValue without conversion

## 📖 Essential Reading

1. **`.github/copilot-instructions.md`** - Detailed architecture guide
2. **`.github/CONTRIBUTING.md`** - Contribution guidelines
3. **`src/FactSystem/Fact.h`** - Parameter system (MOST CRITICAL!)
4. **`src/Vehicle/Vehicle.h`** - Vehicle model (~1477 lines)

## 🧑‍💻 Coding Style

- **Naming**: Classes `PascalCase`, methods `camelCase`, privates `_leadingUnderscore`
- **Files**: `ClassName.h`, `ClassName.cc`
- **Defensive**: Always validate inputs, null-check pointers, early returns
- **Logging**: `QGC_LOGGING_CATEGORY(MyLog, "qgc.component")` + `qCDebug(MyLog)`
- **Braces**: Always use, even for single-line if statements
- **Formatting**: `.clang-format`, `.clang-tidy`, `.editorconfig` configured in repo root
- **Comments**: Keep minimal - code should be self-documenting. No verbose headers or documentation files unless explicitly requested.

## 🔗 Resources

- **Dev Guide**: https://dev.qgroundcontrol.com/
- **User Docs**: https://docs.qgroundcontrol.com/
- **MAVLink**: https://mavlink.io/
- **Qt 6**: https://doc.qt.io/qt-6/

---

**Key Principle**: Match existing code style. Use defensive coding. Respect the Fact System architecture.
