# Utility library tests

Build the reusable utilities without the QGroundControl application or GPS modules:

```sh
cmake -S test/Utilities/Standalone -B build/utility-libraries \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/utility-libraries
ctest --test-dir build/utility-libraries --output-on-failure
```

The standalone build requires a C++20 compiler and Qt 6.8 or newer with Core,
Network and Test. It links the production logging-category, I/O, timing, network
I/O, wire-decoding and JSON-validation targets without QML, application settings,
or external GPS dependencies.

Each utility declares its public headers with a CMake `HEADERS` file set. The
build compiles every header in isolation and builds a separate consumer that
links only its owning target, preventing accidental dependencies between tests.

`UtilityLibraryTest` covers wire values, CRC, scheduling, JSON validation and
logging registration. `TimestampedByteBufferTest` covers partial reads, retained
timestamps, discard boundaries and producer/consumer notifications. These tests
also run in the normal QGC unit suite, alongside the UDP, scheduler and logging
model integration tests.

To check the wire and CRC target without finding or linking Qt:

```sh
cmake -S test/Utilities/Standalone -B build/utility-wire \
    -DQGC_UTILITY_WIRE_ONLY=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/utility-wire
ctest --test-dir build/utility-wire --output-on-failure
```
