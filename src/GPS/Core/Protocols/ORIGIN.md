# Origin of the receiver families

Four of the families in this directory began as ports of [PX4-GPSDrivers](https://github.com/PX4/PX4-GPSDrivers)
(BSD-3-Clause, `LICENSE.md`), which QGroundControl built from source until the native drivers replaced it. This
note records which upstream revision they came from and how to compare a family with what upstream changed
since. It states only what the two repositories show; the commands reproduce it.

## Revisions

| What | Revision | Where it shows |
| --- | --- | --- |
| Last upstream revision QGroundControl built | `cd6f506afda9bd6e8e4645f094dcf5415f1f8be4` (2026-05-28), with two QGroundControl patches, `ubx-position-time-mode.patch` and `non-ubx-safety.patch` | `git show cd7132bf07:src/GPS/Driver/Px4/CMakeLists.txt` |
| First native drivers | QGroundControl `6f1f4368de` (#15157, 2026-09-20), which removed that build | `git show --stat 6f1f4368de -- src/GPS/Driver` |
| Upstream revision the ports were written from | `ab090ca9dfe8038ae84eb795a0bed0d314896d85` (2026-09-08, upstream #236) | below |
| Rewrite into today's layout | QGroundControl `b42d1d8fd3` (#15166, 2026-10-09) and later commits | `git log -- src/GPS/Core/Protocols` |

The ports are newer than the revision that was built. The u-blox driver of `6f1f4368de` holds comments and
messages that upstream added after `cd6f506`, up to `ab090ca`, and none that `ab090ca` had removed by then. It
also holds what only `ab090ca` and its successor `2a5e78a` have, because upstream's next commit `65611f3` (#239)
reverted it: the UBX-MON-COMMS poll and the `ReadCancelled` result. The Trimble driver of that commit calls
`timeFromUtc()`, which upstream added in between (`7206539`), and the base class the four drivers shared has
`gpsTimeToEpoch()`, which `ab090ca` added. `sbf.cpp` and `femtomes.cpp` differ too little between the two
revisions to tell which one the Septentrio and Femtomes ports were written from; they arrived in the same commit.

```sh
# QGroundControl: what the first native drivers hold
git grep -n 'MON_COMMS\|ReadCancelled\|timeFromUtc\|gpsTimeToEpoch' 6f1f4368de -- src/GPS/Driver
# PX4-GPSDrivers: the commits that added and removed each
git log --oneline -S'UBX_MSG_MON_COMMS' -- src/ubx.h
git log --oneline -S'ReadCancelled' -- src/gps_helper.h
git log --oneline -S'timeFromUtc' -- src/gps_helper.h
git log --oneline -S'gpsTimeToEpoch' -- src/gps_time.h
```

Since the rewrite the families share upstream's behaviour (the commands, configuration keys and their order)
and almost none of its text. Upstream's license notice, which 27 files of `6f1f4368de` carried
(`git grep -l 'PX4 Development Team' 6f1f4368de -- src/GPS`), is in none of today's files. Today's families
also configure a receiver as an RTK base only, where upstream's drivers configure rovers too.

## Families

| `GPSType` | Directory | Upstream files | Ported from |
| --- | --- | --- | --- |
| `UBlox` | `UBX/` | `src/ubx.cpp`, `src/ubx.h` | `ab090ca` |
| `Septentrio` | `SBF/` | `src/sbf.cpp`, `src/sbf.h` | `ab090ca` or `cd6f506`: the port does not tell |
| `Trimble` | `Ashtech/` | `src/ashtech.cpp`, `src/ashtech.h` | `7206539` or later, so `ab090ca` as u-blox |
| `Femto` | `Femto/` | `src/femtomes.cpp`, `src/femtomes.h` | `ab090ca` or `cd6f506`: the port does not tell |
| `Unicore` | `Unicore/` | none | Upstream's `unicore.cpp` only parses `#UNIHEADINGA` and `#AGRICA` for its NMEA driver, which has no base mode |
| `Quectel` | `Quectel/` | none | Upstream has no Quectel driver |
| passive input | `Passive/`, `../NMEA/` | none | Shares no text with upstream's `nmea.cpp`, in `6f1f4368de` or today |

The layers under the families (`GPSCommandChannel`, `GPSProtocolRuntime`, `GPSStreamDemux`, `../RTCM/`) have
no upstream either: upstream's drivers read and write through one callback.

## Where upstream's code went

| Upstream | Here |
| --- | --- |
| `GPSDriverUBX::configure()`: baud detection and port setup | `UBX::Protocol::configure()`, `_setUpPort()` in `UBX/UBXConfigurator.cc` |
| `configureDevice()`: the CFG-VALSET messages of protocol 27 and later | The batches of `UBX/UBXPlan.cc`, one per message, run by `_configureDevice()` |
| `configureDevicePreV27()`, `restartSurveyInPreV27()` | `_configureLegacyDevice()`, `_restartLegacySurveyIn()` and the `legacy*` plans |
| `restartSurveyIn()`, `activateRTCMOutput()` | `_restartSurveyIn()`, `_activateRTCMOutput()`, `Plan::surveyIn()`, `Plan::fixedBase()`, `Plan::rtcmOutput()` |
| `payloadRxDone()` and the `payloadRxAdd*()` functions | `UBX/UBXDecoder.cc` |
| `UBX_CFG_KEY_*` in `ubx.h` | `UBX/UBXConfigKeys.h`, each checked against the key database by `UBXConfigKeyTest` |
| `GPSDriverSBF::configure()`, `payloadRxDone()` | `SBF/SBFPlan.cc`, `SBF/SBFProtocol.cc` |
| `GPSDriverAshtech::configure()`, `activateCorrectionOutput()`, `handleMessage()` | `Ashtech/AshtechPlan.h`, `Ashtech/AshtechProtocol.cc` |
| `GPSDriverFemto::configure()`, `activateCorrectionOutput()`, `handleMessage()` | `Femto/FemtoPlan.h`, `Femto/FemtoProtocol.cc` |

## Comparing a family with upstream

Upstream's changes to one driver since the ports, here u-blox:

```sh
git clone https://github.com/PX4/PX4-GPSDrivers && cd PX4-GPSDrivers
git log --reverse --format='%h %ad %s' --date=short ab090ca..origin/main -- src/ubx.cpp src/ubx.h
git diff ab090ca origin/main -- src/ubx.cpp src/ubx.h
```

Use the upstream files of the table above for another family, and `cd6f506` in place of `ab090ca` to include
what the ports already took. A change matters here when it touches what a base station needs: port setup,
the configuration keys and commands of the table above, survey-in, a fixed position, RTCM output, and the
decoding of position, survey and integrity messages. Rover modes, heading and constellation selection have no
counterpart.

The goldens in `test/GPS/Core/Protocols/golden/` hold every byte a family sends, so a change taken from
upstream shows there as a diff.

All four drivers were last compared with upstream at `496f7d686f5d4c221ac39a5ce26230d667ffdad9` (2026-10-19), by
reading: no receiver was available. Start the next comparison from that revision, and update it here.
