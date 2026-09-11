# GPS library tests

The application, standalone tests, replay, and fuzz targets use the same production
libraries from `src/GPS/Libraries.cmake`. QGC adapters and Fact/QML projections live
in `src/GPS/Integration` and `src/GPS/Presentation`; reusable libraries cannot link
those targets. Qt QML integration annotations are header-only dependencies here;
these tests do not start the QML engine or the QGroundControl application.

```sh
cmake -S src/GPS/Standalone -B build/gps-libraries -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build/gps-libraries --parallel 6
ctest --test-dir build/gps-libraries -L GPS --output-on-failure
```

Public headers compile in separate translation units against their owning library.
`LibraryBoundaries.cmake` rejects application dependencies in GPS targets. CTest
covers contracts, recordings/replay, native protocols, NTRIP, positioning,
connection admission/lifecycle, and shared byte-buffer/scheduler utilities.
Application tests retain real QGC settings, QML, vehicle routing, and serial registry
integration coverage. Hardware acceptance remains separate from both suites.

Use `-DQGC_NO_SERIAL_LINK=ON` to build and test the network-only configuration.
All four receiver families are enabled by default. A constrained consumer can use
`-DBUILD_TESTING=OFF -DQGC_GPS_ENABLE_ASHTECH=OFF -DQGC_GPS_ENABLE_SBF=OFF
-DQGC_GPS_ENABLE_FEMTO=OFF` to link u-blox only. The exhaustive protocol/replay tests
require all families; the `GPSLibraryConsumer` executable provides a link/runtime
smoke check for reduced-family configurations. Persistent receiver IDs remain
stable, and discovery/capability/UI lists omit disabled families.

Native protocols and their report/I/O contracts have no Qt dependency. They can
also be built directly via `test/GPS/Driver/Protocols`; see the neighboring fuzz
and replay documentation for instrumented builds and capture fixtures.
