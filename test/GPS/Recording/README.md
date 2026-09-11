# GPS recording format tests

`GPSRecordingFormatTest` validates the shared recording document without the native
driver, receiver worker, network transports, QML, or application. The library source
and dependency definitions come from `src/GPS/Libraries.cmake`.

```sh
cmake -S test/GPS/Recording -B build/gps-recording -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build/gps-recording
ctest --test-dir build/gps-recording --output-on-failure
```

The normal test build also registers this executable with `Unit;GPS;Recording`
labels. End-to-end capture and native transport replay remain in `test/GPS/Replay`.
