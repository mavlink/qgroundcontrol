# GPS organization

`GPSManager` composes local receiver, NMEA, correction, recording, and presentation
services. Keep application integration here and protocol implementation in the
owning subdirectory.

| Directory | Responsibility | Standalone target |
| --- | --- | --- |
| `Core` | Normalized observations, satellite freshness, receipt timestamps, connection state, and source health | `QGCGPSCore` |
| `Driver` | Receiver configuration, native parsing facade, and cancellable worker transports | `QGCGPSDriver` |
| `Driver/PX4` | Vendored native receiver implementations behind the driver facade | `px4-gpsdrivers` |
| `NMEA` | NMEA decoding and Qt position/satellite adapters; application connection management | `QGCGPSNMEA` for decoding |
| `Receiver` | Worker sessions, bounded mailboxes, profiles, discovery, and receiver presentation | `QGCGPSReceiver` for worker sessions |
| `BaseStation` | Survey-in and fixed-base state and Facts | Application sources |
| `Corrections` | RTCM framing, source selection, delivery diagnostics, and output integration | `QGCGPSCorrections` for routing and framing |
| `Models` | Shared satellite/relative-position models and common position Facts | `QGCGPSModels` for observation models |
| `NTRIP` | Caster protocol, streaming, source-table discovery, and connection UI | `QGCGPSNTRIPSession` for protocol/session logic |
| `PositionManager` | Scoped source registration, observation adapters, source selection, and publication | Application sources |
| `Recording` | Bounded capture, transport/device recording, and export | `QGCGPSRecording` for capture |

Core has no dependency on receiver drivers or NMEA decoding. Both the NMEA decoder
and shared observation models depend on Core. Linking the models does not pull in
receiver workers, correction routing, or recording. Fact-based presentation stays
in the application target because it depends on the application Fact System.
Receiver setting/report presentation also stays there, keeping Driver out of the
standalone observation-model target.

Keep passive, event-driven `QIODevice` input separate from blocking worker
`GPSTransport` implementations. NTRIP also retains its HTTP/TLS session boundary.
Reuse normalized observations and connection contracts across these paths rather
than forcing their different I/O lifecycles into one base class.

Integration remains with the owning application subsystem:

- Persisted Facts and metadata live in `src/Settings`; generated settings-page
  definitions and their components live in `src/AppSettings`.
- Vehicle MAVLink handlers stay in `src/Vehicle/FactGroups` and share the common
  `GPSPositionFactGroup` from `Models`.
- General socket helpers, including `UdpIODevice` and `UdpForwarder`, live in
  `src/Utilities/Network`.

Tests mirror these directories under `test/GPS`. Replay and fuzz harnesses compile
the production parsers through `test/GPS/Replay/ParserTargets.cmake`; update those
paths when moving parser sources. Network utility tests live under
`test/Utilities/Network`. See [GPS acceptance tests](../../test/GPS/README.md) for
the focused test command and manual checks.

Shared runtime contracts have one owner:

- `GPSReceiverProfile` carries receiver and endpoint intent after persisted settings
  are converted. `GPSReceiverAttempt` identifies a generation and terminal state.
- `GPSSatelliteStore` accepts observations and expires each constellation's view
  and use reports. Health and models consume its snapshots.
- `RTCMFrameDecoder` and the complete-frame validator preserve first-fragment age
  across input transports. NTRIP and native correction events carry attempt IDs.
- Transport delivery records distinguish accepted, written, uncertain, and
  undelivered bytes. Correction deadlines preserve a receive opportunity between
  frames and account for serial wire time.
- `GPSRecordingDocument` provides the versioned format shared by recording and
  replay; its bounded validation reuses `QGCJsonValidation`.
