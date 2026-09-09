# GPS parser fuzzing

These opt-in libFuzzer executables compile the actual QGC NTRIP HTTP/chunk decoder,
RTCM parser, and NMEA stream splitter with AddressSanitizer and UndefinedBehaviorSanitizer.
The NMEA harness also calls Qt's real sentence parser. Qt itself is instrumented only
when an instrumented Qt SDK is supplied; an ordinary Qt installation remains usable.
The targets are disabled by default and require Clang with libFuzzer.

## Build and smoke test

```sh
cmake -S test/GPS/Fuzz -B build/gps-fuzz -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DQGC_BUILD_GPS_FUZZERS=ON
cmake --build build/gps-fuzz
ctest --test-dir build/gps-fuzz --output-on-failure -L Fuzz
```

Each CTest smoke run uses a fixed seed, 1,000 mutations, a 64 KiB input limit,
512 MiB RSS limit, and a two-second per-input timeout. HTTP asserts equivalent
whole-input and fragmented decoding, including failures. RTCM asserts complete
frame bounds, message IDs, and CRC validity. NMEA asserts checksum-valid bounded
output and exercises Qt parsing after fragmented delivery. Sanitizers catch memory
and undefined-behavior failures in the instrumented code. These runs are regression
smokes, not evidence of exhaustive parser coverage.

## Longer local campaigns

CMake copies the synthetic seed corpora into the build directory. Run against those
writable copies so libFuzzer does not add mutations to the source tree:

```sh
build/gps-fuzz/QGCGPSHttpFuzz build/gps-fuzz/corpus/http \
  -max_total_time=60 -max_len=65536 -timeout=2 -rss_limit_mb=512 \
  -artifact_prefix=build/gps-fuzz/artifacts/
```

Use `QGCGPSRtcmFuzz` with `corpus/rtcm` or `QGCGPSNmeaFuzz` with `corpus/nmea`
for the other parsers. HTTP seeds include both ICY and chunked binary RTCM bodies;
RTCM and NMEA seeds include valid and deliberately invalid checksums. All seeds
are synthetic, with no caster credentials or receiver captures.

A crash artifact can be reproduced by passing its filename as the sole positional
argument to the same executable. Reduce it with libFuzzer's `-minimize_crash=1`
before turning it into a focused parser regression test. Keep malformed binary
seeds byte-for-byte intact; line-ending normalization changes HTTP framing.
