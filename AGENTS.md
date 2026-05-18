# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [tools/README.md](tools/README.md) — Development scripts and tooling
- [test/TESTING.md](test/TESTING.md) — Test framework, base classes, CTest labels, MultiSignalSpy, coverage
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — All enforced linters (clang-format, clang-tidy, ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy, vehicle-null-check, check-no-qassert)

## Review Process

Your output will be reviewed by another AI agent before being accepted. Write code and commit messages that are easy to machine-review: keep changes focused and minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes, commented-out code, or ambiguous TODOs.

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` - Parameter system foundation
2. `src/Vehicle/Vehicle.h` - Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

## Build & Test Commands

See [tools/README.md](tools/README.md) for the full tooling reference and [test/TESTING.md](test/TESTING.md) for the complete testing guide.

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

## Android: Install + Logcat

Workflow for installing a debug-built APK to a connected device and capturing logs.

Prereqs (on PATH): `adb`, `zipalign`, `apksigner` (Android `build-tools/<ver>/`). Debug keystore at `~/.android/debug.keystore` (alias `androiddebugkey`, password `android`).

```bash
# Adjust APK path to your Qt-Android build kit/configuration
APK=build/Qt_6_10_3_for_Android_arm64_v8a-Debug/android-build-QGroundControl/QGroundControl.apk

# Re-sign with the debug keystore (Qt's androiddeployqt output is unsigned/misaligned for adb install)
zipalign -p -f 4 "$APK" "${APK%.apk}-aligned.apk"
apksigner sign --ks ~/.android/debug.keystore --ks-pass pass:android --ks-key-alias androiddebugkey \
    --key-pass pass:android --out "${APK%.apk}-signed.apk" "${APK%.apk}-aligned.apk"

# Install (replace existing); -r keeps data, add -d to allow downgrade
adb install -r "${APK%.apk}-signed.apk"

# Launch (force-stop first for a clean session)
adb shell am force-stop org.mavlink.qgroundcontrol
adb shell am start -n org.mavlink.qgroundcontrol/.QGCActivity

# Logcat: clear, then stream QGC + serial-related tags only
adb logcat -c
adb logcat -v time \
    QGroundControl:V QGCActivity:V QGCUsbSerialManager:V QGCUsbPermissionHandler:V \
    QGCFtdiSerialDriver:V QGCSerialListener:V QGCUsbSerialProber:V \
    AsyncUsbWritePump:V UsbSerialEnumerator:V UsbSerialIoBridge:V UsbSerialLifecycle:V '*:S'

# Enable per-tag VERBOSE for code paths gated on Log.isLoggable(TAG, VERBOSE).
# Tag must be ≤23 chars; setprop survives until reboot.
adb shell setprop log.tag.QGCSerialListener VERBOSE
adb shell setprop log.tag.UsbSerialIoBridge VERBOSE
adb shell setprop log.tag.AsyncUsbWritePump VERBOSE

# Qt-side verbose categories — set via QT_LOGGING_RULES before launch, or in-app via the log viewer
adb shell am start -n org.mavlink.qgroundcontrol/.QGCActivity \
    --es "QT_LOGGING_RULES" "Android.Serial.Engine.debug=true;VehicleSetup.FirmwareUpgrade.debug=true"
```

Tip: run logcat in a background shell (`run_in_background=true`) and grep the captured file rather than streaming into the agent context.

## Golden Rules

1. **Fact System**: ALL vehicle parameters use Facts. Never create custom parameter storage.
2. **Multi-Vehicle**: ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`
3. **Firmware Plugin**: Use `vehicle->firmwarePlugin()` for firmware-specific behavior.
4. **QML Sizing**: Use `ScreenTools.defaultFontPixelHeight/Width`, never hardcoded values.
5. **QML Colors**: Use `QGCPalette`, never hardcoded colors.
6. **Match existing style**: Follow conventions of surrounding code. See CODING_STYLE.md.

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
│   ├── linux.yml, macos.yml, windows.yml  # Platform build + test
│   ├── android.yml, ios.yml               # Mobile builds
│   ├── _detect-changes.yml                # Reusable: skip builds on unrelated PRs
│   ├── build-results.yml                  # Aggregate PR comment (workflow_run trigger)
│   ├── build-gstreamer.yml                # GStreamer SDK builds
│   ├── custom-build.yml                   # Custom build validation
│   ├── docker.yml                         # Docker image builds
│   ├── pre-commit.yml                     # Linting and formatting checks
│   ├── check-links.yml                    # Markdown link validation
│   ├── ci-scripts.yml                     # CI Python script tests
│   ├── analysis.yml                       # Static analysis
│   ├── performance.yml                    # Performance benchmarks
│   ├── pr-checks.yml                      # PR validation checks
│   ├── release.yml                        # Release automation
│   ├── docs_deploy.yml, doxygen_deploy.yml  # Documentation deployment
│   ├── cache-cleanup.yml, cache-cleanup-pr.yml  # Cache maintenance
│   ├── crowdin.yml, lupdate.yml           # Translation workflows
│   ├── dependency-review.yml              # Dependency security review
│   ├── scorecard.yml                      # OpenSSF Scorecard
│   ├── flatpak.yml                        # Flatpak builds
│   ├── px4-metadata.yml                   # PX4 metadata sync
│   ├── stale.yml                          # Stale issue/PR management
│   └── welcome.yml                        # New contributor welcome
├── actions/
│   ├── cmake-configure/                   # CMake configure with consistent options
│   ├── cmake-build/                       # Build with timing, reviewdog, ccache
│   ├── run-unit-tests/                    # CTest runner with JUnit output
│   ├── detect-changes/                    # Path-based change detection per platform
│   ├── attest-and-upload/                 # SBOM attestation + artifact upload
│   ├── attest-sbom/                       # SBOM generation and attestation
│   ├── deploy-docs/                       # Deploy built docs to external repo
│   ├── gstreamer/                         # Build GStreamer from source
│   ├── setup-python/                      # Python + uv + dependency installation
│   ├── qt-install/                        # Qt SDK installation with caching
│   ├── qt-android/, qt-ios/               # Mobile Qt setup
│   ├── coverage/                          # Code coverage reports
│   ├── test-report/                       # Test result publishing
│   ├── test-duration-report/              # Test timing analysis
│   ├── benchmark-runner/                  # Performance benchmark runner
│   ├── size-analysis/                     # Binary size tracking
│   ├── collect-artifact-sizes/            # Artifact size collection
│   ├── download-all-artifacts/            # Cross-workflow artifact download
│   ├── build-config/                      # Read build-config.json values
│   ├── build-setup/                       # Common build environment setup
│   ├── build-action/                      # Unified build action
│   ├── cache/                             # Caching helpers
│   ├── custom-build/                      # Custom build support
│   ├── docker/                            # Docker build helpers
│   ├── install-dependencies/              # Platform dependency installation
│   ├── aws-upload/                        # AWS S3 upload
│   ├── playstore/                         # Google Play Store upload
│   ├── upload/                            # Generic artifact upload
│   ├── verify-executable/                 # Post-build executable verification
│   └── common/                            # Shared action utilities
├── scripts/                               # Python scripts for CI jobs
│   ├── templates/                         # Jinja2 templates (build_results.md.j2)
│   └── tests/                             # Tests for CI scripts (pytest)
└── build-config.json                      # Centralized version numbers
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
