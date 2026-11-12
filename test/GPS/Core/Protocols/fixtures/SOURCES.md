# Receiver fixtures

[README.md](README.md) describes how a recording of a receiver connection becomes a fixture.

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

## Captures

The `*.cap` files are unmodified byte slices of logs that receivers produced, from the regression
logs of [gpsd](https://gitlab.com/gpsd/gpsd) (BSD-2-Clause) at commit
`092e62282f7390840b272692bc258853643b072c` and from the pinned pyubx2. `GPSCaptureDecodeTest`
decodes each one whole, byte by byte and in chunks of seven bytes, and they seed the protocol
fuzzers. The golden transcripts do not read them. A capture that a user sent has no upstream
file: its row names the sender, the receiver and its firmware instead.
`GPSSatelliteReportTest` checks the satellites of each NAV-SAT in them, and of the GSV and GSA of
`gpsd-ac12-pashr.cap`. Its expected values were read from the bytes by hand, at the offsets of the
u-blox interface description, and are not in `GPSCaptureExpectations.h`.

| File | Upstream file | Offset, length | Content |
| --- | --- | --- | --- |
| gpsd-zed-f9t-base.cap | [gpsd `ublox-zed-f9t-rtcm3.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/ublox-zed-f9t-rtcm3.log) | 327, 3216 | ZED-F9T base holding a surveyed-in position: two epochs of UBX navigation output, whose NAV-PVT report the time-only fix, and RTCM 1005, 1077, 1087, 1097, 1127, 1230 and 4072 |
| gpsd-zed-f9p-rtk.cap | [gpsd `ublox-zed-f9p-rtk.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/ublox-zed-f9p-rtk.log) | 462, 882 | ZED-F9P rover: one epoch with a float carrier solution |
| gpsd-neo-m8p-rover.cap | [gpsd `ublox-neo-m8p.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/ublox-neo-m8p.log) | 881, 534 | NEO-M8P rover: one epoch with the 40-byte NAV-RELPOSNED (version 0) |
| pyubx2-zed-f9p-session.cap | [pyubx2 `ucenter-ZEDF9P-configdebug.log`](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/ucenter-ZEDF9P-configdebug.log) | 14098, 1409 | u-center session with a ZED-F9P (HPG 1.32), both directions: MON-VER poll and reply, CFG-MSG commands with their ACK-ACK, NAV-PVT, NAV-RELPOSNED and the NAV-SVIN of a receiver that runs no survey |
| pyubx2-zed-f9p-rejected.cap | pyubx2 `ucenter-ZEDF9P-configdebug.log` | 19063, 259 | The same session: two ACK-NAK to CFG-MSG commands, around a MON-VER reply |
| gpsd-ac12-pashr.cap | [gpsd `ac12_binary.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/ac12_binary.log) | 510, 1642 | Ashtech AC12: two cycles of NMEA with `$PASHR,POS` and `$PASHR,SAT`, and binary `$PASHR,MCA` and `$PASHR,PBN` |
| gpsd-trimble-rtk.cap | [gpsd `nmea-rtk.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/nmea-rtk.log) | 18587, 1262 | Trimble receiver: GGA and RMC at 10 Hz, three differential fixes and then four RTK fixed ones |

None of these sources records the NAV-SVIN of a running or finished survey, replies to Septentrio
commands, a Trimble `$PASHR,RECEIPT`, or Unicore N4 or Quectel LG290P output.

## Licenses

All oracle projects are BSD-3-Clause; their notices are kept unchanged in `pyubx2-LICENSE`,
`pysbf2-LICENSE` and `pynmeagps-LICENSE`, which also cover the use of the recordings as fuzzer
seeds. `pyubx2-LICENSE` covers the `pyubx2-*.cap` files too. pyrtcm only decodes the RTCM in
`mixed.gps`, which keeps its pyubx2 recording license. gpsd is BSD-2-Clause; `gpsd-LICENSE` is its
`COPYING` unchanged and covers the `gpsd-*.cap` files.

## Synthetic fixtures

The `synthetic-*` files in this directory are invented field values that `synthetic_fixtures.py`
passes to the pinned `UBXMessage`, `NMEAMessage` and `SBFMessage` serializers, which supply the
layouts, checksums and padding. No QGC code is used to build them. `generate_expectations.py`
decodes the recordings and the synthetic files back through the pinned readers and writes
`GPSFixtureExpectations.h`, including each file's SHA-256; no expected value comes from a QGC
decoder. It decodes the captures the same way (`capture_expectations.py`) into
`GPSCaptureExpectations.h`, and those of `test/GPS/NTRIP/fixtures` into its
`NTRIPCaptureExpectations.h`. pynmeagps checks the checksum of `$PASHR,POS` and splits its fields
but does not name them: their order is that of the Ashtech manual, and the script requires the
coordinates to equal those of the GGA of the same fix, which pynmeagps decodes by name.

The files in `../corpus/` are hand-built frames written for QGC's tests (the Unicore and Quectel
base-status seeds follow Unicore N4 EN R1.6 §7.3.27 and Quectel LG290P Protocol V1.1 §2.3.23 with
an invented ECEF position of `(0, 6378237, 0)`); none is a device recording.

## u-blox configuration keys

`UBXConfigKeyDatabase.h` lists the name, id, type and value size of every configuration key in the
`UBX_CONFIG_DATABASE` of the pinned pyubx2, which follows the u-blox interface descriptions;
`pyubx2-LICENSE` covers it. `generate_config_keys.py` writes it, or compares it with `--check`, in
the environment described below. `UBXConfigKeyTest` checks the keys the u-blox driver uses against
it: the scripted receiver of the other tests reads the driver's own key constants.

## Regeneration

Optional; normal builds and tests use only the checked-in bytes and header. The network is needed
only for installation.

```sh
python3 -m venv build/gps-fixture-oracle
build/gps-fixture-oracle/bin/pip install -r test/GPS/Core/Protocols/fixtures/requirements.txt
build/gps-fixture-oracle/bin/python test/GPS/Core/Protocols/fixtures/generate_expectations.py --check
build/gps-fixture-oracle/bin/python test/GPS/Core/Protocols/fixtures/generate_config_keys.py --check
```

Omit `--check` to rewrite the synthetic fixtures and the expectations headers. The script verifies
the installed oracle commits.

With `--upstream <directory>`, the script also compares every capture that is a slice of an
upstream file with that slice, or rewrites those captures without `--check`. A capture that
`capture_expectations.py` lists without an upstream file, as one a user sent, only has to be
there: its row in the table above says where it comes from, and the expectations header pins its
SHA-256. The directory holds checkouts named `gpsd`, `pyubx2` and `pyrtcm` at the commits above:

```sh
git clone https://gitlab.com/gpsd/gpsd.git build/gps-fixture-upstream/gpsd
git -C build/gps-fixture-upstream/gpsd checkout 092e62282f7390840b272692bc258853643b072c
```
