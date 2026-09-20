# Independent receiver fixtures

The recording fixtures below are unmodified frame slices from BSD-3-Clause upstream tests.
The adjacent licenses apply also to the duplicate `upstream-*` fuzz seeds in `../corpus`.
The separately identified synthetic fixtures are generated test data, not recordings.
No upstream runtime dependency is required. Expected values are independently decoded
using the pinned Python implementations.

| Files | Source | Byte offset and length |
| --- | --- | --- |
| nav-pvt.ubx | [pyubx2 NAV recording](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/pygpsdata-NAV.log) | 0, 100 |
| nav-dop.ubx | Same NAV recording | 1906, 26 |
| nav-sat.ubx | Same NAV recording | 758, 532 |
| nav-hpposllh.ubx | [pyubx2 HPPOS recording](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/pygpsdata-NAVHPPOS.log) | 36, 44 |
| pvt-geodetic.sbf | [pysbf2 geodetic recording](https://github.com/semuconsulting/pysbf2/blob/7ea5e3aecb1349396d47f80938cfe20c461af95b/tests/pygpsdata_x5_pvtgeod.log) | 0, 96 |

The fixture test checks units, signs, precision, counted satellites, fragmented delivery and checksum rejection.
Epoch tests may deliberately replace iTOW and recompute the checksum; checked-in bytes remain unchanged.

## Expanded sequence corpus

These are also unmodified slices, from the same pinned projects and covered by
`pyubx2-LICENSE` or `pysbf2-LICENSE` alongside this file.

| File | Upstream test file | Offset, length |
| --- | --- | --- |
| navigation.ubx | pyubx2 `pygpsdata-NAV.log` | 0, 2900 |
| mixed.gps | pyubx2 `pygpsdata-MIXED-RTCM3.log` | 0, 1227 |
| relative.ubx | pyubx2 `pygpsdata-NAV-ZED-X20P.log` | 498, 72 |
| gga.nmea | pyubx2 `pygpsdata-NMEA.log` | 1236, 73 |
| geodetic.sbf | pysbf2 `pygpsdata_x5_pvtgeod.log` | 0, 268 |
| attitude.sbf | pysbf2 `pygpsdata_x5_attitude.log` | 0, 84 |

`GPSFixtureExpectations.h` contains independently decoded coordinates, every
NAV-SAT identity/signal/elevation/azimuth/use flag, RTCM message IDs and sizes,
SBF velocity covariance, unavailable relative-position state, and NMEA GGA
values. QGC-specific tests separately cover fragmentation, mixed protocols,
checksum rejection, metadata-only epochs, and time rollover. The attitude
recording has the receiver's unavailable sentinel; it is not a valid-heading
acceptance test.

## Oracle-serialized synthetic cases

`synthetic_fixtures.py` supplies invented, fixed field values to the independent
`UBXMessage`, `NMEAMessage`, and `SBFMessage` serializers. It does not import QGC
code, use QGC packet builders, or read receiver/session logs. The serializers
provide the field layouts, checksums/CRC, and SBF padding. These files have no
upstream recording offsets: all bytes originate from the checked-in generation
method. The identical `synthetic-*` files in `../corpus` are reusable fuzz seeds.

The existing pins in `requirements.txt` are unchanged:

| Oracle | Commit | License notice |
| --- | --- | --- |
| pyubx2 | `9eadeeefc25162206a9200eea1a90bbb8ca315b9` | `pyubx2-LICENSE` |
| pysbf2 | `7ea5e3aecb1349396d47f80938cfe20c461af95b` | `pysbf2-LICENSE` |
| pynmeagps | `0dd3a69752b6df4172e31255748bf794082845cb` | `pynmeagps-LICENSE` |
| pyrtcm | `620833ee9031207d6181c4818f32b20bab31f9b5` | Existing `mixed.gps` bytes retain their pyubx2 recording license; pyrtcm only decodes them |

All four oracle projects are BSD-3-Clause; the original notices are retained
unchanged. Synthetic fixture construction is QGC test code, not copied upstream
test code. `pynmeagps-LICENSE` is reproduced from the pinned package's license file.

| File | Messages and cases |
| --- | --- |
| synthetic-integrity.ubx | NAV-STATUS spoof indication; two-block MON-RF with distinct noise/AGC/jamming fields; RXM-RTCM used/CRC-success then unused/CRC-failure transitions |
| synthetic-timeutc.ubx | Three NAV-TIMEUTC frames: negative nanoseconds crossing into leap day, UTC validity lost, then valid positive nanoseconds; decoded in legacy navigation mode |
| synthetic-gga.nmea | Six GGA sentences: real zero values; empty satellite/HDOP/altitude/geoid fields; southern/western coordinates and negative altitude/geoid; missing geoid alone; no fix; absent coordinates. GP/GN/GA/GL/GB talkers and quality 0/1/2/4/5 |
| synthetic-gst.nmea | Three GST sentences: unequal latitude/longitude errors, zero errors, and missing errors |
| synthetic-valid.sbf | Matching-epoch PVTGeodetic, DOP, VelCovGeodetic, AttEuler, AttCovEuler: RTK fixed, negative undulation, signed N/E/U velocity, nonzero uncertainties, heading wrapping |
| synthetic-zero.sbf | Same five blocks with real zeros, a 2D fix, and zero satellites/uncertainty |
| synthetic-unavailable.sbf | Same five blocks: RTK float with unavailable velocity/course, satellite count 255, accuracy/DOP 65535, covariance errors and floating-point do-not-use values, unavailable attitude |
| synthetic-error.sbf | PVTGeodetic with nonzero receiver error despite a nominal RTK-fixed mode |
| synthetic-invalid-time.sbf | Three PVTGeodetic frames: week 65535, TOW 4294967295, and the out-of-week boundary TOW 604800000 |

The 35 messages total 1,931 bytes. SHA-256 values below apply to both fixture
and fuzz-seed copies:

| File | SHA-256 |
| --- | --- |
| synthetic-integrity.ubx | `d9ff3e8b3d2d12a850f2c9be5da616852f3463a52bdcbe40ed3bd40ceadb42dc` |
| synthetic-timeutc.ubx | `f568418214f6829905c5a31a0393b965fc8502554cee1676262f27a6c84c9e5a` |
| synthetic-gga.nmea | `ea871d5dc40380020bc73ca8e241a32279416980ec2be9feb8479be8acbb4101` |
| synthetic-gst.nmea | `fac5d285a1b65b93519ac747eff5de27c657295e93f38319adacba7623c4c884` |
| synthetic-valid.sbf | `310a9e1c8ab6465be70f8002a59baa1bb68c09bd7afd03a25bb4dcf02e39a969` |
| synthetic-zero.sbf | `28b5d4f1bd3904c412dcec10968a3a0442990331a8e11be34df0ecf7d49fb6cc` |
| synthetic-unavailable.sbf | `21fcf9601a73ca6368293183463dc1617c1ae6932dede14300fd1c46642390b3` |
| synthetic-error.sbf | `c5a7c2bc562dc6b786401ebf56516ba8313db1f5aa61b5a7ff72b852bc4d2137` |
| synthetic-invalid-time.sbf | `647c6b6ba756ecdabe8432247db11a65cd06cd49c873edb6df2733014786fa44` |

`generate_expectations.py` independently parses the serialized bytes back through
the pinned readers. Normalization to the native report contract is explicit:
radians/degrees, up/down velocity, ellipsoid minus geoid altitude, centimetres and
two-sigma uncertainty to RMS metres, square roots of covariance diagonals,
horizontal GST error via `hypot`, and missing fields to NaN/optional absence.
UTC uses Python's timezone-independent Gregorian calendar conversion, including
signed nanoseconds. No expected value is obtained by executing a QGC decoder.

The native tests also assert consumer policy separately from scalar decoding:
missing NMEA coordinates cannot form a GGA position, GNSS week/TOW is not UTC,
unavailable or out-of-week SBF time must not produce a position, and corrupted
PVT must not turn remaining metadata into a fix. Fragmented delivery checks
incomplete frames before completion; checksum corruption and NMEA checksum
truncation are in-memory mutations, not changes to the checked-in fixtures.
The MON-RF comparison covers QGC's current first-block report only, not per-band
reporting. These cases do not claim complete protocol or receiver support.

## Stateful ASCII base-controller fuzz seeds

`../corpus/synthetic-unicore-base.ascii` and `../corpus/synthetic-quectel-base.nmea`
are original synthetic wire data, not device recordings. Their layouts follow
Unicore N4 EN R1.6 §7.3.27 (BESTNAVXYZ) and Quectel LG290P Protocol V1.1 §2.3.23
(PQTMSVINSTATUS), respectively. The invented ECEF position is `(0, 6378237, 0)`
metres; timestamps, uncertainties and observation counts are test inputs, not
hardware observations. Lines use LF, which the shared ASCII framing accepts.

The Unicore seed supplies fixed-position evidence, a duplicate epoch, a newer
epoch, then loss of the fixed solution. CRC-32 values were calculated with Python
`zlib.crc32(body.encode(), 0xffffffff) ^ 0xffffffff`, excluding `#` and `*`.
The Quectel seed supplies survey progress/completion, a duplicate, invalidity,
matching fixed-base evidence and a contradictory fixed coordinate. Its checksums
are bytewise XOR of the ASCII body, excluding `$` and `*`. Neither construction
uses a production QGC encoder.

The fuzzer configures fixed or averaging/survey controllers through the
event-scheduled test peers before decoding these seeds (odd first byte selects
fixed mode). `synthetic-unicore-averaging.ascii` repeats the first Unicore frame
with a leading space; `synthetic-quectel-fixed.nmea` repeats the two Quectel
fixed-position frames with a leading `!`. These noise prefixes select the other
operational mode and are discarded by ASCII resynchronization. All four seeds
have the same synthetic provenance and checksum method described above.
During fuzzed decode,
every read/write/baud/wait callback aborts: only the injected measurement clock
and in-memory decoder are available. Each controller also receives an empty
decode after its five-second freshness limit, exercising revocation without
additional transport data.

## Regeneration

Optional regeneration (requires network only during installation):

```sh
python3 -m venv build/gps-fixture-oracle
build/gps-fixture-oracle/bin/pip install -r test/GPS/Driver/Protocols/fixtures/requirements.txt
build/gps-fixture-oracle/bin/python test/GPS/Driver/Protocols/fixtures/generate_expectations.py --check
```

Omit `--check` to regenerate synthetic fixtures, matching fuzz seeds, and the
expectations header. `--check` performs no writes and verifies all three against
the pinned tools. Update the hash table above if synthetic construction changes.
The script verifies all four installed decoder commit IDs and records fixture
SHA-256 values in the generated header. Normal native CTest runs use only
checked-in bytes and expectations, including when UBX or SBF is disabled.
