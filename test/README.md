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
  - [Category-Specific Targets](#category-specific-targets)
  - [QML Test Split](#qml-test-split)
  - [JUnit XML Output](#junit-xml-output)
- [MultiSignalSpy](#multisignalspy)
- [Code Coverage](#code-coverage)
- [Sanitizers](#sanitizers)
- [Debugging Test Failures](#debugging-test-failures)

## Quick Start

```bash
# Configure and build with tests enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON
cmake --build build

# Run all tests via CTest
cd build && ctest --output-on-failure --parallel $(nproc)

# Run specific test
./QGroundControl --unittest:ParameterManagerTest
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

```cmake
# Basic unit test
add_qgc_test(MyTest LABELS Unit)

# Integration test (auto-locks MockLink resource)
add_qgc_test(MyIntegrationTest LABELS Integration Vehicle)

# Test with custom timeout
add_qgc_test(MySlowTest LABELS Slow TIMEOUT 300)

# Test that must run alone (locks all resources)
add_qgc_test(MyExclusiveTest LABELS Integration SERIAL)

# Test with specific resource lock
add_qgc_test(MyTest LABELS Unit RESOURCE_LOCK TempFiles)
```

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
| `Serial`         | Must run alone (no parallel) — set automatically by `SERIAL`                              |
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

Prefer the QGC wrapper macros over raw `QSignalSpy::wait()`/`QTest::qWaitFor()` — they
emit readable failure messages that include the signal/condition name and the elapsed
time:

```cpp
QVERIFY_SIGNAL_WAIT(spy, TestTimeout::shortMs());          // spy.count() >= 1
QVERIFY_SIGNAL_COUNT_WAIT(spy, 3, TestTimeout::shortMs()); // spy.count() >= 3
QVERIFY_NO_SIGNAL_WAIT(spy, 250);                          // see gotcha below
QVERIFY_TRUE_WAIT(condition, TestTimeout::shortMs());      // QTRY_VERIFY wrapper
QCOMPARE_TRUE_WAIT(actual, expected, TestTimeout::shortMs());
```

**Gotcha — `QVERIFY_NO_SIGNAL_WAIT` always burns the full timeout.** It has to, because
proving absence requires waiting the entire window. Keep its timeout as tight as
possible (a few hundred ms), and put it *after* any `QVERIFY_TRUE_WAIT` that already
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

## Running Tests

### Via CTest

```bash
ctest --parallel $(nproc)          # All tests in parallel
ctest -L Unit                      # Unit tests only
ctest --output-on-failure -L Unit  # Unit tests, print output on failure
ctest -L Integration               # Integration tests
ctest -LE "Flaky|Network"          # Exclude flaky and network tests
ctest -R "Camera|Mission"          # Tests matching pattern (name regex)
ctest -R MyTest                    # Run a single test by name
ctest --rerun-failed               # Re-run only failed tests
```

### Via `just`

```bash
just test                                    # Labels "Unit|Integration", excludes "Flaky|Network"
LABELS=Unit just test                        # Override labels
EXCLUDE=Network just test                    # Override exclusions
LABELS=Slow EXCLUDE=Flaky just test          # Both at once
```

`just test` wraps `ctest --output-on-failure -L "<LABELS>" -LE "<EXCLUDE>"` and matches
the label filters CI uses by default.

### Via the QGroundControl Binary (unittest build)

```bash
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
ninja check              # All tests
ninja check-unit         # Unit tests only
ninja check-integration  # Integration tests
ninja check-fast         # Exclude slow tests
ninja check-ci           # CI-safe (excludes Flaky, Network)
ninja check-flaky        # Repeat until-fail to surface flaky failures (excludes Network)
```

### Category-Specific Targets

```bash
ninja check-missionmanager   # MissionManager tests
ninja check-vehicle          # Vehicle tests
ninja check-utilities        # Utilities tests
ninja check-mavlink          # MAVLink tests
ninja check-comms            # Comms tests
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
ctest -R QmlQuickTests --output-on-failure

# In-process sanity check runner
ctest -R QmlTestFileValidator --output-on-failure
```

### JUnit XML Output

```bash
# Single test with XML output
./QGroundControl --unittest:MyTest --unittest-output:results.xml

# All tests via CTest with JUnit output
ctest --output-junit results.xml
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

## Debugging Test Failures

### Enable Verbose Output

```bash
QGC_TEST_VERBOSE=1 ./QGroundControl --unittest:MyTest
```

### Run Single Test Method

```bash
./QGroundControl --unittest:MyTest -- -v2 _specificMethod
```

### Common Issues

1. **Test timeout**: Check for blocking operations, increase timeout with `TIMEOUT`
2. **Resource conflicts**: Use `RESOURCE_LOCK` or `SERIAL` for exclusive access
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
