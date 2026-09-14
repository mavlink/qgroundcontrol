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
I/O and JSON-validation targets without QML, application settings,
or external GPS dependencies.

Each utility declares its public headers with a CMake `HEADERS` file set. The
default build runs CMake's `VERIFY_INTERFACE_HEADER_SETS` checks and builds a
separate consumer that links only its owning target, preventing accidental
dependencies between tests.

`QGCIOConsumer` checks explicit and fallback receipt timestamps, including unknown
receipts. `QGCNetworkIOConsumer` receives localhost datagrams and verifies that
partial reads retain each datagram's receipt time and closing resets it.

`CRC32Consumer` checks the Math checksum header directly without Qt, including
incremental updates and empty input. `UtilityLibraryTest` covers scheduling,
JSON validation and logging registration. The deterministic scheduler is provided by the shared
`QGCTestTiming` test-support target. These tests also run in the normal QGC unit suite, alongside the UDP, scheduler and logging
model integration tests.

To check the CRC header without finding or linking Qt:

```sh
cmake -S test/Utilities/Standalone -B build/utility-crc \
    -DQGC_UTILITY_CRC_ONLY=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/utility-crc
ctest --test-dir build/utility-crc --output-on-failure
```
