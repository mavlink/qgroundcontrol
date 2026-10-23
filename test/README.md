# QGroundControl Unit Testing

> Agent guidance (naming, review process, architecture patterns) lives in [AGENTS.md](../AGENTS.md);
> this doc covers the test framework itself.

## Table of Contents

- [Quick Start](#quick-start)
- [Test Framework Overview](#test-framework-overview)
  - [Base Test Classes](#base-test-classes)
- [Writing Tests](#writing-tests)
  - [Test Quality](#test-quality)
  - [Basic Test Class](#basic-test-class)
  - [Data-Driven Tests](#data-driven-tests-default-for-permutations)
  - [Register in CMakeLists.txt](#register-in-cmakeliststxt)
  - [CTest Labels & Timeouts](#ctest-labels--timeouts)
  - [Wait/Timeout Helpers](#waittimeout-helpers)
- [Running Tests](#running-tests)
  - [Via CTest](#via-ctest)
  - [Via `just`](#via-just)
  - [Via the QGroundControl Binary](#via-the-qgroundcontrol-binary-unittest-build)
  - [Convenience Targets](#convenience-targets)
  - [QML Test Split](#qml-test-split)
- [Portable Utility Tests](#portable-utility-tests)
- [GPS Protocol Golden Transcripts](#gps-protocol-golden-transcripts)
- [MultiSignalSpy](#multisignalspy)
- [Manual Test Plans](#manual-test-plans)
- [Code Coverage](#code-coverage)
- [Sanitizers](#sanitizers)
- [Debugging Test Failures](#debugging-test-failures)

## Quick Start

Debug builds enable tests (`QGC_BUILD_TESTING`) by default.

```bash
just configure && just build                               # Debug build in build/
just test                                                  # Unit|Integration, excluding Flaky|Network
ctest --test-dir build -R ParameterManagerTest             # One test (all of its shards)
build/Debug/QGroundControl --unittest:ParameterManagerTest # One suite, directly (Linux path)
```

## Test Framework Overview

Tests are Qt `QObject`-based classes that subclass one of the framework's base classes
(below) and self-register via `UT_REGISTER_TEST`. There is no central list of tests to
update — `main.cc`, `QGCApplication`, `QGCCommandLineParser`, and `UnitTestList` are all
generic and enumerate tests at runtime via `UnitTest::registeredTests()`.

### Base Test Classes

Pick the narrowest base class that gives you the fixtures you need — each one builds on
the last, so you get the parent's setup/teardown for free.

| Class                         | Extends       | Use For                                              |
| ----------------------------- | ------------- | ---------------------------------------------------- |
| `UnitTest`                    | —             | Basic tests, no special setup                        |
| `JsonTest`                    | `UnitTest`    | Tests parsing/validating JSON documents              |
| `StateMachineTest`            | `UnitTest`    | Tests driving a `QStateMachine`-based controller     |
| `CommsTest`                   | `UnitTest`    | Tests requiring LinkManager/MockLink                 |
| `TerrainTest`                 | `UnitTest`    | Tests requiring TerrainQuery                         |
| `VehicleTest`                 | `UnitTest`    | Tests requiring a connected Vehicle                  |
| `VehicleTestManualConnect`    | `VehicleTest` | Vehicle tests that connect explicitly mid-test       |
| `VehicleTestNoInitialConnect` | `VehicleTest` | Vehicle tests that skip the default auto-connect     |
| `VehicleTestAPM`              | `VehicleTest` | Vehicle tests against an ArduPilot-flavored MockLink |
| `ParameterTest`               | `VehicleTest` | Tests involving ParameterManager                     |
| `MissionTest`                 | `VehicleTest` | Tests involving MissionController on a live Vehicle  |
| `OfflineMissionTest`          | `UnitTest`    | Mission tests that don't need a connected Vehicle    |

## Writing Tests

### Test Quality

Cover as much behavior as possible with as little code as possible — a few strong tests beat many
redundant ones. More tests ≠ better coverage.

- **One behavior per test, one reason to fail.** Each test asserts a single contract or edge case. If
  two tests fail for the same cause, merge them.
- **Consolidate permutations.** Fold near-identical cases that vary only in input into one
  [data-driven test](#data-driven-tests-default-for-permutations), not copy-pasted methods.
- **Test behavior, not implementation.** Assert observable outputs and effects, not private state or
  call order — the latter break on every refactor without catching real regressions.
- **Prioritize edge cases and failure paths** (boundaries, empty/null, error handling) over repeated
  happy-path variations. Don't test Qt or the standard library.
- **A test that can't fail is noise.** No tautologies, no asserting the mock. If you can't name the
  regression it catches, don't add it — and before extending a suite, read it and consolidate into the
  nearest existing test rather than appending a redundant one.

### Basic Test Class

```cpp
#include "UnitTest.h"

class MyTest : public UnitTest {
    Q_OBJECT
private slots:
    void initTestCase();    // One-time setup (optional)
    void cleanupTestCase(); // One-time teardown (optional)
    void init();            // Per-test setup (optional)
    void cleanup();         // Per-test teardown (optional)

    void _myTestCase();     // Test method (prefix with _)
};

UT_REGISTER_TEST(MyTest, TestLabel::Unit)
```

Available `TestLabel::*` enum values: `Unit`, `Integration`, `Vehicle`, `MissionManager`,
`Comms`, `Utilities`, `Slow`, `Network`, `Serial`, `Joystick`, `AnalyzeView`, `Terrain`.
CTest `LABELS` set in `CMakeLists.txt` are a superset — some tests carry extra
CTest-only names (e.g. `MAVLink`, `Camera`) that have no corresponding `TestLabel` enum
value; the two lists are independent.

### Data-driven tests (default for permutations)

For tests that repeat the same assertions across a set of literal inputs, use Qt's
table-driven pattern (`_data()` + `QTest::addColumn`/`QFETCH`) rather than copy-pasting
near-identical method bodies. Each row gets its own failure tag, so a single bad
permutation is identified without re-running the whole method. See
`Utilities/Compression/QGCCompressionTest.cc` (`_testFormatDetection`,
`_testDetectFormatFromMagicBytes`) for the canonical shape.

### Register in CMakeLists.txt

Add test sources to the `QGCUnitTests` object library. The application executable links it and runs
the suite; tests compile with the application's include paths and definitions, and their precompiled
header adds `<QtTest/QTest>` so application sources do not compile QtTest.

```cmake
target_sources(
    QGCUnitTests
    PRIVATE
        MyTest.cc
        MyTest.h
)

# Basic unit test
add_qgc_test(MyTest LABELS Unit)

# Integration test
add_qgc_test(MyIntegrationTest LABELS Integration Vehicle)

# Test with custom timeout
add_qgc_test(MySlowTest LABELS Slow TIMEOUT 300)

# Timing-sensitive test: runs alone (RUN_SERIAL) in sanitizer/coverage builds, otherwise reserves 2 CTest processors
add_qgc_test(MyExclusiveTest LABELS Integration SERIAL)

# Tests sharing a fixed resource never run concurrently (SERIAL and RESOURCE_LOCK are mutually exclusive)
add_qgc_test(MyTest LABELS Unit RESOURCE_LOCK Joystick)

# Long test class split across CTest entries MyUITest.1 .. MyUITest.3 that run in parallel
add_qgc_test(MyUITest LABELS Integration SHARDS 3)
```

Every test gets its own `TMPDIR` (and `TMP`/`TEMP` on Windows) under `build/test-tmp/`, so
`RESOURCE_LOCK TempFiles` is ignored. `ENV_MODIFICATION` and `SKIP_REGEX` set CTest's
`ENVIRONMENT_MODIFICATION` and `SKIP_REGULAR_EXPRESSION`.

`SHARDS <count>` gives each entry every count-th test function in declaration order
(`--unittest-shard:<index>/<count>`), so new functions are picked up automatically. Data rows of
one function stay in the same shard; a shard that selects no functions fails, so lower `SHARDS`
when a class loses functions.

### CTest Labels & Timeouts

Only three labels set the CTest timeout directly; everything else is a filterable
category that combines with one of them (or falls back to the 90s default).

**Timeout tiers** (`cmake/QGCTest.cmake`, non-sanitizer build):

| Label                 | Timeout | Applies When                                              |
| --------------------- | ------- | --------------------------------------------------------- |
| `Unit`                | 60s     | `LABELS` includes `Unit` and not `Slow`                   |
| `Integration`         | 120s    | `LABELS` includes `Integration` and not `Slow`            |
| `Slow`                | 180s    | `LABELS` includes `Slow` (overrides `Unit`/`Integration`) |
| *(none of the above)* | 90s     | Default fallback (`QGC_TEST_TIMEOUT_DEFAULT`)             |

Sanitizer builds (`QGC_ENABLE_ASAN`/`UBSAN`/`TSAN`/`MSAN`) raise these to at least 180/360/540/270s
(`test/CMakeLists.txt`) to absorb instrumentation overhead. `TIMEOUT <seconds>` on `add_qgc_test()`
always wins; `${QGC_TEST_TIMEOUT_EXTENDED}` (180s, 360s with sanitizers) is the usual value for long tests.

**Category labels** (filtering only, no timeout of their own):

| Label            | Meaning                                                                                   |
| ---------------- | ----------------------------------------------------------------------------------------- |
| `Vehicle`        | Requires a MockLink-connected Vehicle                                                     |
| `MissionManager` | Mission planning tests                                                                    |
| `Comms`          | Communication/link tests                                                                  |
| `Utilities`      | Utility class tests                                                                       |
| `Network`        | Needs network; excluded from PR suites, covered by manual Linux coverage runs             |
| `Flaky`          | Intermittent failures; excluded from PR suites, covered by manual Linux coverage runs     |
| `Serial`         | C++ `TestLabel::Serial` tag, no CTest effect; `add_qgc_test(... SERIAL)` doesn't add it   |
| `Joystick`       | Joystick/controller tests                                                                 |
| `AnalyzeView`    | Log analysis and geo-tagging tests                                                        |
| `Terrain`        | Terrain query and tile tests                                                              |
| `GPS`            | GPS receiver, corrections and NTRIP tests; every test under `test/GPS` carries it         |
| `Fuzz`           | Fuzzer smoke runs: fixed inputs through a fuzz harness, without libFuzzer                 |
| `QML`            | QML test runners and QML lint checks                                                      |
| `NoSanitizer`    | Incompatible with ASan/UBSan; excluded by sanitizer CI and the `Linux-sanitizers` preset  |
| `StockUI`        | Asserts stock UI that custom builds replace; excluded from Custom Build integration tests |
| `Portable`       | [Portable utility tests](#portable-utility-tests)                                         |
| `CMake`          | [CMake script tests](../cmake/README.md#tests); the GStreamer ones also carry `GStreamer` |

Other CTest-only labels (`MAVLink`, `Camera`, `Viewer3D`) only name an area.

### Wait/Timeout Helpers

#### Prefer `TestTimeout::*` over literal millisecond values

All `.wait()`, `QTRY_*_WITH_TIMEOUT`, and `QVERIFY_*_WAIT` calls should use the
CI-adaptive helpers rather than hard-coded numbers. The helpers auto-scale (roughly 2×)
on CI so the same test works on a loaded build agent without slowing local runs:

```cpp
#include "UnitTest.h"  // provides TestTimeout namespace

TestTimeout::shortMs()   // 1s local / 2s CI   — quick signals, in-memory ops
TestTimeout::mediumMs()  // 5s local / 10s CI  — normal async work, file I/O
TestTimeout::longMs()    // 30s local / 60s CI — vehicle connect, FTP, metadata
```

Chrono-typed variants are also available: `TestTimeout::shortDuration()`,
`mediumDuration()`, `longDuration()`.

#### Wait macros

Prefer the QGC wrapper macros over raw `QSignalSpy::wait()`/`QTest::qWaitFor()` — they
emit readable failure messages that include the signal/condition name and the elapsed
time:

```cpp
QVERIFY_SIGNAL_WAIT(spy, TestTimeout::shortMs());          // a new emission (see gotcha below)
QVERIFY_SIGNAL_COUNT_WAIT(spy, 3, TestTimeout::shortMs()); // spy.count() >= 3
QVERIFY_NO_SIGNAL_WAIT(spy, 250);                          // see gotcha below
QVERIFY_TRUE_WAIT(condition, TestTimeout::shortMs());      // QTRY_VERIFY wrapper
QCOMPARE_TRUE_WAIT(actual, expected, TestTimeout::shortMs());
```

**Gotcha — `QVERIFY_SIGNAL_WAIT` waits for a new emission.** It calls `QSignalSpy::wait()`,
so a signal already in the spy doesn't count. Use `QVERIFY_SIGNAL_COUNT_WAIT(spy, n, …)`
when the signal may already have fired, e.g. when another thread emits it.

**Gotcha — `QVERIFY_NO_SIGNAL_WAIT` always burns the full timeout.** It has to, because
proving absence requires waiting the entire window. Keep its timeout as tight as
possible (a few hundred ms), and put it *after* any `QVERIFY_TRUE_WAIT` that already
proves state has settled. Prefer a positive barrier: send a trailing valid input and expect
exactly one signal, or wait for the terminal signal and then check the spy is empty. When the
code under test handles a reply synchronously, check the spy right after the call. In UI tests,
use `findVisibleItem(root, name, 0)` for negative checks after a positive barrier and inside
`QTRY_*`.

**Gotcha — waits in teardown after a failure.** Once the current test function has failed,
`QTestEventLoop` returns immediately, so `QSignalSpy::wait()`, `QVERIFY_SIGNAL_WAIT` and
`QVERIFY_SIGNAL_COUNT_WAIT` don't wait. Teardown helpers poll with
`QTest::qWaitFor(condition, timeout)` and settle before asserting.

**Gotcha — MockLink telemetry starts after the MAVLink v2 upgrade.** MockLink sends MAVLink v1
until QGC's first v2 message, and QGC drops v1 messages other than HEARTBEAT (and RADIO_STATUS).
MockLink therefore sends its first 1 Hz batch (SYS_STATUS, battery, HOME_POSITION, …) right after
that upgrade, then once a second. Tests that need two batches still wait about a second.

#### Condition polling

```cpp
QVERIFY(UnitTest::waitForCondition([&]() { return thing.ready(); },
                                   TestTimeout::shortMs(),
                                   QStringLiteral("thing.ready()")));
```

#### Avoid

- `QTest::qWait(N)` — unconditional sleep, doesn't early-exit. The `check-no-fixed-qwait`
  pre-commit hook rejects any non-zero literal passed to `qWait()` in `test/`.
- Polling `while (!done) { QTest::qWait(20); }` — use `QTRY_VERIFY_WITH_TIMEOUT` or
  `waitForCondition` instead.
- Literal milliseconds in timeouts. If you *must* use a literal (e.g. tight
  `QVERIFY_NO_SIGNAL_WAIT`), add a comment explaining why `TestTimeout::*` isn't
  appropriate.

## Running Tests

### Via CTest

```bash
ctest --test-dir build --parallel $(nproc)          # All tests in parallel
ctest --test-dir build --output-on-failure -L Unit  # Unit tests, print output on failure
ctest --test-dir build -L Integration               # Integration tests
ctest --test-dir build -LE "Flaky|Network"          # Exclude flaky and network tests
ctest --test-dir build -R "Camera|Mission"          # Tests matching a name regex
ctest --test-dir build --rerun-failed               # Re-run only failed tests
ctest --test-dir build --output-junit results.xml   # JUnit XML report
```

### Via `just`

```bash
just test                                    # Labels "Unit|Integration", excludes "Flaky|Network"
LABELS=Unit just test                        # Override labels
EXCLUDE=Network just test                    # Override exclusions
LABELS=Slow EXCLUDE=Flaky just test          # Both at once
JOBS=4 just test                             # Override parallelism (default: all cores)
```

`just test` runs `ctest --preset default --build-config <BUILD_TYPE> --parallel <JOBS> --no-tests=error -L
"<LABELS>" -LE "<EXCLUDE>"`; the test preset (`cmake/presets/common.json`) turns on output on failure. The `default`
test preset tests `build/` only; for a preset with its own build directory, run `ctest --preset <name>` (for example
`Linux-sanitizers`).

CI runs the `Unit` and `Integration` labels as separate CTest runs with the same exclusions. Linux CI uses one
process per CPU (four on its runners), except two for coverage integration tests; to match it, run
`ctest --test-dir build -L Integration -LE 'Flaky|Network' --output-on-failure --parallel 4`. Settings and
temporary directories are isolated per test, and local network fixtures allocate ephemeral ports. CTest
`RESOURCE_LOCK` still serializes tests sharing hardware or fixed resources. `SERIAL` tests get `RUN_SERIAL` only in
sanitizer (`QGC_ENABLE_ASAN`/`UBSAN`/`TSAN`/`MSAN`) and coverage builds; other builds run them alongside other
tests with `PROCESSORS 2`. Preserve this isolation when adding integration tests.

### Via the QGroundControl Binary (unittest build)

The binary is `build/Debug/QGroundControl` on Linux (`QGroundControl.app/Contents/MacOS/QGroundControl` on macOS,
`QGroundControl.exe` on Windows).

```bash
./QGroundControl --list-tests                          # List all registered tests
./QGroundControl --list-tests --label=Unit,Utilities   # List tests matching labels
./QGroundControl --unittest:MyTest                     # Run one test suite
./QGroundControl --unittest                            # Run all tests
./QGroundControl --unittest --label=Unit               # Run by label filter (comma-sep)
./QGroundControl --unittest:MyTest --unittest-output:results.xml  # JUnit XML in results-MyTest.xml
./QGroundControl --unittest:MyTest --unittest-stress:50           # Stress: 50 iterations
./QGroundControl --unittest:MyTest --unittest-shard:2/3           # Every 3rd test function, starting at the 2nd
                                                                  # (results-MyTest.2.xml with --unittest-output)
```

`--unittest:<Class>` runs every function of the class; there is no single-function selector.

### Convenience Targets

Each target builds the application first (`cmake --build build --target <target>`):

```text
check                # All tests
check-unit           # Unit tests only
check-integration    # Integration tests, one at a time
check-fast           # Exclude Slow
check-ci             # Exclude Flaky and Network
check-flaky          # Repeat until-fail (QGC_TEST_FLAKY_REPEAT, default 3) to surface flaky failures
check-missionmanager # One label: also check-vehicle, check-utilities, check-mavlink, check-comms
```

### QML Test Split

There are two separate QML-related test entry points with different purposes:

- `QmlQuickTests` (recommended for real QML testing): Runs `tst_*.qml` via a dedicated
  Qt Quick Test executable (`QGCQmlQuickTests`) out-of-process.
- `QmlTestFileValidator` (sanity/structure check): Runs inside the regular `UnitTest`
  harness and validates test discovery/setup, but is not the primary Qt Quick execution
  path.

```bash
# Real QML quick tests
ctest --test-dir build -R QmlQuickTests --output-on-failure

# In-process sanity check runner
ctest --test-dir build -R QmlTestFileValidator --output-on-failure
```

## Portable Utility Tests

Windows x64 and the macOS Release CI leg enable `QGC_BUILD_PORTABLE_TESTS` alongside the normal
application build (the macOS Debug leg runs the Unit and Integration suites instead, same as
Linux). The ten `Portable.*` CTest entries use small Qt Test executables, with the same test
bodies and production utility libraries as the Linux application tests. They require no
QGCApplication, vehicles, or QML engine. The portable command-line parser target enables
its own test hooks; the packaged application keeps its normal build configuration.

```bash
cmake -S . -B build -DQGC_BUILD_PORTABLE_TESTS=ON
cmake --build build --target portable-tests --parallel 8
ctest --test-dir build --build-config Debug --output-on-failure -L Portable
```

Use `PortableTest` and `QGC_REGISTER_PORTABLE_TEST` for these suites. Full application
builds use the existing UnitTest harness; standalone executables use Qt Test with warnings
failing tests. Keep application-dependent fixtures in the full harness.

## GPS Protocol Golden Transcripts

`GPSGoldenTranscriptTest` pins the native GPS receiver protocols on a virtual clock. For each
scenario it records host-to-receiver bytes, baud changes, configuration evidence, identity and
decoded events. It also records the events decoded from every file in `test/GPS/Core/Protocols/corpus/`
and `test/GPS/Core/Protocols/fixtures/` at several chunk sizes. The expected transcripts live in
`test/GPS/Core/Protocols/golden/`, and `test/GPS/Core/Protocols/Support/GoldenTranscript.h` documents the driver
seam. The suite also checks that a decoder armed without I/O for a recording replay
(`GPSProtocolRuntime::armDecodeOnly()`) decodes those files as the configured decoder does.
[`test/GPS/Core/Protocols/GPSGoldenTranscriptTest.cc`](GPS/Core/Protocols/GPSGoldenTranscriptTest.cc) describes the
format. A scenario that behaves exactly as another one names it in `sameAs` and has no golden of its own, and a
scenario records its stream phase only when that differs from every other scenario's of its family.

A mismatch prints a diff hunk. Rewrite the goldens only for a deliberate, justified behaviour
change, then review the diff. Update mode also removes files no scenario produces:

```bash
QGC_GPS_GOLDEN_UPDATE=1 ctest --test-dir build -R GPSGoldenTranscriptTest
```

`test/GPS/Standalone` builds `QGCGPSCore` and `QGCNTRIPHttp` without the application, with the receiver
qualification runner and the NTRIP decoder smoke test. With Clang, `-DQGC_GPS_LIBFUZZER=ON` adds the protocol and
NTRIP libFuzzer harnesses and builds both libraries with ASan and UBSan:

```bash
cmake -S test/GPS/Standalone -B build-gps -DCMAKE_CXX_COMPILER=clang++ -DQGC_GPS_LIBFUZZER=ON
cmake --build build-gps && ctest --test-dir build-gps
```

## MultiSignalSpy

```cpp
MultiSignalSpy spy;
spy.init(myObject);

// String-based API (recommended)
QVERIFY(spy.emittedOnce("valueChanged"));           // Emitted exactly once
QVERIFY(spy.onlyEmittedOnce("valueChanged"));       // Only this signal fired
QVERIFY(spy.waitForSignal("valueChanged", TestTimeout::mediumMs())); // Wait with timeout
QVERIFY(spy.notEmitted("errorOccurred"));           // Signal not emitted
spy.clearAllSignals();

// Fluent expectation API
QVERIFY(spy.expect("signal").once());
QVERIFY(spy.expect("signal").atLeastOnce());
QVERIFY(spy.expect("signal").never());
QVERIFY(spy.expect("signal").times(3));

// Argument extraction
int value = spy.argument<int>("valueChanged");

// Multiple-signal API (each signal emitted exactly once)
QVERIFY(spy.emittedOnce("signal1", "signal2"));
```

## Manual Test Plans

`plans/` holds `.plan` files for manually checking 2D/3D mission display. There is one for each
MockLink home location and PX4/ArduPilot vehicle type. Each plan mixes altitude frames
(relative, AMSL, calculated above terrain, and terrain frame on ArduPilot) and item types over
~3 km legs, so the paths cross terrain. The plans exercise the UI and aren't meant to be flown.
Start a MockLink with the matching home location, then load the plan in Plan View and upload it.

`PlanMasterControllerTest::_testManualTestPlansLoad` loads every plan, so they stay loadable as the
plan format changes. Regenerate them after changing the generator:

```bash
test/plans/generate_test_plans.py
```

## Code Coverage

```bash
just coverage   # Build with coverage, run tests, generate HTML + XML report
```

`just coverage` configures `build/Linux-coverage/` with the `Linux-coverage` preset (line tables and unity build, like CI)
and runs CI's two test phases through its test preset (excluding `Flaky|Network`): `Unit` at one process per CPU,
then `Integration` at `-j2`.
See [tools/README.md](../tools/README.md#coverage_reportpy) for the `coverage_report.py` wrapper flags and the
underlying CMake `coverage*` targets.

## Sanitizers

Linux CI runs the `Unit` and `Integration` suites with ASan + UBSan through the `Linux-sanitizers` preset, which
also enables LSan leak detection (`QGC_TEST_DETECT_LEAKS=ON`) and excludes `NoSanitizer` tests. Reproduce it with
`QT_ROOT_DIR` pointing at the Qt installation:

```bash
cmake --workflow --preset Linux-sanitizers   # Configure, build and test in build/Linux-sanitizers
ctest --preset Linux-sanitizers -R MyTest    # Re-run one test after a rebuild
```

ThreadSanitizer can't be combined with ASan; CI runs it only from a manual Analysis workflow dispatch:

```bash
cmake --preset Linux-debug -B build-tsan -DQGC_ENABLE_TSAN=ON
cmake --build build-tsan && ctest --test-dir build-tsan -L "Unit|Integration" -LE "Flaky|Network|NoSanitizer"
```

`cmake/QGCTest.cmake` sets the enabled sanitizers' `*SAN_OPTIONS` (each with `halt_on_error=1`, plus the
suppression files generated in the build directory) on every CTest test, replacing any value in your shell. Run the
test binary directly to try other options. `QGC_TEST_DETECT_LEAKS` is `OFF` outside the
preset because LSan needs `ptrace` permission, which containers and Yama often deny.

## Debugging Test Failures

### Enable Verbose Output

```bash
QGC_TEST_VERBOSE=1 build/Debug/QGroundControl --unittest:MyTest
```

### Common Issues

1. **Test timeout**: Check for blocking operations, increase timeout with `TIMEOUT`
2. **Resource conflicts**: Use `RESOURCE_LOCK` for exclusive access (`SERIAL` runs a test alone
   only in sanitizer and coverage builds)
3. **Flaky signals**: Use `MultiSignalSpy::waitForSignal()` instead of `QSignalSpy::wait()`
4. **Stale singletons**: Ensure proper cleanup in `cleanup()` method
5. **Unexpected log messages (strict mode)**: Every test runs with strict log checking, so
   any log message that is not explicitly expected or suppressed causes `QFAIL`. If the
   code under test emits a warning you didn't anticipate, use the QGC base-class helpers
   instead of `QTest::ignoreMessage` (which is banned by the `check-no-qtest-ignore-message`
   pre-commit hook):

   ```cpp
   // Preferred: one-shot expectation — asserts the message was actually emitted
   expectLogMessage("MAVLink.LibEvents.HealthAndArmingCheckReport",
                    QtWarningMsg,
                    QRegularExpression(QStringLiteral("Flight mode group not set")));
   somethingThatLogs();
   verifyExpectedLogMessage();

   // Last resort only: persistent suppression — does NOT assert the message fires
   ignoreLogMessage("qt.bluetooth",
                    QtWarningMsg,
                    QRegularExpression(QStringLiteral("Cannot find a compatible running Bluez")));
   ```

   `expectLogMessage(...)` must be followed by `verifyExpectedLogMessage()`.
   If verification is omitted, `cleanup()` fails the test.

   **Always prefer `expectLogMessage` + `verifyExpectedLogMessage`.** That
   pairing both silences the strict-mode check *and* asserts the message was
   actually emitted, so a regression that removes the log will break the test.

   Use `ignoreLogMessage(...)` only as a last resort for non-deterministic or
   environment-dependent noise that cannot be tied to a precise call site.

6. **Expensive SKIP paths**: If a test conditionally `QSKIP`s based on a backend probe
   (e.g. "SDL didn't expose a second virtual joystick"), cache the probe result in a
   test-class member so subsequent skipping tests don't each burn a full timeout.
   See `JoystickManagerTest::_multiJoysticksSupported()` for the pattern.

7. **Skipping in `init()`**: a `QSKIP` in `init()` also skips `cleanup()`, and `UnitTest`
   requires `cleanup()` once its `init()` has run. Skip before calling `UnitTest::init()`, or
   skip inside the test function.
