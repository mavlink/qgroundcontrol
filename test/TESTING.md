# QGroundControl Unit Testing

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

## Running Tests

### Via CTest

```bash
ctest --parallel $(nproc)          # All tests in parallel
ctest -L Unit                      # Unit tests only
ctest -L Integration               # Integration tests
ctest -LE "Flaky|Network"          # Exclude flaky and network tests
ctest -R "Camera|Mission"          # Tests matching pattern
ctest --rerun-failed               # Re-run only failed tests
```

### Via the QGroundControl binary (unittest build)

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

- `QmlQuickTests` (recommended for real QML testing): Runs `tst_*.qml` via a dedicated Qt Quick Test executable (`QGCQmlQuickTests`) out-of-process.
- `QmlTestRunner` (sanity/structure check): Runs inside the regular `UnitTest` harness and validates test discovery/setup, but is not the primary Qt Quick execution path.

```bash
# Real QML quick tests
ctest -R QmlQuickTests --output-on-failure

# In-process sanity check runner
ctest -R QmlTestRunner --output-on-failure
```

## Writing Tests

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

`UT_REGISTER_TEST` auto-registers the class at static-init time. There is no central list of tests to update — `main.cc`, `QGCApplication`, `QGCCommandLineParser`, and `UnitTestList` are all generic and enumerate tests at runtime via `UnitTest::registeredTests()`.

Available `TestLabel::*` enum values: `Unit`, `Integration`, `Vehicle`, `MissionManager`, `Comms`, `Utilities`, `Slow`, `Network`, `Serial`, `Joystick`, `AnalyzeView`, `Terrain`. CTest `LABELS` in `CMakeLists.txt` are a superset (e.g. `MAVLink`, `Settings`) — the two lists are independent.

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

## CTest Labels & Timeouts

| Label | Timeout | Description |
|-------|---------|-------------|
| `Unit` | 60s | Fast, isolated tests |
| `Integration` | 120s | Requires MockLink/Vehicle |
| `Slow` | 180s | Long execution time |
| `Network` | 120s | Requires network access |
| `Flaky` | 120s | May occasionally fail |
| `Vehicle` | 120s | Vehicle-related tests |
| `MissionManager` | 120s | Mission planning tests |
| `Utilities` | 60s | Utility function tests |
| `Comms` | 60s | Communication tests |
| `MAVLink` | 60s | MAVLink protocol tests |

## Wait/Timeout Helpers

### Prefer `TestTimeout::*` over literal millisecond values

All `.wait()`, `QTRY_*_WITH_TIMEOUT`, and `QVERIFY_*_WAIT` calls should use the CI-adaptive helpers rather than hard-coded numbers. The helpers auto-scale (roughly 2×) on CI so the same test works on a loaded build agent without slowing local runs:

```cpp
#include "UnitTest.h"  // provides TestTimeout namespace

TestTimeout::shortMs()   // 1s local / 2s CI   — quick signals, in-memory ops
TestTimeout::mediumMs()  // 5s local / 10s CI  — normal async work, file I/O
TestTimeout::longMs()    // 30s local / 60s CI — vehicle connect, FTP, metadata
```

Chrono-typed variants are also available: `TestTimeout::shortDuration()`, `mediumDuration()`, `longDuration()`.

### Wait macros

Prefer the QGC wrapper macros over raw `QSignalSpy::wait()`/`QTest::qWaitFor()` — they emit readable failure messages that include the signal/condition name and the elapsed time:

```cpp
QVERIFY_SIGNAL_WAIT(spy, TestTimeout::shortMs());          // spy.count() >= 1
QVERIFY_SIGNAL_COUNT_WAIT(spy, 3, TestTimeout::shortMs()); // spy.count() >= 3
QVERIFY_NO_SIGNAL_WAIT(spy, 250);                          // see gotcha below
QVERIFY_TRUE_WAIT(condition, TestTimeout::shortMs());      // QTRY_VERIFY wrapper
QCOMPARE_TRUE_WAIT(actual, expected, TestTimeout::shortMs());
```

**Gotcha — `QVERIFY_NO_SIGNAL_WAIT` always burns the full timeout.** It has to, because proving absence requires waiting the entire window. Keep its timeout as tight as possible (a few hundred ms), and put it *after* any `QVERIFY_TRUE_WAIT` that already proves state has settled.

### Condition polling

```cpp
QVERIFY(UnitTest::waitForCondition([&]() { return thing.ready(); },
                                   TestTimeout::shortMs(),
                                   QStringLiteral("thing.ready()")));
```

### Avoid

- `QTest::qWait(N)` — unconditional sleep, doesn't early-exit.
- Polling `while (!done) { QTest::qWait(20); }` — use `QTRY_VERIFY_WITH_TIMEOUT` or `waitForCondition` instead.
- Literal milliseconds in timeouts. If you *must* use a literal (e.g. tight `QVERIFY_NO_SIGNAL_WAIT`), add a comment explaining why `TestTimeout::*` isn't appropriate.

## MultiSignalSpy

```cpp
MultiSignalSpy spy;
spy.init(myObject);

// String-based API (recommended)
QVERIFY(spy.emittedOnce("valueChanged"));           // Emitted exactly once
QVERIFY(spy.onlyEmittedOnce("valueChanged"));       // Only this signal fired
QVERIFY(spy.waitForSignal("valueChanged", 5000));   // Wait with timeout
QVERIFY(spy.notEmitted("errorOccurred"));           // Signal not emitted
spy.clearAllSignals();

// Fluent expectation API
QVERIFY(spy.expect("signal").once());
QVERIFY(spy.expect("signal").atLeastOnce());
QVERIFY(spy.expect("signal").never());
QVERIFY(spy.expect("signal").times(3));

// Argument extraction
int value = spy.argument<int>("valueChanged");

// Mask-based API for multiple signals
quint64 mask = spy.mask("signal1", "signal2");
QVERIFY(spy.emittedOnceByMask(mask));
```

## Base Test Classes

| Class | Use For |
|-------|---------|
| `UnitTest` | Basic tests, no special setup |
| `CommsTest` | Tests requiring LinkManager/MockLink |
| `VehicleTest` | Tests requiring a connected Vehicle |
| `ParameterTest` | Tests involving ParameterManager |
| `MissionTest` | Tests involving MissionController |
| `TerrainTest` | Tests requiring TerrainQuery |
| `FTPTest` | Tests involving FTPManager |

## Code Coverage

```bash
# Build with coverage enabled (requires gcov, lcov, genhtml)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON
cmake --build build

# Run tests then generate report
cd build
ctest --output-on-failure
ninja coverage-report

# Report generated in build/coverage-report/index.html
```

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

## JUnit XML Output

```bash
# Single test with XML output
./QGroundControl --unittest:MyTest --unittest-output:results.xml

# All tests via CTest with JUnit output
ctest --output-junit results.xml
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
5. **"Uncategorized log messages" lint failure**: If the code under test logs via plain
   `qDebug()`/`qWarning()`/`qCritical()` (no `qCDebug(Category)` wrapper), use the QGC
   base-class helper instead of `QTest::ignoreMessage`:

   ```cpp
   // Correct — satisfies the uncategorized-log lint in the QGC harness
   expectLogMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Flight mode group not set")));
   somethingThatLogs();
   ```

6. **Expensive SKIP paths**: If a test conditionally `QSKIP`s based on a backend probe
   (e.g. "SDL didn't expose a second virtual joystick"), cache the probe result in a
   test-class member so subsequent skipping tests don't each burn a full timeout.
   See `JoystickManagerTest::_multiJoysticksSupported()` for the pattern.
