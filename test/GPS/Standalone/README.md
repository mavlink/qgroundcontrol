# GPS library boundary checks

The Contracts library contains receiver configuration, validation, and transport
results without application or driver dependencies. Its native layer provides
Qt-free base settings, constellation identities, altitude datums, and I/O statuses.
The Core library adds position and satellite observations, source health, and
survey status. It uses native contracts, Qt Core, Qt Positioning, and the shared
timing and logging libraries, not the receiver configuration library.
The QML registration header, `src/GPS/GPSQmlTypes.h`,
is compiled only by the application. The positioning service handles source registration,
selection, and recovery; QGC owns permissions and platform/custom/NMEA source
creation. The NMEA library owns passive sentence framing, Qt position decoding,
satellite assembly, and independent receipt-based freshness. Its input device is
borrowed; it does not write receiver configuration or depend on QGC settings.

CMake's `VERIFY_INTERFACE_HEADER_SETS` compiles each public header independently
using only its owner's public link interface. These checks are part of the
default build. Executable consumers verify accepted-position expiry, source
registration, and session ownership without linking QGroundControl. They use
`QGCTestTiming` for deterministic scheduling and also run in the application's
Unit suite.

The NMEA protocol is a separate Qt-free static library. Its consumer links only
`QGCGPSNMEAProtocol` and exercises sentence decoding, constellation resolution, and
satellite assembly without a Qt application.

The default standalone build requires a C++20 compiler and Qt 6.8 or newer with Core and
Positioning:

```sh
cmake -S test/GPS/Standalone -B build/gps-libraries -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-libraries
ctest --test-dir build/gps-libraries --output-on-failure
```

`GPSTransport.h` exposes the public interface using the canonical
`GPSIOStatus.h` enums and `GPSTransportResult.h` results directly. Two consumers compile
these headers in both include orders, in standalone and application builds.
Socket waiting is private to the TCP and UDP implementations.

The transport libraries provide typed open/read/write results and independent
serial, TCP, and UDP receiver connections. They require Qt Core and Network, plus
SerialPort when serial support is enabled. The existing PX4 driver callbacks
translate these results to their legacy integer interface. TCP and UDP classes
are available to receiver factories; settings and UI for selecting them belong
to the later receiver-lifecycle work.

Select only the desired components with `QGC_GPS_COMPONENTS`; dependencies are
added automatically. For example, build the network transports without serial,
Positioning, NMEA, or the application:

```sh
cmake -S test/GPS/Standalone -B build/gps-transports -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation \
  -DQGC_GPS_COMPONENTS=ReceiverTransports -DQGC_NO_SERIAL_LINK=ON
cmake --build build/gps-transports
ctest --test-dir build/gps-transports --output-on-failure
```

The available components are `NativeContracts`, `Contracts`, `Core`, `NMEA`, `Positioning`, `Transport`,
`ReceiverTransports`, `RTCMFramer`, `RTCM`, and `Corrections`. The `Transport` library
needs Qt Core, Contracts, and the logging library, not Qt Network. The contract
and base-configuration tests additionally use Qt Test, not Qt Positioning.
Core survey-status coverage stays with the Core component.
All components are enabled by default. `RTCM` and `Corrections` automatically include `RTCMFramer`.

## Receiver data contracts

`QGC::GPSNativeContracts` publishes the shared base configuration, constellation,
altitude-datum, and I/O-status types used by the active drivers and NMEA decoders.
`QGC::GPSContracts` adds Qt receiver configuration, validation, connection errors,
and transport results. The existing PX4 driver remains in use; these targets do
not enable new receiver settings or replace its runtime.

The RTK provider and driver consume `GPSBaseStationConfig` directly. Shared
receiver validation preserves the uint32 survey-duration range and existing
fixed-base float wire limits.

`GPSReceiverConfig::validationError()` checks configuration shape and wire limits.
The active driver separately rejects unsupported roles, protocols, and settings.
Receiver identity and manufacturer matching remain in the RTK connection path;
there is no separate capability catalog or profile policy.
`GPSAltitudeDatum` defines the shared native datum values (`Unknown=0`,
`MeanSeaLevel=1`, `Ellipsoid=2`) used directly by observations and survey status.
Native report batches, command transactions, and generalized receiver profiles
are deferred until their runtime consumers are introduced.

