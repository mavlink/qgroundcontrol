# Native GPS protocol tests

These host tests link the production `QGCGPSNative` libraries for UBX, Ashtech,
SBF, and Femto. They require C++20 and CMake 3.25 or newer, but no Qt, PX4 runtime,
receiver, or application build. The existing pinned GeographicLib dependency is
resolved through the shared production dependency helper.

## Build and run

Run from the repository root:

```sh
cmake -S test/GPS/Driver/Protocols -B build/gps-protocol-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build build/gps-protocol-tests --parallel 4
ctest --test-dir build/gps-protocol-tests --output-on-failure -L Unit
```

The suite covers decoded reports, independent binary fixtures, configuration
handshakes, I/O failures, receiver modes, and UTC conversion under three time
zones. Checks remain active in Release builds. Narrow a run with
`ctest --test-dir build/gps-protocol-tests --output-on-failure -R GPSProtocolUbx`.

`QGC_GPS_ENABLE_UBX`, `QGC_GPS_ENABLE_ASHTECH`, `QGC_GPS_ENABLE_SBF`, and
`QGC_GPS_ENABLE_FEMTO` select the compiled families; at least one must remain
enabled. SBF and Femto low-level Position-mode tests exercise their internal
protocol implementations, not a supported public facade role. The public
`gpsValidateReceiverConfig` contract still restricts Position to u-blox.

## Facade safety and legacy comparison

The separate facade suite needs Qt Core, Test, and Positioning. It exercises
the unchanged `GPSDriver::configure()` / `receive()` API and public reports:

```sh
cmake -S test/GPS/Driver -B build/gps-driver-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build build/gps-driver-tests --parallel 4
ctest --test-dir build/gps-driver-tests --output-on-failure -R GPSDriverTest
```

`GPSDriverTest` covers stale ACK/NAK handling, receiver identity classification,
time-mode and SBAS readback, malformed transport progress, configuration
evidence, survey callbacks, and restarting a retained survey. A write completion
is not a receiver acknowledgement, and an acknowledgement is not verified
readback.

Configure the facade test entry point with `-DQGC_BUILD_GPS_HARDWARE_TESTS=ON`
to retain the test-only PX4 comparison runtime, `GPSPx4DataTest`, and
`GPSHardwareRunner.LegacySafety`. That option also requires Qt Network and,
unless `QGC_NO_SERIAL_LINK=ON`, SerialPort. Production `QGCGPSDriver` never
links the legacy runtime. See the [hardware runner guide](../Hardware/README.md)
for scripted and explicitly requested physical runs.

## Sanitizer-backed fuzzing

Use a separate Clang build:

```sh
cmake -S test/GPS/Driver/Protocols -B build/gps-protocol-fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ \
  -DQGC_BUILD_GPS_PROTOCOL_FUZZER=ON -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build build/gps-protocol-fuzz --parallel 4
ctest --test-dir build/gps-protocol-fuzz --output-on-failure
```

The option instruments the same production protocol and NMEA libraries with
AddressSanitizer, UndefinedBehaviorSanitizer, and libFuzzer coverage; it does
not compile parallel copies of their sources. `GPSProtocolFuzzSmoke` runs
10,000 deterministic iterations against a build-directory copy of `corpus`.
The harness checks bounded decoded batches and aborts if decoding attempts
device I/O.

For a longer local run, keep generated corpus additions outside the source tree:

```sh
build/gps-protocol-fuzz/QGCGPSProtocolFuzz -max_total_time=60 -max_len=8192 \
  build/gps-protocol-fuzz/corpus
```

## Fixture provenance and limits

[Fixture sources and regeneration instructions](fixtures/SOURCES.md) identify
the independent decoder projects, pinned inputs, and retained licenses.
Protocol source files retain their BSD attribution.

Fixtures and scripted receivers establish software behavior only. They do not
verify physical receiver operation, survey accuracy, or a real retained-survey
reset. Physical evidence must be collected separately with the hardware runner.
