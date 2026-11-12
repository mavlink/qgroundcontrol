# GPS

`src/GPS` connects GNSS receivers to the ground station and routes RTK corrections to vehicles.

- **Receivers.** The receiver of the GNSS Receiver settings is configured as an RTK base station (role
  "RTK base station") or only read (role "Not configured": an existing base station, or any receiver that reports
  a position). A second one, the position receiver, is never configured and only tells where the ground station is.
  A receiver connects over a serial device, TCP or Bluetooth, and one that is only read also over UDP.
- **Corrections.** RTCM 3 frames come from the local base station, an NTRIP caster or a UDP port. One stream is
  selected and sent to vehicles as MAVLink `GPS_RTCM_DATA`, and on request to a UDP address and a log file.
- **Ground station position.** One position, from a receiver or from this device's own position source, for the
  maps, Follow Me, set-home, Remote ID and the GGA sentence a caster is sent.

The user guide describes this as the user sees it, and is the reference for the names the user interface shows:
[GNSS Receiver](../../docs/en/qgc-user-guide/settings_view/gnss_receiver.md) and
[RTK Corrections](../../docs/en/qgc-user-guide/settings_view/ntrip_rtk.md). Paths below are relative to `src/GPS`
unless they start with `src/`, `test/` or `docs/`.

## Layers

```text
application    Vehicle, FollowMe, RemoteIDManager; QML through QGroundControl.gpsManager and .positionManager
               ----------------------------------------------------------------------------------------------
src/GPS        GPSManager, GPSSettingsBindings                creates the services, hands them the settings
               Receiver                 NTRIP                 the two services that hold a connection
               Corrections  Positioning  Connection  NTRIP/HTTP *  what they feed; when a connection is retried;
                                                              one HTTP exchange with a caster
src/GPS/Core * GPSReceiverWorker                              one receiver session, on a thread of its own
               Protocols                                      GPSDriver, its framework, the receiver families
               NMEA    RTCM    Transport                      wire formats and byte links
               Common                                         types, requests, reports
```

`*` builds without the application. A directory includes headers only of directories on lines below its own:

| Directory | Owns | Includes, within `src/GPS` |
| --- | --- | --- |
| `Core/Common` | `GPSType`, the request (`GPSReceiverConfig`), the reports (`GPSReceiverReports.h`), the event a session reports (`GPSReceiverEvent`), the family descriptors, the detection hint, clock and math | Nothing |
| `Core/NMEA`, `Core/RTCM`, `Core/Transport` | NMEA sentences and epochs; RTCM framing, station position and stream content; `GPSTransport` with the serial, TCP, UDP, pipe and recording transports | `Core/Common` |
| `Core/Protocols` | `GPSDriver`, `GPSProtocolRuntime`, `GPSCommandChannel`, `GPSStreamDemux`, `GPSReceiverDetector` and a folder for each family | The three above, `Core/Common` |
| `Core` | `GPSReceiverWorker` | `Core/Protocols`, `Core/Transport`, `Core/Common` |
| `NTRIP/HTTP` | `NTRIPHttpSession`, the request and response codec, `NTRIPConnectionConfig` | Nothing |
| `Connection` | `GPSConnectionSupervisor` | Nothing |
| `Positioning` | `GPSObservation`, `GPSSourceHealth`, `PositionManager` | `Core/Common` |
| `Corrections` | `GPSCorrectionManager`, `GPSCorrectionSelector`, `RTCMMAVLink`, UDP input, the log, the loss alert | `Core/Common`, `Core/RTCM` |
| `NTRIP` | `NTRIPManager`, its transport, `NTRIPGgaReporter`, the source table | `NTRIP/HTTP`, `Connection`, `Corrections`, `Positioning`, `Core/Common`, `Core/NMEA`, `Core/RTCM` |
| `Receiver` | `GPSReceiver`, its session and connection policy, its serial and Bluetooth sides, status Facts, wording | `Corrections`, `Positioning`, `Core`, `Core/Transport`, `Core/Common` |
| `src/GPS` itself | `GPSManager`, `GPSSettingsBindings` | `Receiver`, `NTRIP`, `Corrections`, `Positioning` |
| `Qml` | The QML module `QGroundControl.GPS` | `Core/Common`, in `GPSQmlTypes.h` |

GPS code outside `src/GPS`:

