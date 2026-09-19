# GPS library boundary checks

GPS value types live with their owners: receiver configuration, identity and native
reports in `Receiver/`, I/O statuses and results in `Transport/`, and altitude datums in `Core/`.
Qt Core is the baseline dependency; boundaries exclude unrelated networking,
positioning and application code rather than prohibiting Qt.
Compiled GPS libraries use `qt_add_library(... STATIC ...)` for Qt target
finalization. Alias and interface-only targets use CMake's `add_library`; autogen
settings remain explicit so value/parser-only libraries do not run moc unnecessarily.
The shared `GPSConstellation.h` remains Qt-free and is exported by both Core and
the NMEA protocol target without a separate contracts library.
The Core library adds position and satellite observations, source health, and
survey status. It uses Qt Core, Qt Positioning, and the shared timing and logging
libraries, not receiver configuration or native drivers.
The QML registration header, `src/GPS/GPSQmlTypes.h`,
is compiled only by the application. The positioning service handles source registration,
selection, and recovery; QGC owns permissions and platform/custom/NMEA source
creation. The NMEA positioning library owns passive sentence framing, Qt position decoding,
satellite assembly, and independent receipt-based freshness. Its input device is
borrowed; it does not write receiver configuration or depend on QGC settings.

CMake's `VERIFY_INTERFACE_HEADER_SETS` compiles each public header independently
using only its owner's public link interface. These checks are part of the
default build. GPS and utility tests share the header/consumer wiring in
`test/LibraryBoundaryChecks.cmake`, retaining their own labels, timeouts and
component-specific dependencies. Executable consumers verify accepted-position expiry, source
registration, and session ownership without linking QGroundControl. They use
`QGCTestTiming` for deterministic scheduling and also run in the application's
Unit suite.

Receiver-configuration, native conversion, accepted-state, and stream-transport behavior suites reuse
`PortableTest`: the application harness runs them against its production objects,
while standalone builds create narrow Qt-backed executables. The library consumers
remain small linkage/default/smoke checks rather than a second behavior-test framework.

`GPSReceiverConfigTest` owns the native validation/capability tables, including
wire limits and adjacent floating-point boundaries. `GPSBaseStationConfigTest`
covers the Qt diagnostic adapter. `GPSNativeDataTest` exercises the production
report adapter's enum mappings, motion validity, unavailable values and satellite
snapshots. Receiver configuration and report value types share a Qt-Core-backed
library; report metatype declarations belong to their defining headers, not the RTK worker.

Run receiver validation without the application or native protocol dependency:

```sh
cmake -S test/GPS/Receiver -B build/gps-config-tests -G Ninja \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-config-tests
ctest --test-dir build/gps-config-tests --output-on-failure
```

Run native facade/conversion tests and the optional hardware runner without the application:

```sh
cmake -S test/GPS/Driver -B build/gps-driver-tests -G Ninja \
    -DQGC_BUILD_GPS_HARDWARE_TESTS=ON \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-driver-tests
ctest --test-dir build/gps-driver-tests --output-on-failure -L Unit
```

Native application test builds also register `CMake.GPSMinimal.Receiver`,
`CMake.GPSMinimal.Transport`, `CMake.GPSMinimal.NMEAProtocol`,
`CMake.GPSMinimal.RTCM`, `CMake.GPSMinimal.MavlinkPacket`,
`CMake.GPSMinimal.Native`, and `CMake.GPSMinimal.Driver` in the existing
Unit/CMake test lane. Each case configures a fresh temporary standalone build,
builds the default targets and isolated header checks, and
runs exactly the selected consumers. CMake's file API is used to reject runtime
targets outside the selected component and unrequested receiver artifacts.
Every case permits Qt Core, with Qt Test for behavior suites, while disabling
Network, Positioning, SerialPort, and QML. The matrix rejects legacy PX4 runtime targets. Single- and multi-configuration
generators use the active test configuration. Cross-compiled application builds
do not register these host-executed cases.
The native case builds the production protocol libraries and checks their public
headers without concrete transports, positioning, QML or an external receiver runtime.
The NMEA and RTCM cases reject sibling GPS targets, including native receiver
drivers, even though their sources share the `Driver/Protocols/` directory.
The packetizer case likewise rejects sibling GPS targets, including RTCM decoding.

