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
worker tokens are released. Export is a bounded synchronous QSaveFile transaction
on the controller thread, allowed only after stop. Recording limits and export
failures do not stop or alter receiver I/O.

## Replay format

Exports use the version 1 format in [the replay test guide](../../../test/GPS/Replay/README.md).
Every event includes a `stream` identifier; simultaneous connections and reconnect
attempts get distinct identifiers. `GPSReplayTrace::load`/`fromJson` accepts an
optional stream identifier and defaults to the first stream. Its `profile` retains
the selected session's configuration metadata. The byte player does not automatically
create a configured GPSDriver from that metadata; the test/caller selects its driver
and configuration explicitly.

`at_us` records I/O completion relative to capture start, starting at 1. `started_us`
records the operation's start when available. Timestamps are monotonic; concurrent
receiver operations are ordered when appended. These are application I/O times,
not wire-level hardware timestamps. A capture begun on an already-open receiver
inserts an `open` event with `resumed: true`. Such a capture omits earlier
configuration and must be used as an input-stream replay or with a matching
preconfigured decoder. Reconnect after starting capture to obtain a full native
configuration transaction.

Additional event kinds `session`, `configuration_started`, `configuration_finished`
and `close` are informative markers; the replay loader skips them. A close can carry
a negative cancellation reason. `open_error` and `baud_error` reproduce failed
operations. A `write_error` contains the attempted bytes and reported return value;
negative results do not prove that no prefix reached the physical receiver.

Session `profile` fields map directly to these C++ values:

| JSON field | C++ value |
| --- | --- |
| `transport` | `GPSRecordingMetadata::Transport` enum; Unknown if the worker factory hides the endpoint |
| `protocol`, `role` | `GPSReceiverConfig::outputProtocol`, `role` enum values |
| `driver` | `GPSType` enum value, or -1 for a passive stream |
| `baud` | Passive serial initial baud, or 0 when only subsequent `baud` events establish it |
| `configured` | Whether QGC configured this receiver |
| `constellation_mask`, `dynamic_model`, `output_rate_hz` | Corresponding GPSReceiverConfig fields |
| `heading_offset_deg` | GPSReceiverConfig::headingOffsetDeg |
| `base.fixed` | GPSBaseStationConfig::useFixedBase |
| `base.survey_accuracy_m`, `base.survey_duration_s` | Survey-in accuracy and duration |
| `base.latitude`, `base.longitude`, `base.altitude_m`, `base.accuracy_m` | Fixed-base coordinates and accuracy |

`configuration_finished.value` is `GPSDriver::ConfigurationStatus`. Record the
QGC revision alongside a shared capture because enum meanings and native receiver
configuration can evolve between revisions.