```sh
cmake -S test/GPS/Standalone -B build/gps-native-contracts -G Ninja \
  -DQGC_GPS_COMPONENTS=NativeContracts -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/gps-native-contracts
ctest --test-dir build/gps-native-contracts --output-on-failure
```

Source health, satellite state, and NMEA activity reuse the shared monotonic
receipt/deadline helper;
changing a timeout does not re-ingest a fix or revive retired satellite counts.
Satellite value headers can be consumed without position-policy headers.
`GPSAcceptedStateTest` exercises these contracts and survey-status provenance
in both application and standalone builds, using the shared test scheduler for expiry.

Position policy is explicit: the default retains the current ground-station
accuracy filtering, motion additionally rejects unreliable course, and Remote ID
can use measured ellipsoid altitude. GGA can use a valid raw receiver fix;
diagnostics can inspect invalid-fix coordinates without authorizing their use.
Every accepted source-health view still expires with its original receipt.
`GPSPositionPolicy` projects the whole observation, including altitude datum.
Consumers use this policy or the freshness-gated source-health API directly.
The position service preserves its source/session gates while exposing the selected
policy. Follow Me requests motion data; Remote ID requests its own altitude projection
and retains its region-specific requirements and fixed-location mode.

`VehicleGPSFactGroup` owns vehicle position and integrity Facts; GPS2 inherits that
storage. The vehicle groups retain MAVLink decoding, partial high-latency updates,
metadata, and the existing `GNSS_INTEGRITY` signal/UI path without applying
ground-station accuracy filters. Local positioning continues through the positioning
service rather than an unused parallel Fact projection. Separate relative/integrity
stores and presenters remain deferred with their producers and consumers.
Native-driver replacement and unified receiver lifecycle remain separate.

NTRIP vehicle inputs use receipt-stamped observations from the existing vehicle
owners. GPS coordinates and MSL altitude come from the same GPS report; fused
coordinates and altitude remain a separate source. Inputs without a valid fix
and finite MSL altitude, or with receipts at least five seconds old, are
unavailable to GGA. Communication loss clears those snapshots. Fresh repeated
coordinates refresh their receipt, but reading a cached observation does not.
Vehicle GGA eligibility does not require ground-station accuracy extensions;
raw MAVLink Fact display retains its existing partial-update behavior.

## Correction routing

`RTCM` owns framing, CRC validation, timestamped decoding, and MAVLink payload
fragmentation in `src/GPS/RTCM/`. Consumers use `RTCMFrameDecoder` for timestamped
results or `RTCMFramer` for Qt-free frame views; there is no separate parser facade.
`Corrections` owns source registrations,
selection, routing, the delivery ledger, and the event model in
`src/GPS/Corrections/`. Both expose isolated public-header checks and standalone
consumers; neither depends on the application, native receiver drivers,
recording, serial support, or Qt Positioning.

```sh
cmake -S test/GPS/Standalone -B build/gps-corrections -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation \
  -DQGC_GPS_COMPONENTS=Corrections
cmake --build build/gps-corrections
ctest --test-dir build/gps-corrections --output-on-failure
```

`QGC::GPSRTCMFramer` exposes the C++20 framing header without Qt. Its standalone
consumer and isolated header check compile without Qt include paths or libraries:

```sh
cmake -S test/GPS/Standalone -B build/gps-framer -G Ninja \
  -DQGC_GPS_COMPONENTS=RTCMFramer -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/gps-framer
ctest --test-dir build/gps-framer --output-on-failure
```

The aggregate `QGC::GPSRTCM` library remains Qt Core-backed. Corrections keeps
Qt Core public and RTCM private; consumers using RTCM APIs link them explicitly.

The application adapters live alongside their features in `src/GPS/Corrections/`
and `src/GPS/RTCM/`. `src/GPS/CMakeLists.txt` compiles them only into the application,
not the standalone libraries. They connect the existing base receiver, NTRIP client,
and UDP input to one shared MAVLink sequence domain.
The correction manager applies routing and UDP settings before enabling ingress;
`GPSManager` composes the producers and outputs without duplicating that wiring.
NTRIP configuration composes independent connection, RTCM-filter, and UDP-forward
values. The HTTP transport receives only connection/filter values; settings
conversion remains in `GPSManager`.
The HTTP boundary uses `QHttpHeaders` with bounded framing validation before
passing payload to RTCM decoding. Legacy ICY streams remain supported.
Server retry hints accept delta seconds or an HTTP date, are bounded to five
minutes, and feed the existing reconnect policy rather than a separate session
controller. Stopping or replacing an attempt retires its retry hint.
`NTRIPReentrancyTest` runs in the application harness, reusing its production
objects. The same cases can run independently through `test/GPS/NTRIP/Standalone`;
the application build does not create a second NTRIP executable.
Source registrations reject callbacks from retired sessions. UDP framing keeps
each sender separate and limits work per event-loop turn. Only UDP can opt out
of RTCM validation; other sources must submit validated frames.
NTRIP health retains actual receipt times across queued callbacks. Receive-only
telemetry replay links are excluded from correction outputs.

