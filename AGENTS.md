# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [tools/README.md](tools/README.md) — Development scripts and tooling
- [test/TESTING.md](test/TESTING.md) — Test framework, base classes, CTest labels, MultiSignalSpy, coverage
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — All enforced linters (clang-format, clang-tidy, ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy, vehicle-null-check, check-no-qassert, check-no-qtest-ignore-message)

## Review Process

Your output will be reviewed by another AI agent before being accepted. Write code and commit messages that are easy to machine-review: keep changes focused and minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes, commented-out code, or ambiguous TODOs.

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` - Parameter system foundation
2. `src/Vehicle/Vehicle.h` - Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

## Build & Test Commands

- Recommended workflow: [tools/README.md](tools/README.md) Quick Start (`just configure / build / test / lint / check`).
- Testing details (CTest labels, coverage, sanitizers): [test/TESTING.md](test/TESTING.md).
- CI Python script tests: [.github/ci-overview.md](.github/ci-overview.md#tests).

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
    QGCFtdiSerialDriver:V QGCSerialPort:V QGCUsbSerialProber:V \
    UsbSerialEnumerator:V UsbAttachDetachReceiver:V '*:S'

# Enable Java serial VERBOSE for code paths gated on Log.isLoggable(..., VERBOSE).
# Tag must be ≤23 chars; setprop survives until reboot.
adb shell setprop log.tag.QGCSerial VERBOSE

# Qt-side verbose categories — set via QT_LOGGING_RULES before launch, or in-app via the log viewer
adb shell am start -n org.mavlink.qgroundcontrol/.QGCActivity \
    --es "QT_LOGGING_RULES" "Android.Serial:verbose.debug=true;VehicleSetup.FirmwareUpgrade.debug=true"
```

Tip: run logcat in a background shell (`run_in_background=true`) and grep the captured file rather than streaming into the agent context.

### Running unit tests on a device

A `QGC_BUILD_TESTING=ON` APK is unittest-capable. Pass QGC's `--unittest` flags via the
`applicationArguments` intent extra (Qt's `QtLoader` appends them to argv):

```bash
adb shell am start -n org.mavlink.qgroundcontrol/org.mavlink.qgroundcontrol.QGCActivity \
    --es applicationArguments "--unittest:NmeaSerialDeviceTest --onscreen"
```

`--onscreen` is required — Qt-for-Android ships no offscreen QPA plugin, so the default test path
aborts with "no Qt platform plugin could be initialized". On-device is a smoke check only (clean
exit = no crash/ASSERT in logcat); QtTest stdout isn't routed to logcat and scoped storage blocks
reading `--unittest-output` XML. Run on the **host via `ctest`** for the authoritative verdict.

For a **dedicated CI test APK** (args fixed for the build), bake them in at configure time via Qt's
`QT_ANDROID_APPLICATION_ARGUMENTS` instead of the runtime intent — androiddeployqt writes them to the
manifest as `android.app.arguments` and `QtLoader` appends them to argv on launch (same codepath):

```bash
cmake ... -DQGC_BUILD_TESTING=ON -DQT_ANDROID_APPLICATION_ARGUMENTS="--unittest:NmeaSerialDeviceTest --onscreen"
adb shell am start -n org.mavlink.qgroundcontrol/org.mavlink.qgroundcontrol.QGCActivity  # no intent extra needed
```

Trade-off: static (reconfigure + redeploy to change the filter) and tech-preview (Qt 6.0+). Prefer the
intent extra above for ad-hoc/iterative runs; it also overrides the baked-in args at launch.

## Golden Rules

See [.github/CONTRIBUTING.md#architecture-patterns](.github/CONTRIBUTING.md#architecture-patterns) for the canonical list (Fact System, Multi-Vehicle null-check, FirmwarePlugin, QML integration) and [CODING_STYLE.md#common-pitfalls](CODING_STYLE.md#common-pitfalls) for the full pitfall list with code examples.

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

See [.github/ci-overview.md](.github/ci-overview.md) for the workflow/action/script layout and CI conventions (dependencies, shared helpers, bootstrap scripts, build config, GitHub Actions output).

---

**Key Principle**: Match the style of code you're editing. See [CODING_STYLE.md](CODING_STYLE.md) for conventions and [CODING_STYLE.md#examples](CODING_STYLE.md#examples) for canonical Vehicle/Fact/QML snippets.
