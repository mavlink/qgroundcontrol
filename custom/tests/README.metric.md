# Forced metric settings regression fixture

Run from any host directory with the existing `qgc-ubuntu-docker` SDK image:

```sh
docker run --rm --network none --cap-drop ALL --security-opt no-new-privileges \
  --entrypoint python3 -e CMAKE_PREFIX_PATH=/opt/Qt/6.12.0/gcc_64 \
  -e TMPDIR=/test-tmp --tmpfs /test-tmp:exec,mode=1777 \
  -v /home/dbaniel1232/CODRONE/qgroundcontrol:/repo:ro -w /repo \
  qgc-ubuntu-docker custom/tests/run_heartbeat_test.py
```

The image's default entrypoint builds QGC, so explicitly override it as shown.
The SDK prefix above is the installed path in the verified local image. The source
mount is read-only; all generated sources, build outputs and test settings are
container-local. No devices, host network, display, running QGC or simulator are
used. To exercise a metric system locale, add `-e LC_ALL=C -e LANG=C` before the
image name. Qt Test function names may be appended to select individual tests.

## Production implementation contract

The policy is implemented in the production
`CustomPlugin::adjustSettingMetaData` and `CustomPlugin::registerCustomSettings`
hooks, preserving heartbeat-off and joystick-on enforcement. The runner extracts
both hook definitions verbatim on every invocation; it does not insert policy.
Keep new hook dependencies self-contained, or extend extraction to include their
actual production definitions verbatim rather than creating policy stubs.

The actual settings group is `UnitsSettings::settingsGroup` (`Units`). Required
values from the real `UnitsSettings.h` declarations:

| Setting | Required enum |
| --- | --- |
| horizontalDistanceUnits | HorizontalDistanceUnitsMeters |
| verticalDistanceUnits | VerticalDistanceUnitsMeters |
| areaUnits | AreaUnitsSquareMeters |
| speedUnits | SpeedUnitsMetersPerSecond |
| temperatureUnits | TemperatureUnitsCelsius |
| weightUnits | WeightUnitsKg |

At construction the real `SettingsFact` invokes the metadata hook. Hidden facts
ignore persisted values but deliberately do not save the overridden default, so
registration must persist the metric value even when the Fact is already metric.
Read-only metadata alone does not reject Fact setters: runtime writes must also
be restored synchronously, including their persisted value. Coordinate format and
same-named settings in other groups must remain unaffected.

## Coverage and RED evidence

- Real unmodified `Fact`, `FactMetaData`, `SettingsFact`, `MavlinkSettings`, and
  `UnitsSettings` sources; actual C++-generated unit enum metadata, not JSON mocks.
- `QGC::runningUnitTests()` is false to exercise saved-value loading.
- Startup values/defaults before registration, with clean and saved-imperial cases.
- Scoped hidden/read-only metadata and preservation of all enum entries.
- Registration persistence, including facts materialized before registration.
- All non-policy enum values across raw, cooked, raw/value property, force and
  container writes (90 data rows), checking both final value and QSettings.
- Existing heartbeat/joystick regression cases retained.

First RED execution selected `metricStartupIgnoresSavedImperial`: build succeeded,
12 expected assertion failures, exit 12. Horizontal distance remained feet (0)
instead of meters (1); corresponding failures covered all six settings.

Final complete fixture execution against the unchanged production hooks, under
both the image's US locale and `LC_ALL=C`: **23 passed, 120 failed, 0 skipped;
exit 120**. All failures were missing metric-policy assertions: 12 startup,
6 metadata, 12 registration, 90 runtime. Existing heartbeat/joystick tests and enum
integrity/scoping checks passed. No compiler warnings or runtime warning/fatal
messages occurred in the C-locale run.

After implementing the production hooks, the complete fixture passed under both
`en_US.UTF-8` and `LC_ALL=C`: **143 passed, 0 failed, 0 skipped** in each run.
Rerun the full command in both locales after changes and require zero failures.
This isolated fixture is not a full application build, packaged UI test, or
wire-level heartbeat test.