| Where | What |
| --- | --- |
| `src/Settings` | `RTKSettings`, `PositionReceiverSettings`, `NTRIPSettings` and `GPSCorrectionSettings`, each with its Facts in a JSON file such as `src/Settings/RTK.SettingsGroup.json`. They include nothing of `src/GPS`; `Receiver` and `Corrections` include them for the persisted enums |
| `src/Toolbar/GPSIndicator.qml`, `src/Toolbar/GPSIndicatorPage.qml` | The toolbar indicator and its drop-down |
| `src/AppSettings/pages/GNSSReceiver.SettingsUI.json`, `src/AppSettings/pages/RTKCorrections.SettingsUI.json` | The two settings pages, as lists of components of `Qml` |
| `src/Vehicle/FactGroups/VehicleGPSFactGroup.h` | The vehicle's own GPS as Facts, which `Corrections` and `NTRIP` read. The ground receiver's are `Receiver/GPSReceiverFactGroup.h` |
| `src/Vehicle/Vehicle.cc`, `src/FollowMe/FollowMe.cc`, `src/Vehicle/RemoteIDManager.cc` | Readers of the ground station position, through `GPSManager::positionManager()` |

Two rules hold this together:

- **`Core` and `NTRIP/HTTP` are free of the application.** They are the libraries `QGCGPSCore` and `QGCNTRIPHttp`: no
  settings, Facts, QML or `Vehicle`, only Qt without QML and utility libraries. `test/GPS/Standalone` builds them on
  their own, so an application header there breaks that build. The `Core/Protocols` headers are private to `QGCGPSCore`.
- **The include directions have no cycle.** No include goes against the table, and no header of another directory
  includes a `Core/Protocols` header. Inside `Core/Protocols`, the shared code includes family headers in one file:
  `GPSStreamDemux.cc` takes the framers of `SBFBlocks.h` and `UBXFrame.h`.

## Glossary

Quoted names are what the settings show today; the user guide is the reference for them.

- **Receiver**: a `GPSReceiver` service. `GPSManager::receiver()` is the receiver of the GNSS Receiver settings,
  `GPSManager::positionReceiver()` the position receiver, which is always passive.
- **Family**: the receivers one protocol configures, a `GPSType`; "Receiver manufacturer" in the settings.
- **Driver, protocol, plan**: `GPSDriver` runs a family's protocol class (`GPSFamilyProtocol`), which sends the
  commands of its plan through a `GPSCommandChannel`.
- **Session**: one connection's lifetime: a `GPSReceiverSession` with its `GPSReceiverWorker` thread for a
  receiver, an `NTRIPHttpSession` for one exchange with a caster. The rest of `src/GPS` runs on the main thread.
- **Connection supervisor**: a `GPSConnectionSupervisor`, the value that says for a connection that is kept up
  when its next attempt opens, how long a retry waits, what it does behind a gate such as a missing network, and
  when it gives up. Its owner opens the attempts and times the delays: `NTRIPManager`, for the caster connection.
- **Base state**: `GPSBaseState`: `Inactive`, `Active` while a survey or averaging runs, `Valid` once the base
  position holds. Base modes are "Survey-In", "Fixed position" and "Receiver-managed averaging".
- **Correction source**: where RTCM comes from, a `GPSCorrectionSettings::CorrectionSource`: `LocalReceiver`
  ("GNSS receiver"), `Ntrip` or `Udp`. A source registers with `GPSCorrectionManager::openSource()`.
- **Stream**: the frames of one source (`GPSCorrectionStream`). `GPSCorrectionSelector` selects the one stream that
  vehicles receive: a fresh one, the GNSS receiver before NTRIP before UDP, unless a source is chosen.
- **Caster**: an NTRIP server ("NTRIP Caster"). **Mountpoint**: one of its streams ("Mountpoint"), listed in its
  source table.
- **GGA source**: the position `NTRIPGgaReporter` reports to the caster ("GGA Position Source"): the vehicle's GPS
  or estimate, the receiver, or the ground station position.
- **Ground station position**: what `PositionManager` publishes as `gcsPosition` ("Ground Station Position"), from
  a receiver or this device.
- **Observation**: a `GPSObservation`, a position with its receipt time, fix quality and altitude datum, whatever
  produced it. A `GPSSourceHealth` keeps a source's latest one and says whether it is still usable.
- **Golden transcript**: the text of what the driver sent and published in one scenario (`GPSGoldenTranscriptTest`).
- **Scripted receiver**: `ScriptedReceiver`, a `GPSTransport` of the tests whose model answers as a family's receiver.

## Receiver families

