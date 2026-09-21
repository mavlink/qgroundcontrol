# GPS implementation

The GPS module separates receiver protocols, transport, accepted observations, positioning,
and correction delivery. Keep these boundaries when adding receiver families or consumers.

## Ownership and data flow

| Layer | Responsibility | Execution and ownership |
| --- | --- | --- |
| `Driver/Protocols` | Frame decoding, receiver commands, native reports | Synchronous, injected I/O; no application singleton or QML dependency |
| `Transport` | Serial/TCP/UDP I/O, bounded writes, cancellation | Owned by the receiver worker that uses it |
| `Driver` | Translate native report batches into application types | Receiver worker; publish owned values, not views into decoder buffers |
| `RTK/GPSProvider` | Configure and receive from one physical receiver | Worker thread; cancellation does not imply that the worker has already exited |
| `RTK/GPSRtk` | Authorize the current session and expose Facts | Application thread; retire registrations before asynchronous worker cleanup |
| `Core` | Observation validity, altitude provenance, satellite and freshness state | Owner-thread stores using an injectable monotonic scheduler |
| `Positioning` | Source registration, selection, Qt positioning publication | Application thread; selected-source identity and session authorize observations |
| `Corrections` | Select ingress, route complete RTCM frames, account for destinations | Owner-thread router, selector, and ledger with separate responsibilities |
| `NTRIP` | Caster connection, source tables, GGA, reconnect policy | Event-driven Qt networking; explicit session start/stop owns deferred work |

Receiver retirement invalidates registrations and visible state immediately. The worker's
serial reservation remains alive until the worker exits, preventing a replacement session
from opening the same device too early. Do not replace this with a GUI-thread wait or release
the reservation merely because cancellation was requested.

## Protocol contracts

A binary frame owns its payload until completion. Do not also interpret RTCM payload bytes as
NMEA or receiver acknowledgements. Drain retained RTCM recovery candidates before consuming
more transport bytes; resetting after a malformed candidate can discard the next valid preamble.

Use shared NMEA parsing and metadata rules rather than independently interpreting fix quality,
coordinates, or metadata ages. A valid quality-zero GGA with blank coordinates means **fix
lost**, not malformed input. Position traffic must not extend the lifetime of unrelated heading,
accuracy, or receiver-time reports.

Receiver command construction must preserve the selected connection port, required line
termination, and coordinate precision. A write, acknowledgement, readback, and operational
receiver observation are different evidence levels. An acknowledgement alone does not prove
that a requested survey duration or datum was applied.

Capabilities describe implemented behavior, not everything a receiver family might support.
Hide or reject unsupported controls rather than silently accepting settings that have no effect.
In particular, bounded correction writes can be unsupported even when receiver configuration
and reading are supported by a platform transport.

## Accepted observations

`GPSObservation` keeps the coordinate, fix quality, optional metadata, altitude datum, and
original receipt together. `GPSSourceHealth` supplies validity and freshness policy. Consumers
request the appropriate projection rather than reconstructing coordinates from unrelated Facts.

- Receiver UTC is measurement metadata, not a freshness clock.
- Receipt deadlines use the scheduler's monotonic clock domain.
- Session identifiers and registration tokens reject retired producers.
- Satellite view, satellite usage, accuracy, and integrity can have independent lifetimes.
- Unknown measurements stay absent; zero is a valid coordinate and is not a general sentinel.

The normal position lifetime is five seconds. A live vehicle heartbeat does not refresh its
GPS or fused position. Raw vehicle GPS observations use their own MSL altitude; fused observations
use a separately received global-position sample. HIGH_LATENCY2 position errors are distances,
not dimensionless dilution-of-precision values.

### Altitude and consumer policy

| Consumer | Altitude requirement |
| --- | --- |
| Ground-station map | Display altitude only when the ground-station accuracy policy accepts it |
| Follow Me | Preserve the motion projection and firmware-specific reporting contract |
| Remote ID | WGS84 ellipsoid altitude, either explicitly identified or supplied separately |
| NTRIP GGA | Mean-sea-level altitude |
| RTK fixed base | WGS84 ellipsoid altitude |

An MSL altitude cannot become ellipsoid altitude by changing its label. Conversion needs known
geoid separation or a validated geoid model. Remote ID removes unproven altitude from its
projection; regions requiring operator altitude must then report unavailable positioning.
Where horizontal-only reporting is allowed, the transmitted altitude remains unknown.

GGA source selection checks fresh accepted observations. Available fix quality and DOP are
preserved; unavailable satellite-use counts or DOP are left empty rather than manufactured.
A visible-satellite count is not automatically a used-satellite count.

## Connection and correction health

Keep these states distinct:

1. Transport opened or connected.
2. Bytes received.
3. Valid RTCM frame received.
4. Frame admitted by filtering and source selection.
5. Output accepted or delivered to a particular destination.

Nonempty invalid data must not indefinitely satisfy the valid-correction deadline. Filtering
all valid messages is different from receiving corrupt data and needs a different diagnosis.
Track live MAVLink fanout destinations separately from bounded retired history.

NMEA transport status is independent of decoder health: an occupied UDP port, unavailable serial
device, or reservation conflict must remain visible even before any position source exists.
Qt presentation must normalize unavailable sentinels before combining independent alarm states.

Explicit NTRIP stop cancels deferred configuration/reconnect work. A new user-requested session
gets a new retry budget; automatic retries within that session share its existing budget.
Streaming and finite source-table fetches share endpoint/request policy but need not share their
entire transport implementation.

## Regression coverage

Tests mirror these layers under `test/GPS`, with vehicle consumer coverage under `test/Vehicle`
and `test/FollowMe`. Use the repository's [testing guide](../../test/README.md) and
[development commands](../../tools/README.md).

Prefer scripted I/O that parses complete commands and emits realistic acknowledgements.
Exercise malformed-prefix recovery through drivers as well as the standalone framer, compare
native and Qt NMEA validity behavior, and use injected clocks for freshness boundaries.
Preserve cancellation, reentrancy, source replacement, and reservation-lifetime coverage.
Hardware validation remains necessary for receiver firmware-specific survey and datum behavior.
Ashtech averaging completion is correlated with a current start receipt, requested interval,
and ordered completion rather than accepting any retained completion line. Confirm this receipt
sequence against the deployed receiver firmware. Septentrio configuration explicitly selects
WGS84; unsupported or ambiguous reported datums are errors, not silently relabeled positions.
SBF datum 19 denotes the correction base station's datum. Selecting WGS84 locally does not
establish that external reference frame or transform corrected coordinates.
