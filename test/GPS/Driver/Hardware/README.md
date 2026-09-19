# GPS backend hardware runner

`QGCGPSHardwareRunner` is one reusable command-line runner for the production native
`GPSDriver` and a frozen PX4-backed `GPSLegacyDriver`. Both receive the same
`GPSReceiverConfig`, `GPSTransport`, and `GPSDriverSinks`. No UI role is enabled.

The legacy facade was copied from QGroundControl `f1e7700e0` using `git show HEAD`.
Only its class/logging names, shared sink declaration, and test build integration
are changed. A test-only compile definition isolates PX4's global SBF `crc16`
function; native protocol classes already have distinct names. The PX4 pin remains
`cd6f506afda9bd6e8e4645f094dcf5415f1f8be4`, including the existing safety patches.
Production `QGCGPSDriver` does not link PX4.
Its compatibility layouts, report adapter, patches, and dependency setup live
under `test/GPS/Driver/Legacy`, not in the production source tree.

## Build and deterministic checks

From the repository root, with a Qt installation discoverable by `qt-cmake`:

```sh
qt-cmake -S test/GPS/Driver/Hardware -B build/gps-hardware-item5 -G Ninja \
  -DCPM_SOURCE_CACHE="$PWD/.cache/CPM"
cmake --build build/gps-hardware-item5 --parallel 4
ctest --test-dir build/gps-hardware-item5 --output-on-failure -L Unit
```

The standalone entry point adds the production driver and transport CMake
directories; it does not duplicate their source lists. A containing build can
add this directory after defining `QGCGPSDriver`, `QGCGPSReceiverTransports`,
CPM, and `qgc_disable_dependency_warnings`. The legacy backend is test-only.
`QGC_NO_SERIAL_LINK=ON` excludes serial support.
The main driver-test CMake entry point gates this subdirectory behind
`QGC_BUILD_GPS_HARDWARE_TESTS=ON` (off by default). Such containing builds must
also enable receiver transports. An existing `QGCGPSLegacyDriver` target is
reused rather than defined twice.

No test opens hardware. The existing `ScriptedUBXReceiver` supplies the receiver
configuration simulation; this runner only injects navigation/survey fixtures.
`GPSHardwareRunner.LegacySafety` additionally runs the complete original
PX4-internal `GPSLegacySafetyTest` copied from the same baseline. Its assertions
are unchanged; only the standalone test base and isolated logging category
are adapted. These Ashtech/Femto cases remain test-only.

```sh
RUNNER=build/gps-hardware-item5/QGCGPSHardwareRunner
"$RUNNER"                         # plan only; constructs no transport
"$RUNNER" --help                  # lists options; no transport or writes
"$RUNNER" --action suite --backend native --model f9p
"$RUNNER" --action suite --backend legacy --model m8p
"$RUNNER" --action suite --backend legacy --output build/gps-hardware-item5/scripted-legacy.json
"$RUNNER" --action configure --role position --dynamic-model 2
"$RUNNER" --action configure --survey-state retained
"$RUNNER" --action configure --survey-state none
"$RUNNER" --action configure --role position --fault wrong-readback
```

`suite` runs base → Position → base, closes/reopens the transport, reconfigures
base, then tests cancellation of the receive loop. `role-cycle` runs only the
first three stages. `configure` runs one requested role. `cancel` configures
that role, observes briefly, then requests cancellation. Streaming input can
make receive return before cancellation; the runner continues until the stop
request rather than mistaking an early data return for failed cancellation.
This does not prove interruption of a physically blocked read. Reconfiguration is
explicit even for the cancellation action. The runner currently tests
survey-in base configuration, not fixed-position bases.

## Physical receiver safety

**Never run against a flight-critical receiver or an active correction source.**
Disconnect vehicles/consumers of its corrections, stop QGroundControl and other
programs using the same port, and secure the antenna. Obtain authorization for
the exact receiver and endpoint first.

Physical actions require both a concrete endpoint and `--allow-reconfigure`.
Without that flag the runner rejects the action before opening anything.
The default `plan` and `--help` never open a receiver, even with authorization.

Example plans (safe; no I/O):

```sh
"$RUNNER" --transport serial --device /dev/serial/by-id/REPLACE_WITH_AUTHORIZED_DEVICE
"$RUNNER" --transport tcp --host 192.0.2.10 --port 2101
"$RUNNER" --transport udp --host 192.0.2.10 --port 2101 --local-port 2102
```

Only after separate authorization, change `plan` to an explicit action:

```sh
"$RUNNER" --action suite --backend native --transport serial \
  --device /dev/serial/by-id/REPLACE_WITH_AUTHORIZED_DEVICE --allow-reconfigure \
  --survey-duration 60 --survey-accuracy 2 --observe-ms 65000 \
  --output build/gps-hardware-item5/authorized-native.json
```

