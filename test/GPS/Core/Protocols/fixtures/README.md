# Receiver recordings

How a recording of a receiver connection is read, how one that a user sends becomes a fixture and
a regression test, how its dialogue is replayed against the driver, and which receivers the
hardware runner was run against. `SOURCES.md` lists where the fixtures in this directory come
from.

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
call that crossed the link or that the link refused, in the order the session made them.

| Record | Holds | Not recorded |
| --- | --- | --- |
| `SETTINGS` | What the session was asked to do, once, before the other records | |
| `RX` | The bytes one read returned | Reads that timed out or failed |
| `TX` | The bytes the link accepted from one write | The rest of a write that the link accepted the start of |
| `TX-REFUSED` | How the link answered one write of which it accepted nothing, then the bytes of that write. A write the session made after the time it allowed for it is one, answered `timed-out` | A write that was cancelled |
| `BAUD` | The baud rate the link changed to | |
| `BAUD-REFUSED` | A baud rate the link refused | A rate asked for after the session was stopped |

Each record starts with the microseconds from the start of the recording, when the link first
opened, to the return of its call, on the clock the session runs on. So a recording holds both
directions and the baud rate changes with their times, not only what the receiver sent. An `RX`
record is one read, not one message: frames are split and joined as the session read them. One
recording holds one session: QGroundControl starts a file for each connection, and the runner
for each stage, so a `suite` leaves two.

A session goes on after the two refusals, so its recording holds them: detection tries the next
rate when a serial adapter cannot run at one, and a link that carries no writes, as UDP, refuses
a command without ending the session. The answer of a `TX-REFUSED` is `unsupported`, `timed-out`,
`error` or `completed`:

```text
1203 BAUD-REFUSED 921600
1650 TX-REFUSED unsupported b5620a0400000e34
```

A command that the driver writes when the time it allowed for it has just run out is answered
`timed-out` before the link is given its bytes. The driver goes on to its next command, and the
record is all that shows that it tried this one.

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
| `constellations` | Only where a base leaves constellations out of its corrections: the sum of those it sends besides GPS, 1 GLONASS, 2 Galileo, 4 BeiDou, 8 QZSS |
| `base` | `survey` with `duration` (s) and `accuracy` (m), `fixed` with `latitude`, `longitude` (degrees), `altitude` and `accuracy` (m), or `averaging` with `duration` (s) |

QGroundControl and the runner write it. A recording without it, as one made by a recorder that
was not given its session, is read as before and decodes, but its dialogue cannot be replayed
until the line is added by hand from what the sender says the settings were. The numbers are
written so that they read back exactly: a fixed position that differs in its last digit gives
other command bytes.

