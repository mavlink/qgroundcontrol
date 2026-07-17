# CMake Development Guide

This directory contains QGroundControl's reusable build modules, platform configuration, package
assembly, dependency finders, and pure-CMake regression tests. The root `CMakeLists.txt` remains the
entry point for normal builds.

## Layout

| Path | Purpose |
| --- | --- |
| `GStreamer/` | GStreamer discovery, download, validation, installation, and its pure-CMake tests |
| `install/` | Runtime deployment and package assembly |
| `modules/` | Reusable project modules and dependency helpers |
| `platform/` | Android, Apple, Linux, and Windows configuration |
| `tests/` | Configure-time contracts for shared CMake helpers |
| `Find*.cmake` | CMake find modules for optional dependencies |

## Conventions

- Add `include_guard(GLOBAL)` to reusable modules that define functions, macros, options, or global
  state.
- Prefix project-owned functions, macros, targets, and cache variables with `qgc` or `QGC`.
- Validate required and unknown arguments in public helper functions. Fail during configure rather
  than allowing a misspelled option to be ignored.
- Prefer target-scoped properties and commands over directory-global compile or link flags.
- Account for single- and multi-config generators. Custom test targets should pass their active
  configuration to CTest.
- Keep host and target paths distinct in cross-builds, especially for Android and iOS.
- In GStreamer modules with repeated basenames, include the intended file by absolute path.
- When changing an existing file, format only the edited region. Whole-file formatting is reserved
  for new files or an explicit formatting task.

`cmake/modules/CPM.cmake` is vendored. Avoid modifying or reformatting it as part of project CMake
maintenance.

## Tests

After configuring a build with tests enabled, run the shared-module and finder tests with:

```bash
ctest --test-dir build --output-on-failure -L CMake
ctest --test-dir build --output-on-failure -L FindModule
```

The tests under `GStreamer/tests/` are also standalone scripts and may be run directly with
`cmake -P` while iterating on a single helper. Use `just configure`, `just build`, and the relevant
CTest labels for the final validation described in the root [AGENTS.md](../AGENTS.md).

The root [`CMakePresets.json`](../CMakePresets.json) aggregates the platform definitions in
`presets/`. CI selects one of those configure presets and limits workflow overrides to dynamic
values such as signing credentials, sanitizer selection, and target-specific paths. Keep build
type, generator, testing, coverage, toolchain, and cache defaults in the presets so local and CI
configuration cannot drift.

The root file owns `cmakeMinimumRequired`; included fragments declare only their required preset
schema `version`. Preset build directories stay under `${sourceDir}/build` so separate source
checkouts cannot share or overwrite one another's build trees.

List every configure, build, test, package, and workflow preset with:

```bash
cmake --list-presets=all
```

Each visible configure preset has a same-name build preset and workflow preset. Workflows run tests
only for configurations that set `QGC_BUILD_TESTING=ON`; mobile and ARM64 cross-build debug presets
intentionally keep tests disabled because their target binaries cannot be assumed runnable on the
host. For example:

```bash
cmake --workflow --preset Linux-debug
cmake --workflow --preset Linux-deb
```

The `just configure`, `just build`, and `just test` recipes use the matching `default*` configure,
build, and test presets. `tools/configure.py` selects those presets by build type and accepts an
explicit `--preset`; `--no-preset` is reserved for unsupported custom toolchains. Coverage uses
`Linux-coverage`. Docker and VM builders use platform presets when their Qt/toolchain layout matches
the repository model; the Docker aarch64 sysroot and uncommon build-type fallbacks are marked
`preset-exception` because their toolchains are assembled dynamically at runtime.

| Preset family | Required environment | Notes |
| --- | --- | --- |
| `default*` | A discoverable Qt installation | Generic Ninja build in the source tree |
| `Linux*`, `macOS*`, `Windows*` | `QT_ROOT_DIR` | Qt target installation containing `qt.toolchain.cmake` |
| `Linux-arm64*`, `Windows-arm64*` | `QT_ROOT_DIR`, `QT_HOST_PATH` | Target and host Qt installations for ARM64 or cross-builds |
| `Android*` | `QT_TARGET_ROOT_DIR`, `QT_HOST_PATH`, `ANDROID_NDK`, `ANDROID_MIN_SDK` | Qt for Android plus its matching host Qt and Android toolchain |
| `iOS*` | `QT_ROOT_DIR`, `QT_HOST_PATH` | Qt for iOS plus its matching host Qt |

Package presets exist only for CPack-backed artifacts: Linux `.deb`/`.rpm` packages and Windows
NSIS installers. The normal Linux AppImage and macOS DMG are install-time artifacts, so configure
and build their release workflow and then run `cmake --install <build-dir> --config Release`.
Android APK signing/deployment and iOS IPA assembly remain platform workflow steps rather than CPack
operations.

Use `CMakeUserPresets.json` for machine-local paths or derived presets; it remains ignored by Git
and `tools/clean.py` preserves it.
