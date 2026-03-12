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

## CI Structure

Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`, `android.yml`, `ios.yml`) share logic via composite actions and reusable workflows.

```
.github/
├── workflows/
│   ├── linux.yml, macos.yml, ...  # Platform build + test
│   ├── _detect-changes.yml        # Reusable: skip builds on unrelated PRs
│   ├── build-results.yml          # Aggregate PR comment (workflow_run trigger)
│   └── build-gstreamer.yml        # GStreamer SDK builds
├── actions/
│   ├── cmake-configure/           # CMake configure with consistent options
│   ├── cmake-build/               # Build with timing, reviewdog, ccache
│   ├── run-unit-tests/            # CTest runner with JUnit output
│   ├── detect-changes/            # Path-based change detection per platform
│   ├── attest-and-upload/         # SBOM attestation + artifact upload
│   ├── deploy-docs/               # Deploy built docs to external repo
│   ├── gstreamer/                 # Build GStreamer from source
│   ├── setup-python/              # Python + uv + dependency installation
│   └── qt-install/                # Qt SDK installation with caching
├── scripts/                       # Python scripts for CI jobs
│   ├── common/                    # Shared modules (gh_actions, build_config)
│   ├── templates/                 # Jinja2 templates for generated output
│   └── tests/                     # Tests for CI scripts
└── build-config.json              # Centralized version numbers
```

### CI Conventions

- **Dependencies**: CI Python scripts use `httpx` for GitHub API access and `jinja2` for templating. Deps managed in `tools/pyproject.toml` under `[project.optional-dependencies] scripts`.
- **Shared helpers**: `gh_actions.py` provides GitHub API pagination (httpx) with `gh` CLI fallback. Import as `from common.gh_actions import ...`.
- **Bootstrap scripts** (`install_dependencies.py`, `ccache_helper.py`): Use stdlib only — they run before dependencies are installed.
- **Config**: Version numbers and build settings live in `.github/build-config.json`. Read via `common.build_config.get_build_config_value()`.
- **Outputs**: Use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

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