| Vendor | `GPSType` | Folder, also namespace and log category | Goldens | Base modes | Identity it reads | Baud rate when none is selected | Base state from |
| --- | --- | --- | --- | --- | --- | --- | --- |
| u-blox | `UBlox` | `UBX/`, `GPS.Protocols.UBX` | `ublox/` | Survey-in, fixed | Model, firmware | Searches 7 rates | `UBX-NAV-SVIN` in a survey; the driver for a fixed base |
| Trimble MB-Two | `Trimble` | `Ashtech/`, `GPS.Protocols.Ashtech` | `trimble/` | Survey-in, fixed | Board | 115200; after detection its 5 rates, the detected one first | `$PASHR,RECEIPT,POS,AVG` in a survey; the driver for a fixed base |
| Septentrio | `Septentrio` | `SBF/`, `GPS.Protocols.SBF` | `septentrio/` | Survey-in, fixed | None | 115200, or the detected rate | Every `PVTGeodetic` block |
| Femtomes | `Femto` | `Femto/`, `GPS.Protocols.Femto` | `femto/` | Survey-in, fixed | None | 115200, or the detected rate | A GGA of quality 7 ends the survey; the driver for a fixed base |
| Unicore UM980/UM982 | `Unicore` | `Unicore/`, `GPS.Protocols.Unicore` | `unicore/` | Receiver averaging, fixed | Model, firmware | Searches 8 rates | `BESTNAVXYZA`, every second |
| Quectel LG290P | `Quectel` | `Quectel/`, `GPS.Protocols.Quectel` | `quectel/` | Survey-in, fixed | Firmware | Searches 6 rates | `PQTMSVINSTATUS` |
| Any, read only | `Passive` | `Passive/`, `GPS.Protocols.Passive` | `passive/` | None | The rate it was found at | Listens at 9 rates, never writes | None |

- Folders are in `Core/Protocols`, goldens in `test/GPS/Core/Protocols/golden`. Three families have a folder named
  for their protocol and not their vendor: Trimble in `Ashtech/`, Septentrio in `SBF/`, u-blox in `UBX/`. The test
  folder, the receiver model and the scenario file follow the folder; the golden directory is the `GPSType` in
  lower case, and so is the `--family` of the hardware runner.
- What a family supports is its `GPSReceiverDescriptor` (`Core/Common/GPSReceiverDescriptor.cc`). Its `FAMILY`
  record, a `GPSReceiverFamily`, ends its protocol source: `UBX/UBXConfigurator.cc` for u-blox.
- `GPSType::Automatic` ("Detect automatically") is no family: `GPSReceiverDetector` listens, then asks each family with
  its read-only `probe()`, and the family that answers is configured. Its goldens are in `automatic/`.
- A configured base's RTCM is forwarded only while its base state is `Valid`. Septentrio and passive input are
  exempt (`forwardsCorrectionsWithoutBase()`).
- [Core/Protocols/ORIGIN.md](Core/Protocols/ORIGIN.md) records where each driver came from.

## The path of data

A position, from a receiver's bytes to the ground station position:

1. `GPSReceiverWorker`, on its thread, calls `GPSDriver::receiveOutcome()`, and
   `GPSCommandChannel::receiveCycle()` reads the `GPSTransport`.
2. `GPSStreamDemux` frames the bytes. The family's protocol decodes a frame in `onFrame()` and publishes a
   `GPSDecodedPosition` through its `GPSDecodeContext`.
3. `GPSDriver` adapts it to a `GPSPositionReport` for `GPSDriverSinks::onPosition`, and the worker reports it as a
   `GPSReceiverEvent` with its one signal, `reported()`, queued to the main thread.
4. `GPSReceiverSession` takes what it keeps of an event and passes it on to `GPSReceiver`, which visits it: for a
   position it writes the status Facts (`GPSReceiverFactGroup`) and gives its `GPSSourceHealth` a `GPSObservation`.
   A base that knows its antenna position reports that one.
5. `PositionManager` chooses between the receiver source and this device's source and publishes `gcsPosition`.
6. Readers bind to it or ask `PositionManager::acceptedObservation()` for the use they have.

An RTCM frame, from its source to a vehicle link:

1. From a caster: `NTRIPHttpSession` reads the socket and `NTRIPHttpDecoder` removes the HTTP framing.
   `NTRIPHttpTransport` cuts frames with an `RTCMFrameDecoder` and emits `correctionFrameReceived()`, queued to
   `NTRIPManager`.
2. From the local base: steps 1 and 2 above, where the family calls `publishRTCM()`. `GPSDriver` drops the frame unless
   the base state allows it, the worker reports it as a `GPSReceiverRTCM` event with the time it was read, and
   `GPSReceiverSession` forwards it.
