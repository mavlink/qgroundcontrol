# GPS contract boundary test

This standalone consumer links `QGCGPSContracts` with Qt Core alone. It checks
profile normalization and validation and compiles the public configuration,
capability, connection error, and transport result contracts.

```sh
cmake -S test/GPS/Contracts -B build/gps-contracts -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build/gps-contracts
ctest --test-dir build/gps-contracts --output-on-failure
```
