# GPS library boundary checks

The Core library contains RTK configuration, connection types, position
observations, and source health. It uses Qt Core, Qt Positioning, and the shared
timing and logging libraries. The QML registration header, `src/GPS/GPSQmlTypes.h`,
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

`GPSTransport.h` contains the public interface, status enums, and result types.
Socket waiting is a protected GPSTransport method shared by TCP and UDP implementations.

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

The available components are `Core`, `NMEA`, `Positioning`, `Transport`,
`ReceiverTransports`, `RTCMFramer`, `RTCM`, and `Corrections`. `Transport` alone needs Qt Core,
Qt Network, and the logging library.
All components are enabled by default. `RTCM` and `Corrections` automatically include `RTCMFramer`.

## Correction routing

`RTCM` owns framing, CRC validation, timestamped decoding, and MAVLink payload
fragmentation in `src/GPS/RTCM/`. `Corrections` owns source registrations,
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
and unconfirmed bytes. MAVLink and UDP output admission is not receiver
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

On Linux, the application's Unit suite also runs `AndroidGPSCompatibilityTest`.
It compiles the bundled Android QSerialPort and GPS adapter against host Qt,
replacing only the JNI boundary. Coverage includes legacy write results,
unsupported bounded writes, cancellation, quiet receive timeouts, and Java/POSIX
DTR error classification. This executable does not link the desktop Qt SerialPort
library and does not establish physical Android USB delivery.

```sh
ctest --test-dir build --output-on-failure -R '^AndroidGPSCompatibilityTest$'
```