3. Either source calls `GPSCorrectionSourceHandle::submit()`. `GPSCorrectionManager` applies the message filter,
   and `GPSCorrectionSelector` passes frames of the selected stream while they are fresh.
4. `RTCMMAVLink` packs the frame into `GPS_RTCM_DATA` fragments of 180 bytes (`RTCMMAVLinkPacket`) and sends them
   on the primary link of each selected vehicle. The UDP output and the log get the same frame.

## Adding a receiver family

Femtomes is the smallest family, 3 files in `Core/Protocols/Femto`: follow it with `git grep -i femto`. In `src`:

1. `Core/Common/GPSType.h`: the type, before `Automatic`, which stays last.
2. `Core/Common/GPSReceiverDescriptor.cc`: its descriptor, with a `manufacturerId` of its own. A `static_assert`
   fails the build until every type has one.
3. `Core/Protocols/<Family>/`: `<Family>Plan.h` and `.cc` hold the commands, rates and timeouts as data.
   `<Family>Protocol.cc` holds the protocol class, its logging category, the `signature()` that recognises its
   traffic and its `FAMILY`. List them in `Core/Protocols/CMakeLists.txt`.
4. `Core/Protocols/GPSReceiverFamilies.h` and `Core/Protocols/GPSReceiverFamilies.cc`: declare the category and
   `FAMILY`, and add it to the table, whose order is the order of lookup and of detection.
5. `src/Settings/RTK.SettingsGroup.json`: the manufacturer in `baseReceiverManufacturer`. The form follows the
   descriptor through `GPSReceiverPresentation`.
6. `Receiver/GPSReceiverMessages.cc`: the `switch` over the types in `inputProblem()`. For USB discovery, the
   receiver's USB identity in `src/Comms/Serial/USBBoardInfo.json`.
7. The user guide and the table of [ORIGIN.md](Core/Protocols/ORIGIN.md).

In `test/GPS/Core/Protocols`, where the list is long:

1. `Support/<Family>ReceiverModel.h`, the scripted receiver's model, listed in `Support/CMakeLists.txt`, and its
   `Dialect` in `Support/DetectionReceiver.h`.
2. `<Family>/<Family>ProtocolTest.h` and `.cc`, listed in `test/GPS/Core/CMakeLists.txt`.
3. `Scenarios/GoldenScenarios<Family>.cc`: listed in `Support/CMakeLists.txt`, declared in
   `Scenarios/GoldenScenarios.h`, called in `Scenarios/GoldenScenarios.cc`. Detection scenarios are in
   `Scenarios/GoldenScenariosAutomatic.cc`.
4. Goldens: regenerating them writes the family's directory. A sample of its output in `corpus/`, with a row in
   `decoders()` and a rule in `decodersFor()` of `Scenarios/GoldenChecks.cc`, adds a decode golden.
5. Fuzz seeds: that sample seeds the protocol fuzzer, and `seeds()` in `Fuzz/ReplyFuzzSmoke.cc` records a session
   of the model for the reply fuzzer. `Fuzz/ProtocolFuzz.cc` takes every family from `gpsReceiverFamilies()`.
6. The family tables of the shared suites: `GPSProtocolFamilyContractTest.cc` has two, `GPSReceiverDetectorTest.cc`
   a `switch`, and `GPSReceiverConfigTest.cc`, `GPSRecordingDialogueTest.cc`, `GPSReceiverPresentationTest.cc`,
   `GPSReceiverTest.cc` and `GPSReceiverSettingsTest.cc` rows of their own.
7. The runner: `--family` in `Hardware/GPSHardwareRunner.cc`, `Hardware/check_runner.py`, and a row of the
   qualification table in `fixtures/README.md`.

No table lists the families once: `git grep -l 'GPSType::Femto' -- test` names 12 files today, and the models, `Dialect`
and the CMake lists are not among them. Grep for an existing family before trusting this list.

## Testing

`test/GPS` mirrors `src/GPS`. Every test there has the CTest label `GPS`.

