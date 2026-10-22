# Unit Tests

_QGroundControl_ (QGC) contains a set of unit and integration tests that must pass before a pull request will be accepted. The addition of new complex subsystems to QGC should include a corresponding new unit test to test it.

Tests are compiled into the _QGroundControl_ executable when the `QGC_BUILD_TESTING` CMake option is on, which is the default for Debug builds (including `just configure` and the `*-debug` presets).
Each test is registered with CTest, which runs it in a separate _QGroundControl_ process.

## Running Tests

After a Debug build, run the tests with `just` (from the repository root):

```sh
just test                      # Unit and Integration tests, excluding Flaky and Network (like CI)
LABELS=Unit just test          # Override the included labels
EXCLUDE=Network just test      # Override the excluded labels
```

Or call CTest directly:

```sh
ctest --test-dir build --output-on-failure -L Unit   # Tests with the Unit label
ctest --test-dir build -R ParameterManagerTest       # Tests whose name matches a regex
ctest --test-dir build --rerun-failed                # Only the tests that failed last time
```

You can also run tests straight from the executable, which is handy when debugging (see [Command Line Options](../command_line_options.md)):

```sh
./build/Debug/QGroundControl --list-tests                     # List the registered tests
./build/Debug/QGroundControl --unittest:ParameterManagerTest  # Run one test
./build/Debug/QGroundControl --unittest                       # Run all tests
```

## Labels and Parallel Runs

Every test has CTest labels: `Unit`, `Integration` or `Slow` set its timeout, and category labels such as `Vehicle`, `MissionManager` or `Comms` are for filtering.
`Flaky` and `Network` tests are excluded from pull request runs.

CTest runs tests in parallel, with isolated settings and temporary directories for each test.
Tests that share hardware or fixed resources declare a CTest `RESOURCE_LOCK`.
Timing-sensitive tests marked `SERIAL` run alone (`RUN_SERIAL`) in sanitizer and coverage builds, and otherwise reserve two CTest processors.

## Sanitizers and Coverage (Linux)

CI also runs the tests on Linux with AddressSanitizer + UndefinedBehaviorSanitizer and with coverage instrumentation.
Reproduce those runs with the matching presets (`QT_ROOT_DIR` must point at your Qt kit):

```sh-vue
export QT_ROOT_DIR=~/Qt/{{ $frontmatter.qt_version }}/gcc_64
cmake --workflow --preset Linux-sanitizers   # ASan + UBSan build and tests
just coverage                                # Linux-coverage build, tests and report in build/Linux-coverage/
```

CTest sets the sanitizer runtime options (for example `halt_on_error=1`) on every test, overriding any `*SAN_OPTIONS` in your shell; run the test executable directly to try other options.

## Writing Tests

See the [testing guide](https://github.com/mavlink/qgroundcontrol/blob/master/test/README.md) in the source tree for the test base classes, registering a test with `add_qgc_test()`, labels and timeouts, wait helpers and `MultiSignalSpy`.