```sh
ctest --test-dir build --output-on-failure -R '^CMake.GPSMinimal\.'
```

The NMEA protocol is a Qt-Core-backed static library in
`src/GPS/Driver/Protocols/NMEA/`. Its consumer links only
`QGCGPSNMEAProtocol` and exercises sentence decoding, constellation resolution,
satellite assembly and explicit-time GGA formatting without a Qt application.

The default standalone build requires a C++20 compiler and Qt 6.8 or newer with Core and
Positioning:

```sh
cmake -S test/GPS/Standalone -B build/gps-libraries -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-libraries
ctest --test-dir build/gps-libraries --output-on-failure
```

`QGC::GPSTransport` owns the public interface, statuses from `Transport/GPSIOStatus.h`,
and Qt-backed details in `GPSTransportResult.h`. Native protocols use the same
`GPSReadResult` and `GPSWriteResult` rather than duplicate result types, preserving
transport error details and deriving uncertain bytes from shared progress evidence.
These synchronous values do not require Qt
metatype registration. `GPSConnectionError` remains registered for queued worker
signals. Socket waiting is private to the TCP and UDP implementations.

The transport libraries provide typed open/read/write results and independent
serial, TCP, and UDP receiver connections. They require Qt Core and Network, plus
SerialPort when serial support is enabled. The native driver's I/O adapter
preserves these results at the protocol boundary. TCP and UDP classes
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

The available components are `Core`, `NMEAProtocol`, `NMEAPositioning`, `Positioning`,
`Transport`, `ReceiverTransports`, `RTCM`, `MavlinkPacket`, `Corrections`, `NTRIPHttp`,
`NTRIP`, `Receiver`, `Native`, and `Driver`.
`Native` selects the Qt-Core-backed receiver protocols; `Driver` adds the transport
adapter and configuration diagnostics. The `Transport` library
needs Qt Core and the logging library, not Qt Network or RTK configuration.
Receiver transport tests additionally use Qt Test, not Qt Positioning.
Linux standalone builds leave the Android serial compatibility harness disabled.
Enable it with `-DQGC_BUILD_ANDROID_SERIAL_TESTS=ON` when Qt CorePrivate development
files are available. Full Linux application test builds retain that harness.
Core survey-status coverage stays with the Core component.
All components are enabled by default. `NMEAPositioning` includes `Core` and `NMEAProtocol`;
`Corrections` includes `RTCM`. `NTRIP` includes `NTRIPHttp`, `NMEAProtocol`, and `RTCM`.
Formatting belongs to `NMEAProtocol`; framing and timestamped decoding belong to `RTCM`.
`MavlinkPacket` selects only Qt Core-backed GPS_RTCM_DATA fragmentation and its
tests; neither `RTCM`, `Corrections`, nor `NTRIP` implicitly selects it.
`Driver` includes `Native` and `Transport`; `Native` includes `Receiver`, `Transport`,
`NMEAProtocol`, and `RTCM`. Obsolete header-only and adapter component selections
are removed rather than maintained as aliases.

```sh
cmake -S test/GPS/Standalone -B build/gps-native -G Ninja \
  -DQGC_GPS_COMPONENTS=Native -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-native
ctest --test-dir build/gps-native --output-on-failure

cmake -S test/GPS/Standalone -B build/gps-driver -G Ninja \
  -DQGC_GPS_COMPONENTS=Driver -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-driver
ctest --test-dir build/gps-driver --output-on-failure
```

## Owner-local GPS types

The production driver uses the native receiver protocols. `QGC::GPSReceiver` owns
configuration, software request capabilities, evidence, and public receiver reports,
including their Qt metatype declarations. Target declarations
stay beside their sources instead of in build-only folders or per-target include
fragments. Native contract/interface targets remain internal implementation boundaries;
they are not separately selectable library components.

| Directory | Ownership |
| --- | --- |
| `Receiver/` | Configuration, evidence, capabilities, identity, canonical enums, and Qt-integrated receiver reports |
| `Driver/` | Driver facade and Qt configuration diagnostics |
| `Driver/Protocols/` | Qt-Core-backed native receiver protocols and injectable configuration transactions |
| `Driver/Protocols/NMEA/` | Sentence parsing/formatting, constellation normalization, satellite epochs, and checksum handling |
| `Driver/Protocols/RTCM/` | Bounded framing/CRC, timestamped decoding, and decoded-frame values |
| `Positioning/` | Source selection, accepted-position policy, and application source creation |
| `Positioning/NMEA/` | Qt position/satellite adapters, stream handling, and NMEA source management |
| `Corrections/` | Correction source registration, routing, delivery accounting, and UDP ingress |
| `Corrections/MAVLink/` | Lightweight packetization plus application sequence/admission and vehicle-output adapters |
| `RTK/` | Application composition, worker integration, Facts, settings, and auto-connect |

