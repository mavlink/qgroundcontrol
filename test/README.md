# QGroundControl Testing Guide

This is the canonical guide to QGroundControl's test framework, test-writing conventions, CTest
labels, fixtures, and debugging. Use [tools/README.md](../tools/README.md#quality) for the `just`
recipe reference, [.github/README.md](../.github/README.md#tests) for CI invocation, and
[AGENTS.md](../AGENTS.md) for the agent workflow and definition of done.

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
  - [Expected Log Messages](#expected-log-messages)
- [Running Tests](#running-tests)
  - [Via CTest](#via-ctest)
  - [Via `just`](#via-just)
  - [Via the QGroundControl Binary](#via-the-qgroundcontrol-binary-unittest-build)
  - [Convenience Targets](#convenience-targets)
  - [Category-Specific Targets](#category-specific-targets)
  - [QML Test Split](#qml-test-split)
  - [JUnit XML Output](#junit-xml-output)
- [MultiSignalSpy](#multisignalspy)
- [Code Coverage](#code-coverage)
- [Sanitizers](#sanitizers)
- [Fuzzing](#fuzzing)
- [Debugging Test Failures](#debugging-test-failures)

## Quick Start

```bash
# Configure and build a Debug tree with tests enabled
just configure
just build

# Run the repository's default CI-safe test selection
just test

# Iterate on one test
ctest --test-dir build --output-on-failure -R '^ParameterManagerTest$'
```

## Test Framework Overview

Tests are Qt `QObject`-based classes that subclass one of the framework's base classes (below) and
self-register via `UT_REGISTER_TEST`. Each test is also registered with CTest in its area's
`CMakeLists.txt`. There is no central C++ list to update: `main.cc`, `QGCApplication`,
`QGCCommandLineParser`, and `UnitTestList` enumerate tests at runtime via
`UnitTest::registeredTests()`.

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

```cmake
# Basic unit test
add_qgc_test(MyTest LABELS Unit)

# Integration test that shares the MockLink resource
add_qgc_test(MyIntegrationTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)

# Test with custom timeout
add_qgc_test(MySlowTest LABELS Slow TIMEOUT 300)

# Test that must run alone
add_qgc_test(MyExclusiveTest LABELS Integration SERIAL)

# Test with a named shared resource
add_qgc_test(MySettingsTest LABELS Unit RESOURCE_LOCK Settings)
```

Keep behavioral categories in `UT_REGISTER_TEST(..., TestLabel::...)` aligned with the `LABELS`
passed to `add_qgc_test()`. The C++ labels drive `QGroundControl --label`; the CMake labels drive
CTest filtering and timeout selection. Concurrency is separate: `TestLabel::Serial` is runtime
metadata, while the `SERIAL` CMake argument sets CTest's `RUN_SERIAL` property. Labels do not imply
a resource lock—declare `RESOURCE_LOCK` or `SERIAL` explicitly when a test cannot run concurrently.

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

Sanitizer builds (`QGC_ENABLE_ASAN`/`UBSAN`/`TSAN`/`MSAN`) scale these up (180/360/540/270s)
to absorb instrumentation overhead. `TIMEOUT <seconds>` on `add_qgc_test()` always wins.

**Category labels** (filtering only, no timeout of their own):

| Label            | Meaning                                                                                   |
| ---------------- | ----------------------------------------------------------------------------------------- |
| `Vehicle`        | Requires a MockLink-connected Vehicle                                                     |
| `MissionManager` | Mission planning tests                                                                    |
| `Comms`          | Communication/link tests                                                                  |
| `Utilities`      | Utility class tests                                                                       |
| `Network`        | Requires network access — excluded from CI (`check-ci`, `just test`)                      |
| `Flaky`          | Reserved for intermittently-failing tests, excluded from CI; no test currently carries it |
| `Joystick`       | Joystick/controller tests                                                                 |
| `AnalyzeView`    | Log analysis and geo-tagging tests                                                        |
| `Terrain`        | Terrain query and tile tests                                                              |

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

Prefer the QGC signal wrappers over raw `QSignalSpy::wait()`—they emit readable failure messages
that include the signal name and elapsed time:

```cpp
QVERIFY_SIGNAL_WAIT(spy, TestTimeout::shortMs());          // spy.count() >= 1
QVERIFY_SIGNAL_COUNT_WAIT(spy, 3, TestTimeout::shortMs()); // spy.count() >= 3
QVERIFY_NO_SIGNAL_WAIT(spy, 250);                          // see gotcha below
```

For general conditions, prefer Qt's native `QTRY_VERIFY_WITH_TIMEOUT` and
`QTRY_COMPARE_WITH_TIMEOUT`. `QVERIFY_TRUE_WAIT` and `QCOMPARE_TRUE_WAIT` remain compatibility
wrappers for existing tests.

**Gotcha — `QVERIFY_NO_SIGNAL_WAIT` always burns the full timeout.** It has to, because
proving absence requires waiting the entire window. Keep its timeout as tight as
possible (a few hundred ms), and put it *after* any `QTRY_*_WITH_TIMEOUT` that already
proves state has settled.

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

### Expected Log Messages

Every test runs with strict log checking: an unexpected message fails the test. If the behavior
under test intentionally emits a warning, use the QGC base-class helpers instead of
`QTest::ignoreMessage` (which the `check-no-qtest-ignore-message` pre-commit hook rejects):

```cpp
// Preferred: assert that the expected message is emitted by this operation.
expectLogMessage("MAVLink.LibEvents.HealthAndArmingCheckReport",
                 QtWarningMsg,
                 QRegularExpression(QStringLiteral("Flight mode group not set")));
somethingThatLogs();
verifyExpectedLogMessage();

// Last resort: suppress environment-dependent noise that has no precise call site.
ignoreLogMessage("qt.bluetooth",
                 QtWarningMsg,
                 QRegularExpression(QStringLiteral("Cannot find a compatible running Bluez")));
```

Always pair `expectLogMessage()` with `verifyExpectedLogMessage()`. Use `ignoreLogMessage()` only
for nondeterministic environment noise; it suppresses a message without proving that the expected
behavior occurred.

## Running Tests

### Via CTest

Run CTest from the configured build tree, or pass it explicitly with `--test-dir build`:

```bash
ctest --test-dir build --parallel 4                  # All tests, including opt-in categories
ctest --test-dir build -L Unit                       # Unit tests only
ctest --test-dir build --output-on-failure -L Unit   # Unit tests with failure output
ctest --test-dir build -L Integration                # Integration tests
ctest --test-dir build -LE "Flaky|Network"           # Exclude flaky and network tests
ctest --test-dir build -R "Camera|Mission"           # Tests matching a name regex
ctest --test-dir build -R '^MyTest$'                 # Run one test by exact name
ctest --test-dir build --rerun-failed                # Re-run only failed tests
```

### Via `just`

The [`just test` recipe](../tools/README.md#quality) owns the repository's default label and
exclusion filters:

```bash
just test                              # Unit|Integration, excluding Flaky|Network
LABELS=Unit just test                  # Unit tests only
EXCLUDE='Flaky|Network|Slow' just test # Add an exclusion
just test Slow Network                 # Positional overrides: labels, then exclusions
```

Use direct CTest commands for focused iteration; use `just test` for the repository default.

### Via the QGroundControl Binary (unittest build)

The default Debug build places the executable under `build/Debug/`. Run these commands from that
directory (append `.exe` on Windows):

```bash
cd build/Debug
./QGroundControl --list-tests                          # List all registered tests
./QGroundControl --list-tests --label=Unit,Utilities   # List tests matching labels
./QGroundControl --unittest:MyTest                     # Run one test suite
./QGroundControl --unittest                            # Run all tests
./QGroundControl --unittest --label=Unit               # Run by label filter (comma-sep)
./QGroundControl --unittest:MyTest --unittest-output:results.xml  # JUnit XML
./QGroundControl --unittest:MyTest --unittest-stress:50           # Stress: 50 iterations
```

### Convenience Targets

```bash
cmake --build build --target check              # All tests
cmake --build build --target check-unit         # Unit tests only
cmake --build build --target check-integration  # Integration tests
cmake --build build --target check-fast         # Exclude slow tests
cmake --build build --target check-ci           # CI-safe (excludes Flaky, Network)
cmake --build build --target check-flaky        # Repeat until-fail (excludes Network)
```

### Category-Specific Targets

```bash
cmake --build build --target check-missionmanager
cmake --build build --target check-vehicle
cmake --build build --target check-utilities
cmake --build build --target check-mavlink
cmake --build build --target check-comms
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
ctest --test-dir build -R '^QmlQuickTests$' --output-on-failure

# In-process sanity check runner
ctest --test-dir build -R '^QmlTestFileValidator$' --output-on-failure
```

### JUnit XML Output

```bash
# Single test with XML output
build/Debug/QGroundControl --unittest:MyTest --unittest-output:results.xml

# All tests via CTest with JUnit output
ctest --test-dir build --output-junit results.xml
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

## Code Coverage

```bash
just coverage   # Build with coverage, run tests, generate HTML + XML report
```

See [tools/README.md](../tools/README.md#coveragepy) for the `coverage.py` wrapper flags and the
underlying CMake `coverage*` targets (report lands in `build-coverage/`).

## Sanitizers

```bash
# AddressSanitizer (memory errors)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_ENABLE_ASAN=ON

# UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_ENABLE_UBSAN=ON

# Combined ASan + UBSan (recommended default sanitizer lane)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_ENABLE_ASAN=ON -DQGC_ENABLE_UBSAN=ON

# ThreadSanitizer (data races, run separately)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_ENABLE_TSAN=ON
```

## Fuzzing

ClusterFuzzLite runs the MAVLink byte-stream parser under AddressSanitizer when a pull request
changes the fuzzer, its build integration, or QGC's pinned MAVLink configuration. A matching
`master` push stores the comparison build. Targets live in `test/Fuzz/`; the container and build
contract live in `.clusterfuzzlite/`. Keep each target deterministic and independent between
inputs, and provide a valid seed corpus plus a protocol dictionary when the input format includes
checksums or framing bytes.

## Debugging Test Failures

### Enable Verbose Output

```bash
QGC_TEST_VERBOSE=1 build/Debug/QGroundControl --unittest:MyTest
```

### Run Single Test Method

```bash
build/Debug/QGroundControl --unittest:MyTest -- -v2 _specificMethod
```

### Common Issues

1. **Test timeout**: Check for blocking operations, increase timeout with `TIMEOUT`
2. **Resource conflicts**: Use `RESOURCE_LOCK` or `SERIAL` for exclusive access
3. **Flaky signals**: Use `MultiSignalSpy::waitForSignal()` instead of `QSignalSpy::wait()`
4. **Stale singletons**: Ensure proper cleanup in `cleanup()` method
5. **Unexpected log messages**: Follow [Expected Log Messages](#expected-log-messages); do not use
   `QTest::ignoreMessage`.
6. **Expensive SKIP paths**: If a test conditionally `QSKIP`s based on a backend probe
   (e.g. "SDL didn't expose a second virtual joystick"), cache the probe result in a
   test-class member so subsequent skipping tests don't each burn a full timeout.
   See `JoystickManagerTest::_multiJoysticksSupported()` for the pattern.
