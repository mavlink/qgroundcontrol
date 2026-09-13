# GPS library boundary checks

The protocol reports and I/O contracts use standard C++ only. The Qt contracts
require Qt Core. Observations and stores depend directly on the native contracts,
Qt Positioning, and the shared timing and logging libraries. The common monotonic
clock is a Qt-free utility used by both native contracts and Qt adapters. The satellite and relative models depend on those stores.
Fact projections are built with the application because they use QGC's Fact System.

CMake's `VERIFY_INTERFACE_HEADER_SETS` compiles each public header independently
using only its owner's public link interface. The verification targets are part
of the default build. Executable consumers check native deadlines, accepted-state expiry,
session isolation, and model/store integration without linking QGroundControl.
These checks also run in the application's Unit suite.

```sh
cmake -S test/GPS/Standalone -B build/gps-pr4-libraries -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build/gps-pr4-libraries
ctest --test-dir build/gps-pr4-libraries --output-on-failure
```

To verify the native contracts without finding Qt:

```sh
cmake -S test/GPS/Standalone -B build/gps-pr4-native -G Ninja \
  -DQGC_GPS_NATIVE_ONLY=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/gps-pr4-native
ctest --test-dir build/gps-pr4-native --output-on-failure
```

To verify Core and Models without the Qt receiver-contract library:

```sh
cmake -S test/GPS/Standalone -B build/gps-pr4-core -G Ninja \
  -DQGC_GPS_CORE_ONLY=ON -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build/gps-pr4-core
ctest --test-dir build/gps-pr4-core --output-on-failure
```

The Qt Core-only profile consumer is in [../Contracts](../Contracts/README.md).
Connection orchestration, decoding, source selection, and the native driver are
separate changes; these libraries establish their common data contracts.