| Layer | Where, under `test/GPS` | Runs on |
| --- | --- | --- |
| Parsers and values | `Core/Common`, `Core/NMEA`, `Core/RTCM`, `NTRIPHttpCodecTest` | Bytes in, values out |
| Protocol suites | `Core/Protocols/<Family>` and the suites over all families, such as `GPSProtocolFamilyContractTest` | A scripted receiver and a `GPSTestClock`: no real time passes |
| Golden transcripts | `GPSGoldenTranscriptTest`, with `Core/Protocols/Scenarios` and `Core/Protocols/golden` | The same |
| Standalone build | `Standalone` | `Core` and `NTRIP/HTTP` without the application: goldens, runner, fuzzer smoke tests |
| Fuzzers | `Core/Protocols/Fuzz`, `NTRIP/HTTP/Fuzz` | libFuzzer, in the standalone build only; smoke tests also in the application's |
| Runner and simulator | `Core/Protocols/Hardware` | `QGCGPSHardwareRunner` configures a receiver, replays a recording or serves a simulated receiver |
| Services | `Receiver`, `Corrections`, `NTRIP`, `Positioning` | `ManualScheduler`, `ScriptedReceiverWorker`, `MockNTRIPTransport`, `ScriptedNTRIPCaster` |
| Panels | `Qml` | The QML components, created by C++ suites |

```bash
LABELS=GPS just test                                                        # every GPS test
ctest --test-dir build -R FemtoProtocolTest --output-on-failure             # one suite
QGC_GPS_GOLDEN_UPDATE=1 ctest --test-dir build -R GPSGoldenTranscriptTest   # rewrite the goldens, then read the diff
cmake -S test/GPS/Standalone -B build-gps -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=clang++ \
    -DQGC_GPS_LIBFUZZER=ON
cmake --build build-gps && ctest --test-dir build-gps
```

[test/GPS/README.md](../../test/GPS/README.md) says which kind of test a change needs, and describes the test
doubles and the tests of a receiver family.
[test/README.md](../../test/README.md#gps-protocol-golden-transcripts) describes the goldens, the standalone build
and the fuzzers. The [fixtures README](../../test/GPS/Core/Protocols/fixtures/README.md) describes receiver
recordings, their replay and the hardware runner.

## Settings and the user interface

| Class in `src/Settings` | Stored group | Holds |
| --- | --- | --- |
| `RTKSettings` | `RTK` | The GNSS receiver: role, manufacturer, connection, base mode and position, saved base sites. Also `gcsPositionSource` |
| `PositionReceiverSettings` | `PositionReceiver` | The position receiver: `enabled` and its connection |
| `NTRIPSettings` | `NTRIP` | The caster and mountpoint, the GGA source, saved casters and failover |
| `GPSCorrectionSettings` | `NTRIP` as well | The correction source, the vehicles, the RTCM message filter (`ntripWhitelist`), UDP input and output |

No service reads a setting. `GPSSettingsBindings` builds a plain `Configuration` value for each service, applies
it now and whenever a Fact it read changes, and stores what `NTRIPManager` asks to keep. Tests build those values
without settings. Two classes touch settings besides it: `GPSManager`, for the save folder, a surveyed base
position and the receivers detection found, and `GPSFixedBaseEcef`, the form's editor of ECEF coordinates.

The QML is in `Qml`, and [Qml/CMakeLists.txt](Qml/CMakeLists.txt) lists it. By role:

- **Status panels**: `GPSReceiverStatus.qml`, `CorrectionsStatus.qml`, `NTRIPConnectionStatus.qml` and
  `GcsPositionStatus.qml`. The settings pages and the toolbar drop-down show the same four.
- **Settings groups**: the sections of the two settings pages, which the page files above name.
- **The toolbar drop-down**: the vehicle's GPS and the status panels with their Connect buttons, and a button that
  opens each settings page. It holds no settings.

QML reaches the services through `QGroundControl.gpsManager` and `QGroundControl.positionManager`. Most panels
take their service as a property, which the panel suites replace.

## Conventions that bite

- **No application types in `Core` or `NTRIP/HTTP`.** Logic that can live there is tested there.
- **Unity builds.** CI compiles `src` in unity batches ([CODING_STYLE.md](../../CODING_STYLE.md#common-pitfalls)): a
  `.cc` has no file-scope `using namespace` of a project namespace and no file-local name that repeats in another file.
  A family's file-local code therefore stays inside its namespace: most have a `Protocol` and a `signature()`.
- **Goldens are evidence.** A refactor changes none. A change of behaviour regenerates them, and the commit body
  explains every golden that changed.
- **The test log check is strict.** A warning that a test triggers fails it, unless the test expects it with
  `expectLogMessage()`. Protocol suites read `GPS.Protocols.*` warnings through a `GPSProtocolLogCapture`.
- **No invented receiver behaviour.** No run on a receiver is recorded for any family
  ([hardware qualification](../../test/GPS/Core/Protocols/fixtures/README.md#hardware-qualification)). A command
  comes from the vendor's documentation, and the commit body says that no receiver verified it.