TCP/UDP serial bridges must already be set to 115200 baud. Their endpoint is
explicit: there is no discovery, broadcast, port scanning, or automatic fallback.
`--timeout-ms` bounds each stage's open/configure through cooperative cancellation;
transport implementations must honor cancellation. Ctrl+C also requests
cancellation. `--observe-ms` bounds each observation window, independently of
the requested survey duration. A short window cannot establish completion.
For an additional hard process bound on Linux, an operator can wrap a complete
four-stage suite in `timeout --signal=TERM --kill-after=5s 360s`, with the above
65-second observation windows. A forced kill cannot guarantee a final report.

Only UBX supports Position and role cycles. Other receiver families can run
single base configuration checks on hardware; unsupported roles are rejected
before opening the transport. There is no role capability change in QGC's UI.
An explicit dynamic model is supported only for a single Position
configuration, not base mode or a role cycle; these limits come from the
shared receiver configuration validator.

**There is no implicit cleanup or rollback write.** Successful role cycles end
in base mode. A failed/cancelled run can leave any partially applied settings
or the last role active. Read the JSON evidence and restore the intended
configuration using an explicitly authorized command. Reopening a transport is
not a power cycle or unplug/replug test.

## JSON evidence and exit codes

One JSON object is written to stdout; diagnostic logs use stderr. Preserve both
with the receiver identity, firmware, antenna setup, and operator authorization
in your local test record. Raw readback/version payloads may identify hardware.
`--output PATH` additionally preserves JSON on disk. The parent directory must
exist and the path must be new: previous evidence is never overwritten by a
new invocation. The path is reserved and an initial snapshot committed before
opening a transport. Atomic snapshots follow configuration, each observation
second, and completed stages; the final snapshot matches stdout. A `running`
snapshot with `active_stage` is incomplete evidence, never a completed run.
Output errors abort further operations and return exit 4; stdout still reports
the error and available evidence. A killed process can leave its latest
snapshot, which must remain classified as incomplete.

| Exit | Meaning |
| --- | --- |
| `0` | Safe plan/help completed; **no hardware validation** |
| `1` | A requested operation or scripted wire contract failed |
| `2` | Invalid/unsupported/unauthorized request; no receiver opened |
| `3` | Operations completed but verification remains **inconclusive** |
| `4` | Evidence file could not be reserved or atomically updated |

The current runner deliberately does not claim an overall hardware pass:

- `evidence_origin` separates `scripted` from `physical_transport`. A simulated
  ACK/readback never becomes physical evidence. `physical_hardware_verified`
  stays false.
- `wire_evidence` distinguishes accepted/written bytes, checksum-valid UBX
  ACK/NAK frames, and raw configuration replies. Logging is bounded. A wire
  ACK's class/id may be stale or ambiguous; it is not automatically assigned
  to an individual setting. A successful write is never labelled an ACK.
- Native `configuration_evidence.commands` preserves command-specific outcomes:
  pending, transport-written, acknowledged, readback-verified, rejected, timed
  out, cancelled, or transport error. Legacy exposes only its configure return
  and the same passive wire evidence. Missing evidence is unverified, not pass.
- `requested` records the exact role, survey values, optional dynamic model,
  and constellation mask. Command-level readback evidence does **not** imply
  independent verification of every requested setting. A zero constellation
  mask means retain defaults, not a requested constellation selection.
- UBX `requested_setting_observations` extracts matching writes, ACKs with the
  same message type, and readback values from those captured frames. It does
  not send additional commands. Legacy constellation messages are retained as
  raw frames rather than decoded setting observations. An ACK is explicitly
  uncorrelated; a matching readback observation alone is not promoted to
  verified configuration. New writes discard earlier transitional readbacks.
- `survey_observations` records every bounded callback, including callbacks
  during configure. A duration exceeding elapsed time is
  `retained_or_preexisting`; a short active/nonvalid survey is
  `consistent_with_fresh`, not proof of a restart. An already-valid report is
  never promoted to fresh completion. No survey report remains inconclusive.
- Position/correction message counts are observations, not certification of
  antenna accuracy, fix quality, or delivery through a radio to a rover.
  Full survey completion, setting readback, and physical reconnect checks that
  were not established remain explicitly inconclusive.

The scripted regression test checks the runner itself: both backends, both UBX
generations, role ordering, logical reconnect, receive cancellation, fresh-like
versus retained survey observations, absence of observations, and rejected,
incorrect-readback, and cancelled configuration. Its CTest success is not
hardware acceptance.