Automatic selection prefers a fresh local base, then NTRIP, then UDP. Manual
selection pins a category and optionally an endpoint; All sources forwards
every fresh stream. The existing NTRIP UDP output remains source-specific,
independent of the source selected for vehicles. UDP input settings retain their
existing `NTRIP` storage keys.

`acceptIngress()` returns whether the input was selected for global outputs, not
whether any output admitted it. A selected frame can be rejected by every output;
an unselected frame can still reach an independent source-specific output.

Diagnostics distinguish received, validated, selected, queued, written, dropped,
and unconfirmed bytes. `GPSCorrectionRouter::snapshot()` projects these diagnostics
into its `Snapshot` value using one clock sample, without duplicating ledger or
selector state. MAVLink and UDP output admission is not receiver
acknowledgement and does not prove an RTK fix. These application outputs report
queue admission only and do not accrue written-byte credit. Unconfirmed counters
track explicit uncertainty or reported-write destinations retired before
completion. Local receiver injection and its completion reports remain deferred
with the native receiver lifecycle.

Written-frame counts require complete admission and completion; partial writes
credit bytes only. Source queued-byte totals represent logical payload, while
submitted and terminal byte totals include output fanout.
Received and dropped bytes measure frame-candidate evidence, not raw transport
traffic. Recovered frames can overlap rejected candidates.
The retained `droppedFrames` field counts loss/rejection events, not unique
incomplete frames. Selection, admission, and terminal delivery can each record
a separate event for the same input. For example, rejecting part of a frame at
admission and losing more bytes at completion records two events; the lost-byte
total accumulates the separate portions. A global selection rejection can also
coexist with successful source-specific forwarding.

## Receiver transport compatibility

Transport results distinguish bytes admitted, confirmed by the local transport,
and uncertain; none of these counts is a receiver acknowledgement. Desktop serial and
TCP output queues are limited to 4 KiB. Cancellation is checked between waits
of at most 50 ms. A failed write that accepted data closes the connection and
discards queued output; reopening starts a fresh session. UDP sends one datagram
per write, rejects payloads above 65,507 bytes, filters input to the configured
peer, and retains unread datagram tails. Network serial bridges must already
use 115200 baud. Serial input exhaustion retires the stream because discarded
bytes can invalidate receiver framing; TCP applies backpressure instead.

The transport tests use loopback sockets and Linux pseudo-terminals. They cover
partial reads, peer closure, cancellation, deadlines, bounded buffering, and
write-result translation at the existing driver boundary. These checks do not
establish physical receiver or Android USB delivery.

Android serial retains the upstream Java/JNI and QSerialPort implementation.
Configuration writes use its synchronous API and backend timeout; cancellation
cannot interrupt an in-flight write. Failed writes conservatively report the
unconfirmed suffix as uncertain and close the connection. `writeBounded()` returns
`Unsupported` without sending data. Bounded Android writes, precise partial-write
accounting, and producer-buffer overflow detection are deferred until the Android
serial reliability work is introduced separately.

`GPSStreamTransportTest` exercises the private shared stream-write loop through
real TCP and desktop serial transports, including consecutive-write accounting,
expired deadlines, in-flight cancellation, and connection retirement. It runs in
the application suite and standalone `ReceiverTransports` selection.

On Linux, both suites also run `AndroidGPSCompatibilityTest`.
It compiles the bundled Android QSerialPort and GPS adapter against host Qt,
replacing only the JNI boundary. Coverage includes legacy write results,
unsupported bounded writes, cancellation, quiet receive timeouts, and Java/POSIX
DTR error classification. This executable does not link the desktop Qt SerialPort
library and does not establish physical Android USB delivery.

```sh
ctest --test-dir build --output-on-failure -R '^AndroidGPSCompatibilityTest$'
```
