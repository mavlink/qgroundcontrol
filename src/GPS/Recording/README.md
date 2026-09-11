# Receiver recordings

GPS settings exposes an explicit receiver recorder. Start recording, connect or
reconnect the receiver to include configuration, reproduce the behavior, stop, then
export a JSON file. Capture is off by default and is never restored from settings.
Starting a new capture replaces the previous in-memory capture.

The native worker records its actual GPSTransport calls, including configuration
writes, correction writes, receive chunks, baud changes, errors and cancellation.
Passive NMEA serial/TCP/UDP sources use an unbuffered QIODevice proxy: the existing
decoder remains the only consumer of the underlying stream, and producer receipt
timestamps still flow to the live decoder. Configured NMEA receivers are recorded
at their native transport, avoiding duplicate copies of their decoded NMEA stream.
NTRIP sockets and HTTP authentication are outside this recording path.

Records can contain precise receiver positions and receiver-identifying protocol
bytes. Profile metadata intentionally includes fixed-base coordinates when used.
Device paths, host addresses, receiver display names and credentials are not
included as profile metadata. Review captured protocol bytes before sharing a file.

## Bounds and ownership

`GPSRecordingBuffer` is shared by the controller and receiver stream tokens. A
mutex protects ordered value events; receiver threads never queue byte payloads
to the UI or write files. The controller samples status every 250 ms. The first
limit reached stops capture without discarding the existing prefix: 10,000 events
or 2 MiB of conservative serialized-event accounting. The accounting charges
hexadecimal payload expansion and metadata overhead, keeping exports below the
replay loader's 4 MiB limit. It is an accounting bound, not a claim about exact heap
usage; JSON serialization uses additional bounded temporary memory.

Stop freezes the buffer, and destroying the controller stops it before retired
worker tokens are released. Export is allowed only after stop. Recording limits
and export failures do not stop or alter receiver I/O.

Exports retain version 4 JSON and its 4 MiB bound. Typed event validation is shared
with import; the exporter writes one event at a time without assembling or parsing
a second complete JSON document. Promise-mode QtConcurrent work reports event
progress, checks cancellation between events, and commits through QSaveFile only
after the document is complete. Cancellation before commit preserves any existing
destination; an atomic commit already in progress may finish. Only one export job
is admitted per controller, and starting a new capture does not alter its snapshot.

## Replay format

Exports use the version 4 contract in `GPSRecordingFormat.h`, shared with
[the replay test guide](../../../test/GPS/Replay/README.md). The decoder also reads
versions 1–3, with explicit frozen mappings for version 1 numeric enums. New files use stable string names for transport, driver,
protocol, role, configuration status and write status. Unsupported versions,
unknown metadata keys, wrong JSON types, invalid payloads and inconsistent delivery
counts are rejected before replay, including events in unselected streams.

Every event includes a `stream` identifier; simultaneous connections and reconnect
attempts get distinct identifiers. `GPSReplayTrace::load`/`fromJson` accepts an
optional identifier and defaults to the first stream. `profile` is typed allowlisted
metadata; `recordedEvents` preserves all selected markers and operation timing.
`events` contains executable transport operations. `createGPSReplayDriver` creates
the production driver from captured receiver intent without duplicating manual
configuration. It rejects passive, resumed, truncated and configuration-less captures.
Unknown legacy transport metadata must be supplied explicitly before driver replay.

`at_us` records I/O completion relative to capture start, starting at 1. `started_us`
records the operation's start when available. Timestamps are monotonic; concurrent
receiver operations are ordered when appended. These are application I/O times,
not wire-level hardware timestamps. A capture begun on an already-open receiver
inserts an `open` event with `resumed: true`. Such a capture omits earlier
configuration and must be used as an input-stream replay or with a matching
preconfigured decoder. Reconnect after starting capture to obtain a full native
configuration transaction.

Additional event kinds `session`, `configuration_started`, `configuration_finished`
and `close` are informative markers, retained in `recordedEvents`. A close can carry
a negative cancellation reason. `open_error` and `baud_error` reproduce failed
operations. A `write_error` contains the attempted bytes and reported return value;
negative results do not prove that no prefix reached the physical receiver.

Session `profile` fields map directly to these C++ values:

| JSON field | C++ value |
| --- | --- |
| `transport` | Stable `unknown`, `serial`, `tcp`, `udp`, `udp_listener`, or `udp_peer` |
| `protocol`, `role` | `native`/`nmea`, `position`/`rtk_base` |
| `driver` | `ublox`, `trimble`, `septentrio`, `femto`, or `none` for a passive stream |
| `baud` | Initial serial baud from the immutable session profile, or 0 for autodetection |
| `fixed_baud` | Link baud which cannot change; 115200 for native network bridges, otherwise 0 |
| `configured` | Whether QGC configured this receiver |
| `constellation_mask`, `dynamic_model`, `output_rate_hz` | Corresponding GPSReceiverConfig fields |
| `heading_offset_deg` | GPSReceiverConfig::headingOffsetDeg |
| `base.fixed` | GPSBaseStationConfig::useFixedBase |
| `base.survey_accuracy_m`, `base.survey_duration_s` | Survey-in accuracy and duration |
| `base.latitude`, `base.longitude`, `base.altitude_m`, `base.accuracy_m` | Fixed-base coordinates and accuracy |

`configuration_finished.status` is a stable name: `not_configured`, `ready`,
`unsupported`, `cancelled`, `transport_error`, or `failed`. The version 1 reader
translates the original status integers explicitly.

`bounded_write` stores attempted bytes and `write` evidence: `status`, `accepted`,
`written`, `uncertain`, and the transport's `fatal` state. These counts describe
transport progress, not receiver acknowledgement. Replay returns the exact recorded
outcome and advances virtual time to completion. A caller deadline shorter than the
recorded operation fails with a diagnostic; a trace cannot prove which bytes reached
the receiver before an earlier deadline. Legacy `write_error` remains intentionally
less precise because its integer return code cannot identify a transmitted prefix.

Version 4 records the allowlisted `producer`, `build`, and `configuration_revision`
profile fields. Older captures leave provenance unknown. A driver configuration
revision mismatch adds a diagnostic to strict replay failures; it never skips the
transmitted-byte comparison or declares incompatible bytes acceptable.

In memory, `GPSRecordingEvent::Payload` distinguishes session, open, read, write,
baud, configuration, bounded-write and close data. Capture and replay share these
typed payloads. `GPSRecordingJsonCodec.cc` owns version migration and wire names;
`GPSRecordingValidator` owns timing, profile and byte-evidence invariants. Import
normalizes each wire event before semantic validation, and export uses the same
validator before writing each event.

Version 3 preserves typed open/read outcomes and, where the input device supplies
it, the original producer receipt time separately from read completion. The
`received_us` offset can precede recording start and be negative; it is never
substituted with the consumption timestamp. Native transports without a producer
receipt expose only completion timing. Free-form transport error strings are not
exported. Passive network sources keep fixed baud unknown, since no serial baud
has been configured by QGC.
