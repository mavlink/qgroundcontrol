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
cmake --build build/gps-protocol-tests --parallel
ctest --test-dir build/gps-protocol-tests --output-on-failure -L Unit
```

The suite covers decoded reports, independent binary fixtures, configuration
handshakes, I/O failures, receiver modes, and UTC conversion under three time
zones. Checks remain active in Release builds. Narrow a run with
`ctest --test-dir build/gps-protocol-tests --output-on-failure -R GPSProtocolUbx`.

All native configuration entry points reject invalid base/wire values before any
transport operation. Ashtech receipt tests include long-then-short replay and
poisoned tails. SBF tests distinguish fixed, determining, completed, and invalid
base states without treating navigation PVT accuracy as survey accuracy.
RELPOSNED retains unknown UTC when only GPS time-of-week is available.
Fix-state regressions distinguish unavailable metadata from an explicit loss of
fix, including missing Ashtech coordinates, rejected SBF coordinates, UBX
`GNSSFIXOK`, and NMEA invalid/estimated quality. UBX framing also exercises
overlapping sync prefixes without dropping the following valid frame.

The receiver runtime keeps only its consumed configuration surface: native output,
default receiver-rate setup, supported dynamic-model/constellation requests and
required safety readback. Unused configured-NMEA, general-readback and explicit-rate
APIs are not carried as speculative lifecycle features.

`GPSProtocolAllocation` counts C++ allocations while replaying 1,000 NAV-PVT frames.
After warmup, synchronous `consume()` callbacks reuse event storage; `decode()` still
returns an owning batch. Nested callback and retained-result checks protect that
ownership distinction. This measures allocator traffic, not end-to-end latency.

`QGC_GPS_ENABLE_UBX`, `QGC_GPS_ENABLE_ASHTECH`, `QGC_GPS_ENABLE_SBF`, and
`QGC_GPS_ENABLE_FEMTO` select the compiled families; at least one must remain
enabled. SBF and Femto low-level Position-mode tests exercise their internal
protocol implementations, not a supported public facade role. The public
`gpsValidateReceiverConfig` contract still restricts Position to u-blox.

## Facade safety and hardware validation

The separate facade suite needs Qt Core, Test, and Positioning. It exercises
`GPSDriver::configure()`, typed `receiveOutcome()`, and public reports:

```sh
cmake -S test/GPS/Driver -B build/gps-driver-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build build/gps-driver-tests --parallel
ctest --test-dir build/gps-driver-tests --output-on-failure -R GPSDriverTest
```

`GPSDriverTest` covers stale ACK/NAK handling, receiver identity classification,
time-mode and SBAS readback, malformed transport progress, configuration
evidence, survey callbacks, and restarting a retained survey. A write completion
is not a receiver acknowledgement, and an acknowledgement is not verified
readback.

`GPSNativeDataTest` covers the production report adapter, including unavailable
fields, enum normalization, velocity validity, satellite snapshots and survey
projection. It runs without opting into hardware validation.

Configure the facade test entry point with `-DQGC_BUILD_GPS_HARDWARE_TESTS=ON`
to include the native hardware runner. That option also requires Qt Network and,
unless `QGC_NO_SERIAL_LINK=ON`, SerialPort. Neither application nor test builds
fetch the old PX4 driver dependency. See the [hardware runner guide](../Hardware/README.md)
for scripted and explicitly requested physical runs.

## Sanitizer-backed fuzzing

Use a separate Clang build:

```sh
cmake -S test/GPS/Driver/Protocols -B build/gps-protocol-fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ \
  -DQGC_BUILD_GPS_PROTOCOL_FUZZER=ON -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build build/gps-protocol-fuzz --parallel
ctest --test-dir build/gps-protocol-fuzz --output-on-failure
```

The option instruments the same production protocol and NMEA libraries with
AddressSanitizer, UndefinedBehaviorSanitizer, and libFuzzer coverage; it does
not compile parallel copies of their sources. `GPSProtocolFuzzSmoke` runs
10,000 deterministic iterations against a build-directory copy of `corpus`.
The harness checks bounded decoded batches and aborts if decoding attempts
device I/O.
Sanitizer compile/link requirements follow each instrumented production target.
Direct NMEA and enabled-family consumers verify that they do not need the Native
aggregate to acquire the runtime link flags.

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
