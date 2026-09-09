# GPS deterministic replay

`GPSReplayTest` compiles the production native UBX driver, NMEA stream splitter,
NTRIP session, and correction router into a small Qt test executable. Its transport
advances a virtual monotonic clock when consuming events; no receiver, socket, or
wall-clock delay is needed.

The checked-in traces are synthetic and contain no device recordings or credentials.
The UBX trace fixes the expected configuration writes and NAV-PVT result. Changes to
receiver configuration should be reviewed against that contract before updating the
trace; regenerating expected writes automatically would hide regressions.

## Run

The normal test build registers `GPSReplayTest` with the `Unit`, `GPS`, and `Replay`
labels. It can also be built independently of QGroundControl:

```sh
cmake -S test/GPS/Replay -B build/gps-replay -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build/gps-replay
ctest --test-dir build/gps-replay --output-on-failure
```

The tests cover native configuration and position decoding at three read fragment
sizes, NMEA position and receipt timestamps across disconnect/reopen, exact outgoing
bytes across split writes, timeouts, read errors, short writes, cancellation, and
NTRIP session retirement with stale correction rejection. The NMEA test calls Qt's
actual sentence parser synchronously after splitting; it does not exercise the live
position source's timer-driven publication. The NTRIP test injects a shared clock
for receipt/freshness and explicitly restarts after failure; it does not replace the
session's retry QTimer with a virtual scheduler.

## Trace format

A version 1 trace is a JSON object with an ordered `events` array:

```json
{
  "version": 1,
  "description": "Example transport transaction",
  "events": [
    {"at_us": 1, "kind": "open"},
    {"at_us": 2, "kind": "baud", "value": 115200},
    {"at_us": 3, "kind": "tx", "hex": "b562"},
    {"at_us": 1000, "kind": "rx", "hex": "010203"},
    {"at_us": 2000, "kind": "disconnect"}
  ]
}
```

`at_us` is an integer monotonic microsecond offset, in nondecreasing order. `rx`
and `tx` contain nonempty exact hexadecimal bytes. The remaining event kinds are
`timeout`, `read_error`, `write_error`, and `cancel`. An error's `value` is a
negative host error code; a write error can instead specify a short-write byte
count. `open` starts a connection epoch, and `baud` verifies the requested rate.
A `cancel` event sets the same atomic stop flag used by the transport contract.
Timeouts advance the clock; native driver deadlines may be recorded one microsecond
past their timeout because the driver tests for a strictly elapsed deadline.

Reads may further fragment an RX event while preserving its arrival timestamp.
Writes can split or combine adjacent TX events but must match every byte. Missing,
extra, or mismatched operations latch an event-indexed diagnostic. Tests should
assert both decoded outputs and `complete()` so unread data or unsent commands fail.
Loading is bounded to 4 MiB and 100,000 events.

For a real capture, export both directions with monotonic timestamps, explicit
connection and baud events, and replace identifying data before adding a fixture.
Keep the original RX arrival times when varying read fragmentation. For native
replay, wire `GPSReplayClock` to the driver's injected clock as in
`nativePosition`; this also makes configuration sleeps deterministic. Real hardware
acceptance remains separate from synthetic replay.

## Live captures

The opt-in [receiver recorder](../../../src/GPS/Recording/README.md) exports this
format directly. Pass a `streamId` to `GPSReplayTrace::load`/`fromJson` to select a
specific connection; the default selects the first stream. The parsed trace exposes
its selected `profile` metadata. Informative session/configuration/close markers
are skipped, while `open_error` and `baud_error` reproduce failed calls. A failed
write can include its expected attempted bytes as well as the reported result.

An `open` marked `resumed` represents capture starting on an existing connection,
so earlier configuration is missing. Start capture before reconnecting when a full
native-driver transaction is required. The tests record and export actual native
UBX transactions, reload them, and verify identical decoded positions at three
fragment sizes. They also roundtrip passive NMEA, receipt timestamp preservation,
configuration metadata, failed operations, and bounded concurrent capture.
