# GPS library boundary checks

GPS value types live with their owners: receiver configuration, identity and native
reports in `Receiver/`, I/O statuses and results in `Transport/`, and altitude datums in `Core/`.
The shared `GPSConstellation.h` remains Qt-free and is exported by both Core and
the NMEA protocol target without a separate contracts library.
The Core library adds position and satellite observations, source health, and
survey status. It uses Qt Core, Qt Positioning, and the shared timing and logging
libraries, not receiver configuration or native drivers.
The QML registration header, `src/GPS/GPSQmlTypes.h`,
is compiled only by the application. The positioning service handles source registration,
selection, and recovery; QGC owns permissions and platform/custom/NMEA source
creation. The NMEA library owns passive sentence framing, Qt position decoding,
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

Receiver-configuration, PX4 conversion, accepted-state, and stream-transport behavior suites reuse
`PortableTest`: the application harness runs them against its production objects,
while standalone builds create narrow Qt-backed executables. The no-Qt consumers
remain small linkage/default/smoke checks rather than a second behavior-test framework.

`GPSReceiverConfigTest` owns the native validation/capability tables, including
wire limits and adjacent floating-point boundaries. `GPSBaseStationConfigTest`
covers the Qt diagnostic adapter. `GPSPx4DataTest` uses named cases, field-level
comparisons, and data rows for native enum mappings, motion validity and satellite
families. Their production libraries remain Qt-free.

To run those behavior suites without the application or native protocol dependency:

```sh
cmake -S test/GPS/Receiver -B build/gps-config-tests -G Ninja \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-config-tests
ctest --test-dir build/gps-config-tests --output-on-failure

cmake -S test/GPS/Driver -B build/gps-data-tests -G Ninja \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/installation
cmake --build build/gps-data-tests
ctest --test-dir build/gps-data-tests --output-on-failure
```

Native application test builds also register `CMake.GPSMinimal.DriverReports`,
`CMake.GPSMinimal.ReceiverConfig`, and `CMake.GPSMinimal.Px4Adapter` in the existing
Unit/CMake test lane. Each case configures a fresh temporary standalone build with
Qt discovery disabled, builds the default targets and isolated header checks, and
runs exactly the selected consumers. CMake's file API is used to reject runtime
targets and unrequested configuration artifacts. Single- and multi-configuration
generators use the active test configuration. Cross-compiled application builds
do not register these host-executed cases.
The compatibility case runs `QGCGPSPx4AdapterConsumer` plus the report consumer;
it does not link the Qt-backed `GPSPx4DataTest`.

```sh
ctest --test-dir build --output-on-failure -R '^CMake.GPSMinimal\.'
```

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

`GPSTransport.h` exposes the public interface using the statuses and results in
`Transport/GPSTransportResult.h`. These synchronous values do not require Qt
metatype registration. `GPSConnectionError` remains registered for queued worker
signals. Socket waiting is private to the TCP and UDP implementations.

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

The available components are `Core`, `NMEAProtocol`, `NMEAUtils`, `NMEA`, `Positioning`,
`Transport`, `ReceiverTransports`, `RTCMFramer`, `RTCM`, `Corrections`, `NTRIPHttp`,
`NTRIP`, `DriverReports`, `Px4Adapter`, and `ReceiverConfig`. The `Transport` library
needs Qt Core and the logging library, not Qt Network or RTK configuration.
Receiver transport tests additionally use Qt Test, not Qt Positioning.
Linux standalone builds leave the Android serial compatibility harness disabled.
Enable it with `-DQGC_BUILD_ANDROID_SERIAL_TESTS=ON` when Qt CorePrivate development
files are available. Full Linux application test builds retain that harness.
Core survey-status coverage stays with the Core component.
All components are enabled by default. `NMEA` includes `NMEAProtocol` and `NMEAUtils`;
`RTCM` and `Corrections` automatically include `RTCMFramer`. `NTRIP` includes
`NTRIPHttp`, `NMEAUtils`, and `RTCM`. `Px4Adapter` includes `DriverReports`.

## Owner-local GPS types

The existing PX4 driver remains in use. `QGC::GPSReceiverConfig` owns the Qt-free
configuration and software request capabilities. It shares `Receiver/CMakeLists.txt`
with the independent, header-only `QGC::GPSDriverReports` target. Target declarations
stay beside their sources instead of in build-only folders or per-target include
fragments. Standalone builds add this module with `EXCLUDE_FROM_ALL` and register
only requested consumers, so a report-only build does not compile configuration code.

| Directory | Ownership |
| --- | --- |
| `Receiver/` | Qt-free configuration, capabilities, identity, and native reports |
| `Driver/` | Driver facade and Qt configuration diagnostics |
| `Driver/Px4/` | Private compatibility conversion, layouts, and PX4 backend build wiring |
| `RTK/` | Application composition, worker integration, Facts, settings, and auto-connect |

The private `QGCGPSPx4Adapter` target is Qt-free; its regression executable opts
into the private `Driver/Px4` boundary. The driver facade explicitly calls
`qgc_add_px4_backend()` to attach Qt and the external PX4 protocol implementation.
Report, configuration and compatibility-only builds never invoke that runtime
setup or fetch the dependency. The wrapper's opaque state keeps PX4 layouts out
of its public sinks and the worker's queued signals. Its public header is checked
in isolation in application test builds.