Application and test targets use the native protocols only. The former PX4
backend, compatibility adapter, wire layouts and dependency patches have been
removed. `GPSNativeDataTest` retains relevant conversion coverage against the
production adapter. The facade retains owning
receiver reports in its public sinks and the worker's queued signals. Its public
header is checked in isolation in application and standalone driver builds.

The RTK provider and driver consume `GPSReceiverConfig`, with RTK base as the
unchanged default role and `GPSBaseStationConfig` as its base-only member.
Position mode is supported only for u-blox. Trimble/Ashtech, Septentrio and Femto
Position requests are rejected until their base-to-position transitions can be
implemented and verified; their RTK-base paths remain available. u-blox constellation
and position dynamic-model requests use the existing driver paths.
The supported u-blox role configuration does not expose heading output, so heading-offset
requests, including explicit zero, are not supported. Capabilities
describe software request support, not receiver-model discovery or read-back.
Unsupported role/setting combinations fail before configuration I/O. Output-rate
selection, configured NMEA output and settings/UI controls remain lifecycle work.

The native facade exposes `configure()`, typed `receiveOutcome()`, and owning
report sinks. Consumers use `receiveOutcome()`
to distinguish useful data, ancillary activity, idle time, cancellation and terminal
errors. Inactivity is measured by elapsed time since useful reports, not by counting
short ancillary receive calls.
`configurationError()` and `GPSReceiveResult::detail` retain transport diagnostics
without requiring callers to parse log output.
`configurationEvidence()` adds command outcomes without changing receiver
settings or exposing a new UI: accepted/written byte counts are distinct from
receiver acknowledgement and readback verification. The evidence values live with
the other receiver reports, not in the facade layer.
`writeConfiguration()` requires an explicit deadline and preserves the native
command budget on desktop transports; the generic `write()` wrapper is removed.
Android explicitly retains its synchronous backend limitation; generic unsupported
bounded writes are not retried through an unbounded writer.
Protocols retain injected clocks and absolute command deadlines for deterministic
tests. Conversion to a live Qt deadline happens only at the real transport boundary;
enabling Qt does not replace fake-time tests with wall-clock waits.

Scoped satellite updates are aggregated before publishing full snapshots. Used-only
counts have their own `GPSSatelliteUsageReport` and sink; they do not manufacture
anonymous satellites in view. Empty scoped reports still clear their own system.

The protocol libraries reuse checked little-endian reads and writes, shared NMEA
decoding and RTCM framing, and the existing GeographicLib dependency. Their raw
decoder state remains separate from the public receiver reports, with conversion
inside the driver. The application continues to consume the same report types.
Native state shares the public fix and integrity enums; protocol-specific partial
state, validity checks, altitude projection and snapshot aggregation remain distinct.
NMEA satellite-number normalization and numeric coordinate conversion live in the
NMEA protocol library, not the shared constellation enum or generic receiver base.
The native GGA report mapper is co-located under `Driver/Protocols/NMEA/` but remains
owned by `QGCGPSNativeCommon`; the generic NMEA library does not depend on native reports.
Native configuration entry points also validate base values before I/O. SBF survey
accuracy remains unavailable without actual survey evidence, and fixed-base survey
state is distinct from automatic position determination.
No receiver sessions, recording format, GCS position-source integration, or
settings migrations are introduced.

The [hardware runner](../Driver/Hardware/README.md) exercises the native facade
with the application's requests and transport observation. It is opt-in,
never opens hardware during CTest, requires explicit authorization for physical
configuration, and separates scripted results from physical evidence.

