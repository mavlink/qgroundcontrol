# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [.github/copilot-instructions.md](.github/copilot-instructions.md) — Code structure, CI structure, quick patterns
- [tools/README.md](tools/README.md) — Development scripts and tooling

## Build & Test Commands

```bash
# Configure (Release)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release --parallel

# Run unit tests
cd build && ctest --output-on-failure -L Unit --parallel $(nproc)

# Lint (pre-commit hooks)
pre-commit run --all-files

# Format C++
clang-format -i path/to/changed/files.cc

# CI Python script tests
cd .github/scripts && PYTHONPATH=. python3 -m pytest tests/ -q

# Tools Python tests
cd tools && uv run --extra scripts --extra test pytest tests/ -q
```

## Golden Rules

1. **Fact System**: ALL vehicle parameters use Facts. Never create custom parameter storage.
2. **Multi-Vehicle**: ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`
3. **Firmware Plugin**: Use `vehicle->firmwarePlugin()` for firmware-specific behavior.
4. **QML Sizing**: Use `ScreenTools.defaultFontPixelHeight/Width`, never hardcoded values.
5. **QML Colors**: Use `QGCPalette`, never hardcoded colors.
6. **Match existing style**: Follow conventions of surrounding code. See CODING_STYLE.md.

## Project Layout

```
src/                    # C++/QML application source
├── Vehicle/            # Vehicle state and communications
├── FactSystem/         # Parameter management (Fact, FactGroup, FactMetaData)
├── FirmwarePlugin/     # PX4/ArduPilot abstraction
├── MissionManager/     # Mission planning
├── MAVLink/            # Protocol handling
├── QmlControls/        # Reusable QML components
└── Settings/           # Persistent settings

tools/                  # Development scripts and shared Python modules
├── common/             # Shared Python modules (gh_actions, build_config, etc.)
├── setup/              # Environment setup scripts
├── analyzers/          # Static analysis tools
└── pyproject.toml      # Python dependencies (uv)

.github/
├── workflows/          # CI workflows (linux, macos, windows, android, ios)
├── actions/            # Composite actions (cmake-build, run-unit-tests, etc.)
├── scripts/            # CI-specific Python scripts
│   ├── common/         # Shared CI modules
│   └── templates/      # Jinja2 templates
└── build-config.json   # Centralized version numbers
```

## CI Conventions

- Platform workflows share logic via composite actions and reusable workflows.
- CI Python scripts use `httpx` for GitHub API and `jinja2` for templating.
- Bootstrap scripts (`install_dependencies.py`, `ccache_helper.py`) use stdlib only.
- Version numbers live in `.github/build-config.json`.
- Use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

## C++ Key Patterns

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
