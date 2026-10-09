# Receiver fixtures

## Recordings

Unmodified frame slices of BSD-3-Clause upstream test logs, also used as protocol fuzzer seeds.

| File | Upstream test file | Offset, length |
| --- | --- | --- |
| navigation.ubx | [pyubx2 `pygpsdata-NAV.log`](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/pygpsdata-NAV.log) | 0, 2900 |
| nav-hpposllh.ubx | [pyubx2 `pygpsdata-NAVHPPOS.log`](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/pygpsdata-NAVHPPOS.log) | 36, 44 |
| mixed.gps | pyubx2 `pygpsdata-MIXED-RTCM3.log` | 0, 1227 |
| relative.ubx | pyubx2 `pygpsdata-NAV-ZED-X20P.log` | 498, 72 |
| gga.nmea | pyubx2 `pygpsdata-NMEA.log` | 1236, 73 |
| geodetic.sbf | [pysbf2 `pygpsdata_x5_pvtgeod.log`](https://github.com/semuconsulting/pysbf2/blob/7ea5e3aecb1349396d47f80938cfe20c461af95b/tests/pygpsdata_x5_pvtgeod.log) | 0, 268 |
| attitude.sbf | pysbf2 `pygpsdata_x5_attitude.log` | 0, 84 |

The tests take single messages from these files through `GPSTest::FixtureSlice`
(`Support/GPSProtocolTestBase.h`): NAV-PVT, NAV-SAT and NAV-DOP from `navigation.ubx`, and
the first PVTGeodetic block of `geodetic.sbf`.

## Licenses

All oracle projects are BSD-3-Clause; their notices are kept unchanged in `pyubx2-LICENSE`,
`pysbf2-LICENSE` and `pynmeagps-LICENSE`, which also cover the use of the recordings as fuzzer
seeds. pyrtcm only decodes the RTCM in `mixed.gps`, which keeps its pyubx2 recording license.

## Synthetic fixtures

The `synthetic-*` files in this directory are invented field values that `synthetic_fixtures.py`
passes to the pinned `UBXMessage`, `NMEAMessage` and `SBFMessage` serializers, which supply the
layouts, checksums and padding. No QGC code is used to build them. `generate_expectations.py`
decodes the recordings and the synthetic files back through the pinned readers and writes
`GPSFixtureExpectations.h`, including each file's SHA-256; no expected value comes from a QGC
decoder.

The files in `../corpus/` are hand-built frames written for QGC's tests (the Unicore and Quectel
base-status seeds follow Unicore N4 EN R1.6 §7.3.27 and Quectel LG290P Protocol V1.1 §2.3.23 with
an invented ECEF position of `(0, 6378237, 0)`); none is a device recording.

## Regeneration

Optional; normal builds and tests use only the checked-in bytes and header. The network is needed
only for installation.

```sh
python3 -m venv build/gps-fixture-oracle
build/gps-fixture-oracle/bin/pip install -r test/GPS/Core/Protocols/fixtures/requirements.txt
build/gps-fixture-oracle/bin/python test/GPS/Core/Protocols/fixtures/generate_expectations.py --check
```

Omit `--check` to rewrite the synthetic fixtures and the expectations header. The script verifies
the installed oracle commits.