The native u-blox controller protects Position requests. It disables TMODE3 or the modern time-mode configuration key
and requires both acknowledgement and checked TMODE3/VALGET read-back before
reporting configuration success. Explicit SBAS enable/disable requests also require
acknowledgement and matching RAM values, including L1CA when enabling SBAS.
An unresolved configuration ACK timeout fails closed rather than allowing a
delayed ACK to confirm a later required VALSET. Read-back validates message length,
checksum, key, layer and value; a generic ACK cannot substitute for it.
Capability exceptions come only from a checksum-valid, solicited MON-VER response;
known non-base and older identities avoid unsupported commands. Unidentified
firmware must confirm the change or fail configuration rather than claim
that an existing fixed/survey mode was cleared. No factory reset or configuration
wipe is used.

`GPSDriverTest` uses a stateful scripted receiver to retain base mode across new
driver instances. Pre-v27/modern fixed and survey transitions, rejected/missing
and delayed ACKs, rejected SBAS settings, read-back mismatches, cancellation,
non-base identities and corrupted version reports are covered.
These tests do not establish physical receiver acceptance; additional identity
mapping may be needed for unidentified non-base firmware.

Native family tests cover valid fixed/survey reports, bounded Ashtech receipts,
required stream activation, and configuration failure paths. Older u-blox
firmware remains supported by native protocol code, not by a retained PX4 engine.

Shared receiver validation preserves the uint32 survey-duration range and existing
fixed-base float wire limits. `Driver/GPSReceiverConfigValidation` translates its
error codes through literal, `lupdate`-extractable contexts at the driver diagnostic
boundary, separate from the validation rules.

`gpsBaseStationConfigError()` checks the native base configuration and wire limits.
Fixed-base coordinates and altitude must be supplied explicitly; omitted fields
are rejected, while explicit zero values remain valid.
Receiver identity and manufacturer matching remain in the RTK connection path;
there is no generic contracts library or separate profile/discovery policy.
`GPSAltitudeDatum` defines the shared native datum values (`Unknown=0`,
`MeanSeaLevel=1`, `Ellipsoid=2`) used directly by observations and survey status.

Base-configuration behavior runs in the application harness. The same cases can
build independently from RTK, without fetching native drivers:

```sh
cmake -S test/GPS/RTK -B build/gps-rtk-config -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-rtk-config
ctest --test-dir build/gps-rtk-config --output-on-failure
```

Native reports default unavailable fields to NaN, `std::nullopt`, or an explicit
unknown enum. Invalid producer velocity leaves speed and course unavailable even
when the position fix is valid. Native conversion retains known zero/false diagnostics,
separates MSL and ellipsoid altitude, bounds satellite snapshots, and retains
independent integrity timestamps rather than borrowing a newer position receipt.
Count-only reports do not invent per-satellite detail. Receiver diagnostic evidence
is not correction-transport acknowledgement or proof of an RTK fix.

The driver lends RTCM bytes only for the synchronous sink call. The Qt worker
copies them before queueing and translates native survey values into the existing
`GPSSurveyInStatus`. Position and satellite callbacks carry owning native values.
`GPSProviderTest` exercises the production survey handler and a transport-backed
configuration callback. It covers coordinate validity and ordering, separate
ellipsoid altitude without assigning MSL coordinate altitude, optional accuracy,
the full survey-duration range, flags and queued snapshot ownership.
Existing source-health, observation, satellite stores and vehicle Facts are
unchanged; registering native positioning/diagnostic sources remains lifecycle work.

The receiver contract library builds with Qt Core, without compiling receiver
protocols or depending on positioning, concrete transports, or the application:

```sh
cmake -S test/GPS/Standalone -B build/gps-reports -G Ninja \
    -DQGC_GPS_COMPONENTS=Receiver -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-reports
ctest --test-dir build/gps-reports --output-on-failure
```

The NMEA protocol consumer covers shared constellation-ID normalization and
formatting while rejecting accidental positioning, network and QML dependencies:

```sh
cmake -S test/GPS/Standalone -B build/gps-nmea-native -G Ninja \
  -DQGC_GPS_COMPONENTS=NMEAProtocol -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-nmea-native
ctest --test-dir build/gps-nmea-native --output-on-failure
```

Source health, satellite state, and NMEA activity reuse the shared monotonic
receipt/deadline helper;
changing a timeout does not re-ingest a fix or revive retired satellite counts.
Future receipt timestamps remain rejected until a new observation arrives.
Satellite value headers can be consumed without position headers.
`GPSAcceptedStateTest` exercises these contracts and survey-status provenance
in both application and standalone builds, using the shared test scheduler for expiry.

