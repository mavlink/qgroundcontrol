# Independent receiver fixtures

Unmodified frame slices from BSD-3-Clause upstream test recordings. The adjacent licenses apply
also to the duplicate `upstream-*` fuzz seeds in `../corpus`. No upstream runtime dependency is required.
Expected values were independently decoded using these pinned Python implementations.

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

Optional regeneration (requires network only during installation):

```sh
python3 -m venv /tmp/gps-fixture-oracle
/tmp/gps-fixture-oracle/bin/pip install -r test/GPS/Driver/Protocols/fixtures/requirements.txt
/tmp/gps-fixture-oracle/bin/python test/GPS/Driver/Protocols/fixtures/generate_expectations.py --check
```

Omit `--check` to regenerate. The script verifies all four installed decoder
commit IDs and records the fixture SHA-256 values in the generated header.
Normal native CTest runs use only checked-in bytes and expectations.
