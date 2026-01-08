# QGroundControl AI Assistant Guide

Quick reference for AI coding assistants. **Read the linked docs for details.**

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` - Parameter system foundation
2. `src/Vehicle/Vehicle.h` - Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

## Golden Rules

1. **Fact System**: ALL vehicle parameters use Facts. Never create custom parameter storage.
2. **Multi-Vehicle**: ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`
3. **Firmware Plugin**: Use `vehicle->firmwarePlugin()` for firmware-specific behavior
4. **QML Sizing**: Use `ScreenTools.defaultFontPixelHeight/Width`, never hardcoded values
5. **QML Colors**: Use `QGCPalette`, never hardcoded colors

## Documentation

| Document | Content |
|----------|---------|
| [CONTRIBUTING.md](CONTRIBUTING.md) | Architecture patterns, Fact System, PR process |
| [CODING_STYLE.md](../CODING_STYLE.md) | Naming, formatting, QML style, C++20 features |
| [tools/README.md](../tools/README.md) | Build commands, development scripts |
| [Dev Guide](https://dev.qgroundcontrol.com/) | Complete developer documentation |

## Code Structure

```
src/
├── Vehicle/          # Vehicle state/comms
├── FactSystem/       # Parameter management
├── FirmwarePlugin/   # PX4/ArduPilot abstraction
├── AutoPilotPlugins/ # Vehicle setup UI
├── MissionManager/   # Mission planning
├── MAVLink/          # Protocol handling
├── QmlControls/      # Reusable QML components
└── Settings/         # Persistent settings
```

## Quick Patterns

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

---

**Key Principle**: Match the style of code you're editing. See CODING_STYLE.md for conventions.