Position policy is explicit: the default retains the current ground-station
accuracy filtering. Motion retains measured altitude and its reported uncertainty,
while rejecting unreliable course. The FOLLOW_TARGET sender requires finite
altitude; ArduPilot retains its separate home-altitude behavior. Remote ID can
use measured ellipsoid altitude. GGA can use a valid raw receiver fix.
Every accepted source-health view still expires with its original receipt.
`GPSObservation::projected()` applies these four policies, including altitude datum.
Consumers use the projection or the freshness-gated source-health API directly.
The position service preserves its source/session gates while exposing the selected
policy. Follow Me requests motion data; Remote ID requests its own altitude projection
with a five-second maximum age measured by the source's monotonic clock. It retains
its region-specific requirements and fixed-location mode.

Vehicle GPS FactGroups retain their existing MAVLink decoding, partial updates,
metadata, and `GNSS_INTEGRITY` signal/UI behavior. They do not store GCS
`GPSObservation` values or apply ground-station position policies.

NTRIP's Vehicle GPS source uses the existing GPS latitude/longitude Facts and
vehicle altitude; Vehicle EKF uses the vehicle coordinate. These inputs retain
the existing vehicle data semantics, without per-report receipt-age or
same-report altitude guarantees. Missing, offline-editing, or communication-lost
vehicles are excluded, and the encoder requires a valid coordinate and finite
altitude.
The GCS fallback uses a fresh GGA projection with finite MSL altitude,
not the accuracy-filtered map coordinate or a substituted zero altitude.

## Correction routing

The Qt-Core-backed RTCM library owns framing, CRC validation and timestamped decoding
in `src/GPS/Driver/Protocols/RTCM/`. Consumers use `RTCMFrameDecoder` for timestamped
`RTCMDecodedFrame` values or `RTCMFramer` for borrowed frame views; there is no separate
parser facade. Consumers of decoded values include `RTCMDecodedFrame.h` without
importing the decoder implementation or an old nested-type alias.
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

`QGC::GPSRTCM` exposes both framing and decoded values. Its standalone consumer
and isolated header checks require Qt Core, not concrete transports or positioning:

```sh
cmake -S test/GPS/Standalone -B build/gps-framer -G Ninja \
  -DQGC_GPS_COMPONENTS=RTCM -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-framer
ctest --test-dir build/gps-framer --output-on-failure
```

`QGC::GPSRTCM` remains Qt Core-backed, and Corrections publicly exposes its decoded
frame contract. `QGC::GPSMavlinkPacket` separately owns GPS_RTCM_DATA fragmentation
in `Corrections/MAVLink/`, without RTCM decoding, vehicle/link objects or network
dependencies. Its standalone consumer and `RTCMMavlinkPacketTest` cover packetization
independently of the frame-decoder conformance suite.

The UDP ingress adapter lives in `src/GPS/Corrections/`; MAVLink sequence/admission
accounting and vehicle/link adapters live in `src/GPS/Corrections/MAVLink/`.
`src/GPS/CMakeLists.txt` compiles these adapters only into the application,
not the standalone libraries. They connect the existing base receiver, NTRIP client,
and UDP input to one shared MAVLink sequence domain.
The correction manager applies routing and UDP settings before enabling ingress;
`GPSManager` composes the producers and outputs without duplicating that wiring.
NTRIP configuration composes independent connection, RTCM-filter, and UDP-forward
values. The HTTP transport receives only connection/filter values; settings
conversion and GGA setting subscriptions belong to `NTRIPManager`. `GPSManager`
injects application position providers before initializing the facade.
Source-table requests and cached results include the certificate policy in their
identity. Changing it also retires pooled TLS connections.
`QGC::GPSNTRIPHttp` exports connection/filter configuration, request serialization
and bounded HTTP decoding with only Qt Core and Network. `NTRIPHttpRequest` shares
the existing authentication utility without depending on sockets, RTCM or positioning.
`QGC::GPSNTRIP` adds HTTP/TLS transport,
GGA providers, source-table fetching/model, and connection statistics. It links
RTCM, timing, rate tracking, Qt Positioning, and the existing NMEA formatting
helpers, without QML, serial discovery, native drivers, Bluetooth, or HttpServer.
`QGC::GPSNMEAProtocol` owns the shared NMEA formatting implementation; it is not
compiled again inside NTRIP. GGA formatting accepts explicit fix fields and UTC time;
coordinate-source selection, altitude policy, and obtaining the current time stay
with the caller, outside the protocol library.