A recording does not hold the UTC start time, which is in the file name, or the end of the
connection.

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
   the file name, and the role and base options are those of the session's `SETTINGS` record. An
   `automatic` session names no decoder: give the family it detected, which its
   [dialogue replay](#replaying-the-dialogue) reports as `detected_family`:

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
   wrong. List the capture in that script's `CAPTURES` with its directory alone, and in the list
   of its protocol: a capture without an upstream file is no slice of a public log, and the
   script's `--upstream` check leaves it as it is. Both kinds seed the protocol fuzzers.
4. **Pin the behaviour.** Fix the code, write the golden with
   `QGC_GPS_GOLDEN_UPDATE=1 ctest --test-dir build -R GPSGoldenTranscriptTest`, and review its new
   lines: they are the regression test.

This covers what the receiver sent. The next section covers the dialogue, such as a command the
receiver rejects or answers late.

## Replaying the dialogue

The receiver models answer as the drivers expect, because both were written from the same
reading of the manuals. A recording of a receiver holds what it really answered, and replaying it
as a dialogue puts the driver in front of those answers:

```sh
$R --replay 20260102T030405.006Z-ublox.log --dialogue --action configure
```

`GPSDriver` runs the session the `SETTINGS` record describes, on a virtual clock, on a link that
plays the recording (`GPSDialogueTransport` in `test/GPS/Core/Transport/Support`):

- The receiver's bytes arrive one recorded read at a time, at their recorded times. A recording
  holds no read that returned nothing, so a read that ends before the next bytes arrived times
  out after the timeout the driver gave it.
- While the next record is anything but an `RX`, the recorded session acted before it read more:
  reads time out until the driver has acted.
- What the driver writes must be the bytes of the `TX` records that follow, in order. Only the
  bytes count, not how the write calls divide them.
- A baud rate the driver sets must be that of the next `BAUD` record.
- A write of the bytes of the next `TX-REFUSED` record gets the answer the recorded link gave, and
  the rate of the next `BAUD-REFUSED` record is refused, so that the driver goes on as the
  recorded session did. The link refused one write, so the driver has to make the same one.

The first thing the driver does that the recording does not hold ends the replay. So does a
command the driver has not sent 30 seconds after the recording has it, and a session the driver
ends while records remain. The report's `divergence` names the line of the record, what the
recording has there and what the driver did, each with its time; its `outcome` is `reproduced`,
`diverged`, or `failed` for a file that is damaged, has no settings or is not a session
recording. For an `automatic` session, `detected_family` is the family the driver found, which
decided its commands. The replay ends with the recording, since a recording does not say what its
session did after its last record: one that was cut between two lines replays as far as it goes.
Nothing is decoded for the report, and the session's own duration takes no time to replay.

`--replay` decodes unless `--dialogue` is given. Decoding is the first step above: it works for
raw receiver output, for a recording without settings and whatever the driver sends today. The
dialogue needs the settings and fails as soon as a command of the driver changes, so it is asked
for.

What a replay cannot tell from a driver that changed:

- **What the link refused, in an older recording.** A recording made before the `TX-REFUSED` and
  `BAUD-REFUSED` records existed holds neither. The replay has no record to answer from and
  diverges where the session met a refusal: at a rate a serial adapter does not support, or at
  the first command of a configured receiver on UDP, which carries no writes. A recording made
  before a write after its deadline was recorded holds no such write either: the replay, whose
  clock stands still between two records, makes the write in time and diverges there.
- **A reply at a deadline.** Between two records the replay's clock runs on the driver's own
  timeouts, which a real link overruns a little; data recorded up to 2 ms after the end of a read
  counts as returned by that read. A reply that came closer than that to a deadline of the driver
  can replay on the other side of it. The divergence then shows two times that close together.

### A dialogue as a regression test

`GPSRecordingDialogueTest` replays every `*.recording` file in `dialogues/` and fails with the
divergence. No recording of a receiver has been added yet. To add one:

1. Replay it with the command above. A recording made with an older driver diverges at the first
   command that changed since: cut it there, as described below.
2. Cut it after the last line that matters, to keep the file small. A recording may end after any
   line.
3. Copy it to `dialogues/<receiver>-<what it shows>.recording`: git ignores `*.log`. Name the
   sender, the receiver and its firmware in `SOURCES.md`, with the sender's agreement to publish
   the positions it holds, which include a fixed base position in its `SETTINGS`.

The same suite makes a recording of every golden scenario at run time, from the driver's session
with the scenario's receiver model, and requires each to replay, and to fail at the changed line
when one `TX` byte or one `BAUD` is changed or one answered `TX` record is removed. It records
each scenario again on a link that refuses the first baud rate the session sets, and on one that
refuses every write, and requires those sessions to replay too, and to fail at the changed line
when the rate of a `BAUD-REFUSED` or one byte of a `TX-REFUSED` is changed. Those recordings are
not fixtures: they pass by construction, and the golden transcripts pin the same bytes.

### When the driver's commands change on purpose

A recording of a receiver cannot be made again without the receiver, and what it holds after a
changed command are the receiver's answers to the command the driver no longer sends. When a
dialogue fixture fails:

1. Read the divergence. If the change of the command was not intended, the fixture has done what
   it is for: fix the driver.
2. If it was intended, cut the fixture before the line the divergence names, so that it ends with
   the last record the driver still reproduces. It keeps pinning the dialogue up to there. Never
   edit the recorded bytes to match the new command: the answers after them were given to the old
   one.
3. Say in `SOURCES.md` which change shortened the recording. The receiver's bytes after the cut
   are still what that receiver sends: extract them from the uncut recording (`--extract`) when
   they are worth a decode fixture.
4. A fixture that is left without a `TX` record checks nothing: remove it.
5. Ask the sender, or whoever the hardware qualification table names for the receiver, for a
   recording made with the changed driver. Until it replaces the fixture, the changed part of the
   dialogue rests on the receiver model alone.

## Hardware qualification

Which receivers the runner has configured and observed. **A run is recorded for `ublox` and
`passive` alone.**

| Family (`--family`) | Receiver and firmware | Connection | Action, base mode | Date, QGC commit | Outcome | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `ublox` | ZED-F9P, HPG 1.13 (protocol 27.12) | USB | `suite` and `configure`, also with `--family automatic`: survey-in with MSM7 and with `--compact-rtcm`, fixed; `cancel` | 2026-11-17, a94d8af6f2 | `inconclusive`, no failed check. The antenna had a poor view of the sky: only a survey of 10 s within 100 m completed, and a fixed position 300 m off failed `fixed_base_position`, as it should | Kept by @HTRamsey with the recordings, unpublished: they hold the base position |
| `trimble` | No run recorded | | | | | |
| `septentrio` | No run recorded | | | | | |
| `femto` | No run recorded | | | | | |
| `unicore` | No run recorded | | | | | |
| `quectel` | No run recorded | | | | | |
| `passive` | The output of the same ZED-F9P: NMEA and UBX | USB | `configure`, passive | 2026-11-17, a94d8af6f2 | `inconclusive`, no failed check; nothing written | As above |

What the tests use for the other families, and beside those runs, is not a run against a receiver:

- The drivers configure receiver models (`../Support`). The runner's own test and the simulator's
  suite use the u-blox model with the identities of a ZED-F9P (HPG 1.32) and a NEO-M8P-2
  (HPG 1.40). Without `--transport` the runner configures that model on the model's virtual clock,
  which the report's `script` names: no wait of the session takes time, so the times in the report
  are those of the driver's timeouts, not of a receiver. `--real-time` runs the model on the
  steady clock.
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

A fixed base (`--base-mode fixed`) has the check `fixed_base_position`: how far the position it
was given lies from the one the receiver found itself, for the families that compare the two. More
than 50 m apart fails the stage, and a receiver without a position of its own leaves the check
`inconclusive`. The deadline of such a stage is 30 seconds longer, the time a u-blox receiver is
waited for.

`--family automatic` runs the session QGroundControl runs with **Detect automatically**: the
driver finds the receiver family and then configures it. Each stage reports what it found as
`detected_family`, whose row the run belongs to, and the recordings are named `automatic`, as
QGroundControl's are. Detection can take 45 seconds on a serial port, so a stage's deadline
defaults to 105 seconds. Two things a detected session does not have: a passive role, since
passive input is decoded as it arrives without detecting a receiver, and a decoder for `--replay`,
which takes the detected family instead.
