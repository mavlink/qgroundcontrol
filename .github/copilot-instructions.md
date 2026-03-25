# QGroundControl AI Assistant Guide

The canonical AI agent guide lives in [AGENTS.md](../AGENTS.md). This file provides a brief summary for GitHub Copilot; see AGENTS.md for the full reference including code structure, CI conventions, and quick patterns.

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
| [AGENTS.md](../AGENTS.md) | **Full AI guide**: code structure, CI, patterns, all references |
| [CODING_STYLE.md](../CODING_STYLE.md) | Naming, formatting, QML style, C++20 features |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Architecture patterns, Fact System, PR process |
| [tools/README.md](../tools/README.md) | Build commands, development scripts |
| [test/TESTING.md](../test/TESTING.md) | Test framework, base classes, CTest labels |

---

**Key Principle**: Match the style of code you're editing. See CODING_STYLE.md for conventions.
