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

`PublicHeaders.cmake` declares public header file sets on their owning targets.
Each header compiles separately using its public include name and exact owning
library. Static-library consumers link every object, so missing implementation
dependencies cannot hide behind an unused declaration. `LibraryBoundaries.cmake`
checks transitive dependencies and aliases, including dependencies declared later
by an embedding project. Negative configure probes cover boundary violations. CTest
covers contracts, recordings/replay, native protocols, NTRIP, positioning,
connection admission/lifecycle, and shared byte-buffer/scheduler utilities.
Application tests retain real QGC settings, QML, vehicle routing, and serial registry
integration coverage. Hardware acceptance remains separate from both suites.

Set `QGC_GPS_COMPONENTS` to a semicolon-separated component list to configure only
its dependency closure. The default `All` is used by QGC. Examples:

| Components | Production Qt dependencies | Native receiver families required |
| --- | --- | --- |
| `NativeContracts`, `NMEAProtocol` | None | No |
| `Native` | None | At least one; uses GeographicLib |
| `NMEA` | Core, Positioning | No; does not configure GeographicLib |
| `NTRIPSession` | Core, Network | No |
| `Driver` | Core, Positioning | At least one |

Tests additionally find QtTest when the selected libraries use Qt. Other component
names and their dependencies are declared in `src/GPS/LibraryComponents.cmake`.
For example, this builds and tests NMEA with no native receiver implementation:

```sh
cmake -S src/GPS/Standalone -B build/gps-nmea -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64 \
  -DQGC_GPS_COMPONENTS=NMEA \
  -DQGC_GPS_ENABLE_UBX=OFF -DQGC_GPS_ENABLE_ASHTECH=OFF \
  -DQGC_GPS_ENABLE_SBF=OFF -DQGC_GPS_ENABLE_FEMTO=OFF
cmake --build build/gps-nmea --parallel 6
ctest --test-dir build/gps-nmea -L GPS --output-on-failure
```

Use `-DQGC_NO_SERIAL_LINK=ON` to build and test without serial transports.
All four receiver families are enabled by default. To build u-blox only, add
`-DQGC_GPS_ENABLE_ASHTECH=OFF -DQGC_GPS_ENABLE_SBF=OFF
-DQGC_GPS_ENABLE_FEMTO=OFF` and retain `BUILD_TESTING=ON`. Each enabled family runs
its applicable decoder/configuration tests; shared I/O tests use enabled factories.
Native capture replay exercises UBX fixtures and recorded Ashtech, SBF, and Femto
configuration exchanges. Tests requiring a specific disabled protocol are omitted
or explicitly skipped. Persistent receiver IDs remain stable, and discovery,
capability, and UI lists omit disabled families.

Native protocols can also be built directly via `test/GPS/Driver/Protocols`; see
the neighboring fuzz and replay documentation for instrumented builds and capture
fixtures.
