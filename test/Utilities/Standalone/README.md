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

`QGC::Wire` is a Qt-free, header-only target shared with the DataFlash and ULog
parsers. `LittleEndian::read<T>` decodes integer and IEEE floating-point scalars
from a byte span and offset, returning `std::nullopt` for insufficient input.
It supports unaligned input and rejects out-of-range offsets without overflow.
Boolean types, including cv-qualified forms, are excluded from scalar decoding.
`QGCWireConsumer` covers explicit wire fixtures, signed values, floating-point
special values, truncated buffers and invalid offsets.
The already-Qt DataFlash parser converts binary16 values through `qfloat16` after
the bounded integer read; its application suite compares exact binary32 bits for
signed zero, normal/subnormal boundaries and infinities.

Each utility declares its public headers with a CMake `HEADERS` file set. The
default build uses the shared `test/LibraryBoundaryChecks.cmake` helpers to run
CMake's `VERIFY_INTERFACE_HEADER_SETS` checks and build a separate consumer
that links only its owning target, preventing accidental dependencies between tests.

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

To also build the wire consumer and its isolated public-header check without Qt:

```sh
cmake -S test/Utilities/Standalone -B build/utility-qt-free \
    -DQGC_UTILITY_QT_FREE_ONLY=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/utility-qt-free
ctest --test-dir build/utility-qt-free --output-on-failure
```
