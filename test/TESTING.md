# QGroundControl Unit Testing

## Quick Start

```bash
# Configure, build, and run tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure

# Run specific test
./QGroundControl --unittest:FactSystemTestGeneric
```

## Running Tests

### By Label

```bash
ctest -L Unit              # Fast tests (no vehicle)
ctest -L Integration       # Tests requiring MockLink
ctest -L MissionManager    # Mission manager tests
ctest -LE Slow             # Exclude slow tests
ctest -LE "Flaky|Network"  # CI-safe tests
```

### Convenience Targets

```bash
ninja check           # All tests
ninja check-unit      # Unit tests only
ninja check-fast      # Exclude slow tests
ninja check-ci        # CI-safe (excludes flaky/network)
```

## Writing Tests

### Test Fixtures

```cpp
#include "TestFixtures.h"

// For tests requiring a connected vehicle
class MyVehicleTest : public VehicleTest {
    Q_OBJECT
private slots:
    void _testSomething() {
        QCOMPARE(vehicle()->armed(), false);
    }
};

// For mission planning tests
class MyMissionTest : public MissionTest {
    Q_OBJECT
private slots:
    void _testMission() {
        missionController()->insertSimpleMissionItem(...);
    }
};

// For offline tests (no vehicle)
class MyOfflineTest : public OfflineTest {
    Q_OBJECT
};
```

### Basic Structure

```cpp
#include "UnitTest.h"

class MyTest : public UnitTest {
    Q_OBJECT
private slots:
    void init() override { UnitTest::init(); }
    void cleanup() override { UnitTest::cleanup(); }
    void _myTestCase();
};

UT_REGISTER_TEST(MyTest)
```

### Register in CMakeLists.txt

```cmake
add_qgc_test(MyTest LABELS Unit Category)
add_qgc_test(MyIntegrationTest LABELS Integration Category)
add_qgc_test(SlowTest LABELS Integration Slow TIMEOUT 300)
```

## Test Helpers

### Signal Verification

```cpp
QSignalSpy spy(obj, &Class::signal);
VERIFY_SIGNAL_COUNT(spy, 1);
WAIT_FOR_SIGNAL(spy, timeout_ms);
WAIT_FOR_CONDITION(condition, timeout_ms);
```

### Assertions

```cpp
VERIFY_NOT_NULL(ptr);
VERIFY_FILE_EXISTS(path);
VERIFY_COORDS_EQUAL(coord1, coord2);
QGC_COMPARE_FLOAT(actual, expected, tolerance);
```

### Timeout Constants

```cpp
TestHelpers::kShortTimeoutMs   // 1000ms
TestHelpers::kDefaultTimeoutMs // 5000ms
TestHelpers::kLongTimeoutMs    // 30000ms
```

## MultiSignalSpy

```cpp
MultiSignalSpy spy;
spy.init(myObject);

QVERIFY(spy.checkSignal("valueChanged"));
QVERIFY(spy.checkOnlySignal("valueChanged"));  // Only this signal fired
QVERIFY(spy.waitForSignal("dataReady", 5000));
```

## Code Coverage

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_ENABLE_COVERAGE=ON
cmake --build build
cd build && ninja coverage
```

Requires: `pip install gcovr`

## Memory Sanitizers

```bash
# AddressSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_ENABLE_ASAN=ON

# UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_ENABLE_UBSAN=ON

# ThreadSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_ENABLE_TSAN=ON
```

## CTest Labels

| Label | Description |
|-------|-------------|
| `Unit` | Fast tests, no vehicle |
| `Integration` | Requires MockLink |
| `Slow` | >10s execution time |
| `Flaky` | May occasionally fail |
| `Network` | Requires network access |

Category labels: `ADSB`, `AnalyzeView`, `Camera`, `Comms`, `FactSystem`, `GPS`, `MAVLink`, `MissionManager`, `Terrain`, `Utilities`, `Vehicle`, `Video`

## Resource Locks

Tests sharing resources use CTest `RESOURCE_LOCK` to prevent conflicts:

```cmake
# Test using MockLink (default for Integration)
add_qgc_test(MyTest LABELS Integration Vehicle)

# Test needing specific resource locks
add_qgc_test(MyTest LABELS Integration RESOURCE_LOCK MockLink ParameterManager)

# Test that must run alone
add_qgc_test(MyTest LABELS Integration SERIAL)
```

## JUnit XML Output

```bash
./QGroundControl --unittest --unittest-output:junit-results.xml
```

## Debugging

```bash
./QGroundControl --unittest:MyTest -v2                    # Verbose
QT_LOGGING_RULES="qgc.*=true" ./QGroundControl --unittest # With logging
gdb --args ./QGroundControl --unittest:MyTest             # GDB
```

## Key Rules

1. Always call `UnitTest::init()` first, `UnitTest::cleanup()` last
2. Wait for signals, not time: use `QSignalSpy::wait()` or `WAIT_FOR_CONDITION`
3. Use `VERIFY_NOT_NULL(ptr)` before dereferencing
4. Use timeout constants instead of magic numbers
5. Vehicle parameters aren't ready until `initialConnectComplete`

## File Structure

```
test/
├── CMakeLists.txt           # Test registration
├── UnitTestFramework/
│   ├── UnitTest.h/cc        # Base test class
│   ├── TestFixtures.h       # VehicleTest, MissionTest, OfflineTest
│   ├── TestHelpers.h        # Macros and utilities
│   ├── MultiSignalSpy.h/cc  # Multi-signal verification
│   └── QtTestExtensions.h   # Extended Qt Test macros
├── FactSystem/
├── MissionManager/
├── Vehicle/
├── Video/
└── ...
```