The application and standalone suites link these same production targets.
`NTRIPManager`, settings conversion, and QML registration stay in the application;
the existing QML type names and properties are preserved through foreign-type
registrations. Public headers compile in isolation and executable consumers
link only their owning targets. Shared HTTP authentication and proxy setup
reside in `QGC::NetworkClient`, re-exported by `QGCNetworkHelper.h` for existing
application callers.
Application-only `NTRIPQmlMetadata` and `NTRIPQmlLint` checks cover the generated
exports, properties and controller enum. They use the library's exported moc
metadata without introducing QML dependencies into standalone NTRIP builds.

`NTRIPReentrancyTest` and `NTRIPTlsTest` run against these targets in both the
application harness and standalone executables. Loopback TLS cases cover
encrypted identity/chunked correction delivery, self-signed rejection and
explicit opt-in, hostname mismatch despite opt-in, stop/deletion during the
handshake and before the HTTP response, and retirement across restart/reconnect.
They use test-only certificates, ephemeral ports, and no external caster.

The NTRIP HTTP decoder handles close-delimited, content-length, and chunked
responses, including legacy ICY streams. Header bytes, line lengths, header
counts, chunk sizes, and socket buffering are bounded. Only decoded correction
payloads reach the RTCM decoder; valid payload prefixes retain their receipt
timestamps if later framing fails.
Legacy ICY input probes bounded optional ASCII headers and lets partial-frame
suffixes reach RTCM resynchronization. A binary body can follow optional ICY
headers without a blank separator.
HTTP failures carry numeric or HTTP-date `Retry-After` hints through queued
callbacks. Reconnects use the greater of the existing exponential backoff and
the hint, capped at five minutes, without making authentication or configuration
errors retryable.
Error status and retry hints survive unsupported body encodings and interrupted
diagnostic bodies. Non-authentication error responses may contribute up to
500 body bytes to a sanitized preview of at most 200 characters. Collection
ends on completion, the byte cap, or a non-resetting 250 ms deadline. Error
bodies never reach the RTCM decoder; compressed diagnostic bodies are not
decoded, and authentication failures are reported immediately.
Outgoing streaming and source-table headers use `QHttpHeaders` validation.
Streaming requests retain canonical field-name spelling for legacy casters
that compare names case-sensitively. Mountpoint and credential value case is
preserved. Invalid configuration is reported without admitting a request or
warning that credentials were sent.

```sh
cmake -S test/GPS/NTRIP/Standalone -B build/ntrip-http -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/ntrip-http
ctest --test-dir build/ntrip-http --output-on-failure -L Unit
```

The same NTRIP suites are also included by `QGC_GPS_COMPONENTS=NTRIP`.
The `NTRIPHttp` component builds only the decoder/configuration library and
its consumer, without Qt Positioning or Qt Test:

```sh
cmake -S test/GPS/Standalone -B build/ntrip-framing-minimal -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/installation \
  -DQGC_GPS_COMPONENTS=NTRIPHttp -DCMAKE_DISABLE_FIND_PACKAGE_Qt6Positioning=ON
cmake --build build/ntrip-framing-minimal
ctest --test-dir build/ntrip-framing-minimal --output-on-failure
```

The [HTTP decoder fuzz entry point](../../Fuzz/NTRIPHttpDecoder/README.md) adds
deterministic seed smoke coverage and optional Clang libFuzzer/ASan/UBSan runs
against the same production target. Its compatibility guide also covers local
Qt 6.8 and Windows standalone builds.

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
and unconfirmed bytes. Narrow projections avoid rebuilding every source and
destination for an individual property or event refresh, without duplicating ledger
or selector state. MAVLink and UDP output admission is not receiver
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

Configuration commands use `writeConfiguration()` with an explicit command deadline;
runtime corrections use `writeBounded()`. Transport results distinguish bytes
admitted, confirmed by the local transport, and uncertain. `uncertainBytes()` derives
the outstanding suffix from accepted and
written evidence; none of these counts is a receiver acknowledgement. Correction
ledger uncertainty remains explicit and separate from this transport calculation.
Desktop serial and TCP output queues are limited to 4 KiB.
Cancellation is checked between waits
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
