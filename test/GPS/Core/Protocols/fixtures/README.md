# Receiver recordings

How a recording of a receiver connection is read, how one that a user sends becomes a fixture and
a regression test, and which receivers the hardware runner was run against. `SOURCES.md` lists
where the fixtures in this directory come from.

The commands use the receiver qualification runner, which the Linux test build puts in
`build/Debug` and `test/GPS/Standalone` builds without the application, in `Hardware/`:

```sh
R=build/Debug/QGCGPSHardwareRunner
```

## What a recording holds

QGroundControl records a connection while **Record receiver data** is on
([user guide](../../../../../docs/en/qgc-user-guide/settings_view/gnss_receiver.md)), and the
runner records its own with `--record <directory>`. Both write through `GPSRecordingTransport`
(`src/GPS/Core/Transport`), whose header describes the file: a header line, then one line for each
call that crossed the link, in the order the session made them.

| Record | Holds | Not recorded |
| --- | --- | --- |
| `SETTINGS` | What the session was asked to do, once, before the other records | |
| `RX` | The bytes one read returned | Reads that timed out or failed |
| `TX` | The bytes the link accepted from one write | What the link refused of a write |
| `BAUD` | The baud rate the link changed to | A rate the link refused |

Each record starts with the microseconds from the start of the recording, when the link first
opened, to the return of its call, on the clock the session runs on. So a recording holds both
directions and the baud rate changes with their times, not only what the receiver sent. An `RX`
record is one read, not one message: frames are split and joined as the session read them. One
recording holds one session: QGroundControl and the runner on a physical transport start a file
for each.

The `SETTINGS` record is what decides the commands a session sends, as `key=value` pairs:

```text
0 SETTINGS type=ublox,baud=0,fixed-baud=0,save=0,compact=0,base=survey,duration=60,accuracy=2
```

| Key | Value |
| --- | --- |
| `type` | The receiver type, as in the file name: a family, `passive` or `automatic` |
| `baud` | The selected baud rate; 0 lets the driver detect it |
| `fixed-baud` | The one rate the link runs at: 115200 for TCP and UDP, 0 for a serial port |
| `save` | 1 with consent to save settings to the receiver's flash |
| `compact` | 1 for compact (MSM4) corrections |
| `base` | `survey` with `duration` (s) and `accuracy` (m), `fixed` with `latitude`, `longitude` (degrees), `altitude` and `accuracy` (m), or `averaging` with `duration` (s) |

QGroundControl and the runner write it. A recording without it, as one made by a recorder that
was not given its session, is read as before. The numbers are written so that they read back
exactly: a fixed position that differs in its last digit gives other command bytes.

A recording does not hold the UTC start time, which is in the file name, the end of the
connection, or what the link refused.

`GPSRecordingReader` (`test/GPS/Core/Transport/Support`) is the only parser of the format. The
replay transport, the runner, the receiver simulator and the tests read recordings through it. It
returns every record with its direction and time, and stops with the line number at the first line
that is not a record. The recorder writes through a buffer that it flushes every second, so the
recording of a session that was killed can end within a record: the reader returns the records
before it and reports the cut.

## From a user's recording to a regression test

A recording contains the receiver's positions, for a base the base station's. Commit a part of one
only with the sender's agreement, and name the sender, the receiver and its firmware in
`SOURCES.md`.

1. **Decode it** as QGroundControl does, without the receiver. `--family` is the receiver type in
   the file name, or the family that an `automatic` session detected, and the role and base
   options are those of the session's `SETTINGS` record:

   ```sh
   $R --replay 20260102T030405.006Z-ublox.log --action configure --family ublox
   ```

   The report counts the fixes, satellites, corrections and base status that were decoded, and
   says what the session sent (`sent_bytes`, `baud_changes`); it contains no position. To watch
   the recording in the application, serve it to a passive receiver connection: see the receiver
   simulator in [`tools/simulation`](../../../../../tools/simulation/README.md#gnss-receiver).
2. **Extract the receiver's side.** Add `--extract received.bin` to the command above: the file is
   the raw receiver output, as the fixtures are. Cut the fewest whole messages that show the
   problem, for example `tail -c +<offset + 1> received.bin | head -c <length>`.
3. **Add the fixture** to this directory and to `SOURCES.md`.
   With a suffix that the decode goldens read, such as `.ubx`, `.sbf` or `.nmea`, the receiver
   families that can meet the file decode it at several chunk sizes
   ([GPS Protocol Golden Transcripts](../../../../README.md#gps-protocol-golden-transcripts)).
   `GPSGoldenTranscriptTest` chooses the decoders by the file name and fails for a file that none
   claims: name the file as the rule of its family expects, or add a rule.
   As a `.cap` capture, `GPSCaptureDecodeTest` compares what QGroundControl decodes with what the
   pinned tools decode (`capture_expectations.py`), which suits values QGroundControl may get
   wrong. That script's `--upstream` check takes every capture for a slice of a public log, which
   a user's capture is not, and has to skip it. Both kinds seed the protocol fuzzers.
4. **Pin the behaviour.** Fix the code, write the golden with
   `QGC_GPS_GOLDEN_UPDATE=1 ctest --test-dir build -R GPSGoldenTranscriptTest`, and review its new
   lines: they are the regression test.

This covers what the receiver sent. A problem in the dialogue, such as a command the receiver
rejects or answers late, cannot be replayed yet: the replay discards what the driver writes and
accepts every baud rate. Read the `TX` and `RX` records, and reproduce the dialogue in the
family's receiver model (`../Support`) as a golden scenario. A replay that checks each driver
write against the recorded one is to be built on `GPSRecordingReader`, which returns both
directions and their times for it.

## Hardware qualification

Which receivers the runner has configured and observed. **No run is recorded for any family.**

| Family (`--family`) | Receiver and firmware | Connection | Action, base mode | Date, QGC commit | Outcome | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `ublox` | No run recorded | | | | | |
| `trimble` | No run recorded | | | | | |
| `septentrio` | No run recorded | | | | | |
| `femto` | No run recorded | | | | | |
| `unicore` | No run recorded | | | | | |
| `quectel` | No run recorded | | | | | |
| `passive` | No run recorded | | | | | |

What the tests use instead is not a run against a receiver:

- The drivers configure receiver models (`../Support`). The runner's own test and the simulator's
  suite use the u-blox model with the identities of a ZED-F9P (HPG 1.32) and a NEO-M8P-2
  (HPG 1.40).
- The captures listed in `SOURCES.md` are the output of receivers that other projects logged.
  They show that QGroundControl decodes that output, not that it configured those receivers.

To add a row, run the runner against the receiver. Without `--action` it prints its plan and
opens nothing. `--allow-reconfigure` lets it change the receiver's configuration, which it does
not restore:

```sh
$R --family ublox --transport serial --device /dev/ttyACM0
$R --family ublox --transport serial --device /dev/ttyACM0 --action suite --allow-reconfigure \
    --output evidence.json --record recordings
```

The row takes the receiver and its firmware from the `receiver_identity` of a stage in the
evidence file, the outcome from `outcome` and the failed checks, and says where the evidence file
and the recordings are kept. The runner reports `inconclusive` rather than `passed` for what it
cannot prove, such as a fresh survey; copy that. A run against a receiver model, the simulator or
a replay is not a row.
