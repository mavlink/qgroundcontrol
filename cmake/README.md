# CMake Development Guide

This directory contains QGroundControl's reusable build modules, platform configuration, package
assembly, dependency finders, and pure-CMake regression tests. The root `CMakeLists.txt` remains the
entry point for normal builds.

## Layout

| Path | Purpose |
| --- | --- |
| `*.cmake` | Build options (`CustomOptions.cmake`), shared helpers (`Helpers.cmake`), and test registration (`QGCTest.cmake`) |
| `*.in` | Templates configured into the build directory: `CTestCustom.cmake.in` (Valgrind memcheck) and `ccache-launcher.cmd.in` (Windows ccache launcher) |
| `GStreamer/` | GStreamer discovery, download, validation, installation, and pure-CMake tests |
| `install/` | Runtime deployment and package assembly |
| `modules/` | Reusable project modules and dependency helpers |
| `platform/` | Android, Apple, Linux, and Windows configuration |
| `presets/` | Platform configure, build, test, package, and workflow presets |
| `tests/` | Configure-time contracts for shared CMake helpers |

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
- The lint hooks run cmakelang 0.6.13, which predates `return(PROPAGATE)` and `file(ARCHIVE_EXTRACT)`. Silence its
  findings on those with a `# cmake-lint: disable=` line in the function; no other comment is needed.

`cmake/modules/CPM.cmake` is vendored. Do not modify or reformat it as part of project CMake
maintenance; the pre-commit CMake hooks intentionally exclude it.

The standard application's autogen target depends explicitly on `qgc-analysis-headers` instead
of waiting for all linked libraries. Add new build-time header generators to that prerequisite
target when moc can consume them. Custom builds retain forwarded dependencies because their
additional generators are not known to the standard build.