The RTK provider and driver consume `GPSReceiverConfig`, with RTK base as the
unchanged default role and `GPSBaseStationConfig` as its base-only member.
Position mode is supported only for u-blox. Trimble/Ashtech, Septentrio and Femto
Position requests are rejected until their base-to-position transitions can be
implemented and verified; their RTK-base paths remain available. u-blox constellation
and position dynamic-model requests use the existing driver paths.
The wrapper's u-blox Normal mode disables heading output, so heading-offset
requests, including explicit zero, are not supported. Capabilities
describe software request support, not receiver-model discovery or read-back.
Unsupported role/setting combinations fail before configuration I/O. Output-rate
selection, configured NMEA output, settings/UI controls and general configuration
read-back remain with the native-driver/lifecycle work.

The pinned PX4 backend has content-hashed compatibility patches for receiver safety.
The u-blox patch protects Position requests. It disables TMODE3 or the modern time-mode configuration key
and requires both acknowledgement and checked TMODE3/VALGET read-back before
reporting configuration success. Explicit SBAS enable/disable requests also require
acknowledgement and matching RAM values, including L1CA when enabling SBAS.
An unresolved configuration ACK timeout fails closed rather than allowing a
delayed ACK to confirm a later required VALSET. Read-back validates message length,
checksum, key, layer and value; a generic ACK cannot substitute for it.
Capability exceptions come only from a checksum-valid, solicited MON-VER response;
known non-base and older identities avoid unsupported commands. Unidentified
firmware must confirm the change or fail configuration rather than claim
that an existing fixed/survey mode was cleared. No configuration wipe or dependency
pin update is used.

`GPSDriverTest` uses a stateful scripted receiver to retain base mode across new
driver instances. Legacy/modern fixed and survey transitions, rejected/missing
and delayed ACKs, rejected SBAS settings, read-back mismatches, cancellation,
non-base identities and corrupted version reports are covered.
These tests do not establish physical receiver acceptance; additional identity
mapping may be needed for unidentified non-base firmware.

The non-u-blox patch initializes legacy base settings, ignores Ashtech survey
progress/completion outside base mode, and rejects failure to enable Femto's
required position stream. Ashtech and Femto fixed-base reports have zero survey
duration instead of reading the inactive survey-settings union.
`GPSLegacySafetyTest` exercises those backend paths
directly, including modes that the facade currently rejects.

Shared Qt-free validation preserves the uint32 survey-duration range and existing
fixed-base float wire limits. `Driver/GPSReceiverConfigValidation` translates its
error codes through literal, `lupdate`-extractable contexts at the Qt-facing
boundary without introducing Qt into `Receiver/`.

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
when the position fix is valid. The compatibility adapter retains known zero/false diagnostics,
separates MSL and ellipsoid altitude, clamps legacy satellite arrays, and does not
invent per-satellite detail for count-only SBF reports or recover truncated
Ashtech azimuths. The PX4 state has no independent integrity receipt timestamps;
native integrity snapshots keep `timestampUs == 0` rather than borrowing a newer
position receipt. A clean RTCM report with unknown usage has no validity marker
in that state, so its CRC result remains unavailable. Receiver diagnostic evidence
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

The native report interface builds without finding Qt, compiling the private
adapter or fetching the PX4 driver:

```sh
cmake -S test/GPS/Standalone -B build/gps-reports -G Ninja \
    -DQGC_GPS_COMPONENTS=DriverReports -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/gps-reports
ctest --test-dir build/gps-reports --output-on-failure
```

To include the private compatibility bridge regressions and receiver
configuration without Qt or the PX4 protocol library:

```sh
cmake -S test/GPS/Standalone -B build/gps-native-data -G Ninja \
    '-DQGC_GPS_COMPONENTS=Px4Adapter;ReceiverConfig' \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build/gps-native-data
ctest --test-dir build/gps-native-data --output-on-failure
```

The NMEA protocol consumer covers shared constellation-ID normalization and
rejects accidental Qt dependencies:

```sh
cmake -S test/GPS/Standalone -B build/gps-nmea-native -G Ninja \
  -DQGC_GPS_COMPONENTS=NMEAProtocol -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
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
conversion and GGA setting subscriptions belong to `NTRIPManager`. `GPSManager`
injects application position providers before initializing the facade.
Source-table requests and cached results include the certificate policy in their
identity. Changing it also retires pooled TLS connections.
`QGC::GPSNTRIPHttp` exports connection/filter configuration and bounded HTTP
decoding with only Qt Core and Network. `QGC::GPSNTRIP` adds HTTP/TLS transport,
GGA providers, source-table fetching/model, and connection statistics. It links
RTCM, timing, rate tracking, Qt Positioning, and the existing NMEA formatting
helpers, without QML, serial discovery, native drivers, Bluetooth, or HttpServer.
`QGC::GPSNMEAUtils` owns the shared NMEA formatting implementation; it is not
compiled again inside NTRIP.

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

Transport results distinguish bytes admitted, confirmed by the local transport,
and uncertain. `uncertainBytes()` derives the outstanding suffix from accepted and
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
