# NTRIP caster captures

Unmodified byte slices of what NTRIP casters sent, from the regression logs of
[gpsd](https://gitlab.com/gpsd/gpsd) at commit `092e62282f7390840b272692bc258853643b072c` and from
the tests of the pinned pyrtcm. `NTRIPCaptureTest` decodes each one whole, byte by byte and in
chunks of seven bytes.

| File | Upstream file | Offset, length | Content |
| --- | --- | --- | --- |
| gpsd-sapos-sourcetable.cap | [gpsd `ntrip_sourcetable.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/ntrip_sourcetable.log) | 280, 2517 | NTRIP 2 source table of the SAPOS caster: the `SOURCETABLE 200 OK` response with its headers, 15 CAS, one NET and four STR rows |
| gpsd-orgn-caster.cap | [gpsd `rtcm3.bndm.log`](https://gitlab.com/gpsd/gpsd/-/blob/092e62282f7390840b272692bc258853643b072c/test/daemon/rtcm3.bndm.log) | 276, 1163 | Stream of an Oregon Real-time GNSS Network mountpoint: RTCM 1004, 1006, 1008, 1012, 1013, 1029, 1033 and 1230, with a stray byte between two messages |
| pyrtcm-ntrip-uscl.cap | [pyrtcm `pygpsdata-NTRIP-USCL00CHL0.log`](https://github.com/semuconsulting/pyrtcm/blob/620833ee9031207d6181c4818f32b20bab31f9b5/tests/pygpsdata-NTRIP-USCL00CHL0.log) | 0, 4606 | Stream of the IGS mountpoint USCL00CHL0: 37 messages of 35 types, with 1005 and 1006 |

gpsd keeps the source table with LF line endings. Its `Content-Length` of 2364 is the size of the
body with the CRLF line endings the caster sent, which the test restores. The two streams start at
their first correction, so the test puts the status line of an NTRIP 1 caster before them.

## Licenses

gpsd is BSD-2-Clause; `gpsd-LICENSE` is its `COPYING` unchanged and covers the `gpsd-*.cap` files.
pyrtcm is BSD-3-Clause; `pyrtcm-LICENSE` is its `LICENSE` unchanged and covers
`pyrtcm-ntrip-uscl.cap`.

## Expectations

`test/GPS/Core/Protocols/fixtures/generate_expectations.py` writes `NTRIPCaptureExpectations.h`:
the corrections and station positions as the pinned pyrtcm decodes them, and the source table rows
split at their semicolons. No expected value comes from a QGC decoder.
[`test/GPS/Core/Protocols/fixtures/SOURCES.md`](../../Core/Protocols/fixtures/SOURCES.md) describes
how to run it and how to compare the captures with their upstream files.