`QGC_USE_MOCCACHE` (default `ON`) routes AUTOMOC through a generated `<build>/moccache-launcher`, which runs
`tools/moccache_launcher.py` to import [`tools/moccache/`](../tools/README.md#moccache) with cached bytecode.
It needs Python 3.10 or newer (`QGC_MOCCACHE_PYTHON`) and, on Windows, CMake 3.29; otherwise configure falls back
to plain moc. Configure rewrites the launcher only when its content changes, because AUTOMOC re-runs moc for every
target whose launcher is newer than its output.

`QGC_USE_LLD_LINK` (default `OFF`) links MSVC builds with `lld-link` through `CMAKE_LINKER_TYPE` (CMake 3.29+). It
applies only without IPO (single-config Debug or RelWithDebInfo): lld-link cannot link the `/GL` objects of Release
and multi-config builds, which keep `link.exe`. CMake must find `lld-link` when it first detects the compiler (next to
`cl.exe` or on `PATH`: install LLVM, or add `VC\Tools\Llvm\x64\bin` from Visual Studio's "C++ Clang tools"), or take
it from `-DCMAKE_LINKER_LLD=<path>` on a fresh configure; otherwise configure keeps `link.exe`.

Run the CMake lint hooks on the files you change:

```bash
prek run cmake-format --files path/to/file.cmake
prek run cmake-lint --files path/to/file.cmake
```

The manual-stage `gersemi` hook trials [gersemi](https://github.com/BlankSpruce/gersemi) as a cmakelang
replacement: `prek run gersemi --hook-stage manual --files path/to/file.cmake` prints the diff it would apply
(configured by [`tools/configs/gersemirc.yaml`](../tools/configs/gersemirc.yaml)) without rewriting the file.
cmake-format and cmake-lint remain the enforced checks.

## Tests

After configuring a build with tests enabled, run the shared-module (`tests/`) and GStreamer (`GStreamer/tests/`,
also labelled `GStreamer`) script and fixture tests with:

```bash
ctest --test-dir build --output-on-failure -L CMake
```

Use `just configure`, `just build`, and the relevant CTest labels for the final validation described
in the root [AGENTS.md](../AGENTS.md).

`QGCTest.cmake` provides `add_qgc_test()` (see [test/README.md](../test/README.md#register-in-cmakeliststxt)), the
`check*` convenience targets, and these cache options:

| Option | Default | Purpose |
| --- | --- | --- |
| `QGC_TEST_TIMEOUT_UNIT`, `_INTEGRATION`, `_SLOW`, `_DEFAULT` | 60, 120, 180, 90 | Per-label CTest timeouts in seconds; sanitizer builds raise them |
| `QGC_TEST_PARALLEL_LEVEL` | Logical cores | Parallelism of the `check*` targets |
| `QGC_TEST_FLAKY_REPEAT` | 3 | `check-flaky` repeat count |
| `QGC_TEST_ONSCREEN` | `OFF` | Run tests on the native display instead of offscreen |
| `QGC_TEST_DETECT_LEAKS` | `OFF` | Let ASan run LSan leak detection, which needs `ptrace` permission |

Sanitizer builds set each enabled sanitizer's `*SAN_OPTIONS` (with `halt_on_error=1`) on every CTest test, so
findings fail the test; the values replace any set in the calling environment.

## GStreamer

`GStreamer/` discovers or downloads the GStreamer SDK and creates the `GStreamer::` targets. The GStreamer
[README](../src/VideoManager/VideoReceiver/GStreamer/README.md#build-configuration) describes its modules and configure
options.

Each HwBuffers zero-copy GPU path can be compiled out with `QGC_GST_ENABLE_<PATH>=OFF` (DMABUF, CUDA, GLMEMORY,
IOSURFACE, D3D11, D3D12, AHARDWAREBUFFER, VULKAN; all ON by default). Configure prints which compiled paths the sink
can't reach at runtime.

## Presets

The root [`CMakePresets.json`](../CMakePresets.json) aggregates the platform definitions in
`presets/`. CI selects one of those configure presets and limits workflow overrides to dynamic
values such as signing credentials, sanitizer selection, and target-specific paths. Keep build
type, generator, testing, coverage, toolchain, and cache defaults in the presets so local and CI
configuration cannot drift.

The root file owns `cmakeMinimumRequired`; included fragments declare only their preset schema
`version`. Every file uses version 8, the newest that the minimum CMake (3.28) reads, and names the
official CMake presets JSON schema in `$schema` (version 8 or later) for editor validation and
completion. Preset build directories stay under `${sourceDir}/build` so separate source
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

`Linux-clang-debug` is `Linux-debug` built with Clang, which loads precompiled headers far faster than
GCC: a cold build on 24 cores takes about 75 s instead of 205 s. Clang emits a class's debug info only
where its key function is compiled, so types from libraries without debug symbols (such as Qt without
its debug files) can show as incomplete in the debugger; add `-fstandalone-debug` to
`CMAKE_CXX_FLAGS_DEBUG` if that gets in the way.

The hidden `lean-debug` preset uses line-table debug info (`-g1`), which is all gcov and sanitizer stack traces
need, and unity builds of QGC targets (`QGC_UNITY_BUILD=ON`). CI builds with these presets, which inherit it:

- `Linux-coverage` — Debug with tests and `QGC_ENABLE_COVERAGE=ON`, in `build/Linux-coverage`.
- `Linux-sanitizers` — Debug with tests, ASan + UBSan and `QGC_TEST_DETECT_LEAKS=ON`, in `build/Linux-sanitizers`;
  its test preset also excludes `NoSanitizer`. `cmake --workflow --preset Linux-sanitizers` reproduces CI's
  sanitizer job.
- `iOS-simulator` — x86_64 simulator Debug build without tests.

The `just configure` and `just build` recipes use the matching `default*` configure and build
presets for `BUILD_TYPE`. `just test` always uses the `default` test preset and passes `BUILD_TYPE`
through `--build-config`. `tools/configure.py` selects a configure preset by build type and accepts
an explicit `--preset`, configuring into the preset's `binaryDir` unless `-B` is given. Coverage uses
`Linux-coverage`. Docker uses the platform presets with mounted build-directory overrides;
unsupported build types and its dynamic ARM64 sysroot keep explicit `qt-cmake` configuration.
Multipass and Vagrant both run `deploy/multipass/build_in_vm.py`, which passes the matching `Linux*` preset to
`qt-cmake` with a build-directory override (MinSizeRel has no preset).

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
