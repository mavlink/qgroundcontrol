# NTRIP integration contracts

`NTRIPGgaProvider::PositionProvider` returns a `PositionResult` with an explicit
`GPSAltitudeDatum`. The default datum is unknown. Only valid coordinates with
finite mean-sea-level altitude are eligible for GGA transmission.

Vehicle and accepted GCS MSL providers continue to work. RTK `currentAltitude`
is ellipsoid height and retains that meaning for fixed-base settings. Without
an actual geoid separation, the RTK provider cannot supply GGA altitude:
explicit RTK selection sends no GGA, and Auto continues to the next eligible
provider. A provider performing a real geoid conversion must explicitly label
its resulting altitude `MeanSeaLevel`; an invented zero separation is not a
conversion.

Source-table caching retains caster rows, not the reference position. Each
cached fetch updates distances and ordering for its current coordinate. An
invalid reference clears old distances. Single-row distance changes notify
the distance role without resetting the model. Mutations requested by reset
observers are serialized; controller replacements are deferred until the
model is stable, and superseded fetches do not publish completion.

Connection statistics periodically refresh the shared `DataRateTracker`.
An empty completed rate window reports zero without clearing cumulative
bytes or message counts.

The shared MAVLink output is available through
`QGroundControl.gpsManager.corrections.rtcmMavlink`, not `NTRIPManager`.
Application-side `GPSQmlTypes.h` still provides standalone-library QML
registration. Position selection retains its existing `LegacyPriority` policy.

Focused validation uses the production targets:

```sh
qt-cmake -S test/GPS/NTRIP/Standalone -B build/ntrip-tests -G Ninja
cmake --build build/ntrip-tests --parallel 2
ctest --test-dir build/ntrip-tests --output-on-failure -L Unit
```
