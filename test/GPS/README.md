# GPS tests

`test/GPS` tests [`src/GPS`](../../src/GPS/README.md), whose README describes the subsystem, its layers and its
terms. This file is for a developer who has a GPS change to test and must choose among the kinds of test here.
[test/README.md](../README.md) describes the framework they share. Paths below are relative to `test/GPS` unless
they start with `src/`, `test/`, `cmake/`, `.github/` or `build`.

## The kinds of test

| Kind | Where | What it proves | What a new case costs |
| --- | --- | --- | --- |
| Parser and value suites | `Core/Common`, `Core/NMEA`, `Core/RTCM`, `NTRIP/HTTP/NTRIPHttpCodecTest.cc`, `NTRIP/NTRIPSourceTableTest.cc` | Bytes in, values out, without a link, a clock or a thread | A data row |
| Protocol suites | `Core/Protocols/<Family>/<Family>ProtocolTest.cc` | One family's protocol on `GPSProtocolRuntime`, below `GPSDriver`: what it sends, and what it does with a reply that is late, rejected, split across reads or cancelled | A function that takes the clock, as a row of the suite's `SCENARIOS` |
| Family contract | `Core/Protocols/GPSProtocolFamilyContractTest.cc` | What every family does alike: pure decoding, validation, link failures, its log category, and behind `GPSDriver` the base status and the corrections it releases | Nothing for one family's case. A rule for all of them is one function over its two family tables |
| Golden transcripts | `Core/Protocols/GPSGoldenTranscriptTest.cc`, `Core/Protocols/Scenarios`, `Core/Protocols/golden` | All that `GPSDriver` sends and publishes in one scenario, as reviewed text: the bytes written, baud changes, evidence, identity, decoded events and whether each RTCM frame was forwarded | One `table.add()` line and a generated file to review |
| Decode goldens | The same suite, `Core/Protocols/golden/decode` | The events a family decodes from every sample in `Core/Protocols/corpus` and `Core/Protocols/fixtures`, and that they are the same for the file whole, byte by byte and in chunks of 7 bytes | A sample file whose name a rule of `decodersFor()` in `Core/Protocols/Scenarios/GoldenChecks.cc` claims |
| Capture decode | `Core/Protocols/GPSCaptureDecodeTest.cc`, `NTRIP/NTRIPCaptureTest.cc` | That logs of real receivers and casters (`*.cap`) decode to the values independent tools read from the same bytes | The most: a slice of a log, its source, and expectations generated with the pinned tools |
| Driver, detection, worker | `Core/Protocols/GPSDriverTest.cc`, `Core/Protocols/GPSReceiverDetectorTest.cc`, `Core/GPSReceiverWorkerTest.cc` | What is above a family: deadlines, validation before I/O, automatic detection, the worker's thread and what it reports, which `WorkerReports` records | A test function on a `ModelReceiver` |
| Transports | `Core/Transport` | Each `GPSTransport` against a peer on this machine, such as a socket or a pseudo-terminal, and what a recording holds | A test function |
| Connection supervision | `Connection/GPSConnectionSupervisorTest.cc` | What `GPSConnectionSupervisor` answers to a sequence of what its owner tells it: the phase, the delay before the next attempt and the attempts used, without a clock or a transport | A `Sequence` of steps in `_answers_data()` |
| Services | `Receiver`, `Corrections`, `NTRIP`, `Positioning`, `GPSManagerTest.cc`, `GPSSettingsBindingsTest.cc` | `GPSReceiver`, `GPSCorrectionManager`, `NTRIPManager` and `PositionManager` on the main thread: their states, retries and timeouts, and what reaches the Facts and the vehicles | A test function with a scripted worker or a mock transport, and a `ManualScheduler` |
| Panels and settings pages | `Qml` | The QML components, which C++ suites create outside the main window over a service they script. `Qml/GPSSettingsPagesTest.cc` alone runs the two pages over the application's services and saved settings | A test function that finds an item by its `objectName` |
| Fuzzers and smoke runs | `Core/Protocols/Fuzz`, `NTRIP/HTTP/Fuzz` | Nothing for certain: five libFuzzer harnesses look for an input that breaks a decoder or the driver, and only the standalone tree builds them. Four have a smoke run that sends fixed seeds through the harness without libFuzzer, in the application's build too; `GPSProtocolFuzzSmoke` is 10000 runs of its harness | Nothing: a sample in `Core/Protocols/corpus` is a seed |
| Standalone tree | `Standalone` | That `QGCGPSCore` and `QGCNTRIPHttp` build without the application, and that the goldens, the pipe transport checks, the runner's check and the smoke runs pass there, with `QGC_GPS_LIBFUZZER` under ASan and UBSan | Nothing: it runs what the rows above define |
| Runner, simulator, recordings | `Core/Protocols/Hardware`, `Core/Transport/Support`, `Core/Protocols/fixtures` | What a real receiver answers, by hand against a device or from a recording a user sends. The tree holds neither yet: its own tests run the program against the u-blox model | A recording, cut and committed with its source |

Choose by what changed:

- **How bytes are read**: a parser row, or a sample in `Core/Protocols/corpus`, which the decode goldens and the
  fuzzers read.
- **What a family sends or publishes**: a golden scenario. A protocol suite takes what a transcript does not show:
  `FemtoProtocolTest::_receiverMode` has reads that are cancelled, cut to one byte and preceded by noise, and leaves
  the plain rejections to the goldens.
- **A rule for every family**: the contract suite, which holds the next family to it.
- **What the application does with a receiver or a caster**: a service suite. It runs no protocol: its worker or
  transport is scripted.
- **What the user sees**: a panel suite, over the same scripted service.

A suite is `<Name>Test.h` and `<Name>Test.cc` with a line in `_qgc_gps_suites` (`CMakeLists.txt`) or
`_qgc_gps_core_suites` (`Core/CMakeLists.txt`). `qgc_add_gps_suites()` gives it the CTest name of its file and the
label `GPS`, which three suites outside `test/GPS` carry too: `GPSCorrectionSettingsTest` and `RTKSettingsTest` in
`test/Settings`, and `VehicleGPSFactGroupTest` in `test/Vehicle`. A suite that needs no application registers
with `UT_REGISTER_TEST_LIGHTWEIGHT`, as every suite under `Core` does.

## The test doubles, by layer

| Under test | Double | Where | What it is |
| --- | --- | --- | --- |
| A family, `GPSDriver`, `GPSReceiverWorker` | `ScriptedReceiver` | `Core/Protocols/Support/ScriptedReceiver.h` | A `GPSTransport` without a link. Its `Model` answers each command, or the test queues replies and sets handlers for open, read, write and baud rate. It keeps the `commands()` it was sent |
| | The receiver models | `Core/Protocols/Support/<Model>ReceiverModel.h` | A `ScriptedReceiver::Model` that answers as a family's receiver, for the six families that are configured. Passive input has none: its bench is the base `Model`, which never answers |
| | `ScriptedFaults`, `DetectionReceiver` | `Core/Protocols/Support/ScriptedFaults.h`, `Core/Protocols/Support/DetectionReceiver.h` | Models around a model: rules that answer, silence or fail the commands they match, and a receiver that parses only its own `Dialect`, at its own baud rate |
| | `ReceiverBench`, `ModelReceiver` | `Core/Protocols/Support/ReceiverBench.h` | The rig of the suites above the parsers: a scripted receiver with its model behind fault rules, a `GPSTestClock` and an `EventLog` of what the runtime delivered. `runtime()` gives a protocol suite its `GPSProtocolRuntime`, and the bench is the transport of a `GPSDriver` or a worker |
| | `GPSTestClock` | `Core/Protocols/Support/GPSTestClock.h` | Virtual microseconds, see below |
| `GPSReceiver`, `GPSManager` | `ScriptedReceiverWorker`, `ScriptedGPSReceiver` | `Receiver/Support/ScriptedReceiverWorker.h`, `Receiver/Support/GPSReceiverTestSupport.h` | A `GPSReceiverWorker` that starts no thread and reports what the test calls: `ready()`, `position()`, `baseStatus()`, `rtcm()`, `fail()`, or any event with `send()`. A `ScriptedGPSReceiver` is a real `GPSReceiver` whose workers are these |
| | `ScriptedBluetooth` | `Receiver/Support/ScriptedBluetooth.h` | A Bluetooth platform without a radio: scripted workers, a permission the test gives and a discovery the test fills |
| `NTRIPManager` | `MockNTRIPTransport` | `NTRIP/Support/MockNTRIPTransport.h` | An `NTRIPTransport` without a socket: `simulateConnect()`, `simulateRtcmData()`, `simulateError()`. `injectMockTransport()` makes the manager open it |
| `NTRIPHttpTransport`, `NTRIPHttpSession` | `ScriptedNTRIPCaster`, `CasterSession` | `NTRIP/Support/ScriptedNTRIPCaster.h`, `NTRIP/Support/CasterSession.h` | A caster on the loopback interface, over TCP or TLS, whose responses the test writes by hand. A `CasterSession` is one with its transport and scheduler |
| `PositionManager` | `PositionSource` | `Support/GPSTestHelpers.h` | This device's position source, whose updates and errors the test publishes |
| The services | `ManualScheduler` | `test/Utilities/Timing/Support/ManualScheduler.h` | The `RuntimeScheduler` they take, see below |
| Panels | `RTKSettingsFixture`, `AppReceiver` | `Qml/Support/GPSPanelFixtures.h` | Receiver settings that are restored when the test ends, and the application's receiver with scripted workers |
| | `TestFixtures::QmlEngine`, `TestFixtures::QmlWindowFixture` | `test/UnitTestFramework/QmlTesting/QmlFixtures.h` | Create a component from its source file, and show it |
| Recordings | `GPSRecordingReader`, `ReplayGPSTransport`, `GPSDialogueTransport` | `Core/Transport/Support` | The parser of the recording format, and the transports that play a recording to a decoder or as a dialogue; the last is in `GPSRecordingDialogue.h` |
| The application over TCP | `GPSReceiverSimulator` | `Core/Protocols/Hardware/GPSReceiverSimulator.h` | The u-blox model, or a recording, behind a TCP port |

Three names differ by a word: `ScriptedReceiver` is a transport, `ScriptedReceiverWorker` a worker, and
`ScriptedGPSReceiver` a receiver service.

The timeouts of the code under test run on one of two virtual clocks, and no file uses both:

- **`GPSTestClock`**, under `Core` only. `source()` is the `GPSClock` a `GPSDriver` or a `GPSReceiverWorker` runs
  on, and the receiver models hold the same clock: a wait, or a read that finds nothing, advances it instead of
  sleeping. A scenario of a protocol suite gets a fresh one.
- **`ManualScheduler`**, outside `Core` only: in `Receiver`, `Corrections`, `NTRIP`, `Positioning`, `Qml` and the
  two suites of `test/GPS` itself. The test moves it with `advanceBy()`, which runs the callbacks that are due.

Real sockets, pseudo-terminals and threads still take real time, which a suite bounds with a `TestTimeout`.

## A receiver family's tests

Follow Femtomes, the smallest family. Today `git grep -il femto -- test/GPS` finds it in 39 files, this one among
them, and `git ls-files test/GPS | grep -i femto` finds 18 that are named for it. Besides this file:

- **4 sources of its own**: `Core/Protocols/Support/FemtoReceiverModel.h`, `Core/Protocols/Femto/FemtoProtocolTest.h`
  and `.cc`, and `Core/Protocols/Scenarios/GoldenScenariosFemto.cc`.
- **14 data files**: 4 samples `femto-*.bin` in `Core/Protocols/corpus`, and 10 goldens that are generated, 9 in
  `Core/Protocols/golden/femto` and `Core/Protocols/golden/decode/femto.golden`.
- **24 files of shared code that name it.** 18 of them name Unicore and Quectel too: those are the places a new
  family is added to, listed below. The other 6 hold what is particular to Femtomes, such as its framing in
  `Core/Protocols/GPSStreamDemuxTest.cc`, or use it as an example. A family adds what is particular to it in the
  same way: a detection scenario goes to `Core/Protocols/Scenarios/GoldenScenariosAutomatic.cc`.

So a family is 4 files to write and 18 to edit, in this order. A path below that does not start with `Core`,
`Receiver` or `Qml` is in `Core/Protocols`:

1. **The model**: `Support/<Family>ReceiverModel.h`, listed in `Support/CMakeLists.txt`. For detection, its
   `Dialect` and its case of `parsesCommand()` in `Support/DetectionReceiver.h`.
2. **The suite**: `<Family>/<Family>ProtocolTest.h` and `.cc`, listed in `Core/CMakeLists.txt`.
3. **The scenarios**: `Scenarios/GoldenScenarios<Family>.cc`, listed in `Support/CMakeLists.txt`. Its bench, its
   stream and its `add<Type>()` are declared in `Scenarios/GoldenScenarios.h` and defined or called in
   `Scenarios/GoldenScenarios.cc`.
4. **The samples**: files in `corpus`, with a row in `decoders()` and a rule in `decodersFor()` of
   `Scenarios/GoldenChecks.cc`. Then [regenerate the goldens](#regenerating-expectations).
5. **The tables of the shared suites**, 7 files: `kFamilies` and `baseFamilies()` in
   `GPSProtocolFamilyContractTest.cc`; `NMEA_FORWARDERS` and the families that arm without I/O in
   `GPSGoldenTranscriptTest.cc`; `MANAGED_RECEIVERS` and the capability rows in `Core/Common/GPSReceiverConfigTest.cc`;
   the families of `Core/Transport/GPSRecordingDialogueTest.cc`; and the rows and family lists of
   `Receiver/GPSReceiverPresentationTest.cc`, `Receiver/GPSReceiverTest.cc` and `Qml/GPSReceiverSettingsTest.cc`.
6. **Detection and fuzzing**: the `switch` of `bench()` and the signature and probe rows in
   `GPSReceiverDetectorTest.cc`, and `seeds()` in `Fuzz/ReplyFuzzSmoke.cc`.
7. **The runner**: `--family` in `Hardware/GPSHardwareRunner.cc`, a replay of a sample in `Hardware/check_runner.py`,
   and a row of the qualification table in `fixtures/README.md`.

The names follow two things. The test folder, the model and the scenario file follow the folder in `src`
(`Ashtech`, `SBF`, `UBX`). The golden directory, `add<Type>()` and `--family` follow the `GPSType` (`trimble`,
`septentrio`, `ublox`). No table lists the families once, so grep for an existing family before trusting this
list.

## Regenerating expectations

Three kinds of expected values are generated and checked in. None is regenerated to make a test pass.

| Expectations | Regenerate when | How | Review |
| --- | --- | --- | --- |
| The goldens, `Core/Protocols/golden` | The driver's behaviour changes on purpose, or a scenario or a sample is added | `QGC_GPS_GOLDEN_UPDATE=1 ctest --test-dir build -R GPSGoldenTranscriptTest`, which rewrites what differs and removes the files no scenario produces | `git diff` of the directory: every changed line is a byte the driver sends or an event it publishes. The commit body explains each changed golden. A refactor changes none |
| Fixture and capture expectations: `GPSFixtureExpectations.h` and `GPSCaptureExpectations.h` in `Core/Protocols/fixtures`, `NTRIP/fixtures/NTRIPCaptureExpectations.h` | A fixture or a capture is added. Never for a change of a decoder: no value in them comes from QGroundControl | `Core/Protocols/fixtures/generate_expectations.py`, with the tools `requirements.txt` pins there | The diff holds only the new file's values and its SHA-256 |
| The u-blox key database, `Core/Protocols/fixtures/UBXConfigKeyDatabase.h` | The pinned pyubx2 changes | `Core/Protocols/fixtures/generate_config_keys.py` | `UBXConfigKeyTest` checks the keys the driver uses against it |

[SOURCES.md](Core/Protocols/fixtures/SOURCES.md#regeneration) has the commands, which need the network once to
install the pinned tools. With `--check` both scripts compare instead of writing.

## The runner and the simulator

`QGCGPSHardwareRunner` (`Core/Protocols/Hardware/GPSHardwareRunner.cc`) is one program with six modes, which
reports in JSON. The Linux test build puts it in `build/Debug`, and the standalone tree builds it too.
`GPSHardwareRunner.Scripted` runs `Core/Protocols/Hardware/check_runner.py` against it, and
`GPSReceiverSimulatorTest` runs the simulator inside the suite.

| Mode | Options | What runs |
| --- | --- | --- |
| Plan | None | It prints what it would do and opens nothing |
| Scripted model | `--action configure`, `suite` or `cancel` | The driver against the u-blox model, with `--model`, `--fault` and `--survey-state`, on the model's virtual clock. `--real-time` runs it on the steady clock. Of the other families only `--family automatic` runs here, and detects that model |
| Device | `--transport serial` or `tcp`, `--device`, `--allow-reconfigure` | The driver against a receiver, whose configuration it changes and does not restore. `--record` keeps the session as a recording and `--output` the evidence |
| Replay | `--replay <file> --family <family>` | The family's decoder on a recording or on raw receiver output. Nothing is sent. `--extract` saves the receiver's bytes |
| Dialogue check | `--replay <file> --dialogue` | The driver with the settings the recording holds, against its bytes at their recorded times. The report names the first record the driver does not reproduce |
| Simulator | `--serve <port>` | A receiver for the application's TCP connection: the u-blox model, or with `--replay` a recording played to each client. [tools/simulation](../../tools/simulation/README.md#gnss-receiver) shows it with the application |

A recording a user sends becomes a test in one of two ways, which the
[fixtures README](Core/Protocols/fixtures/README.md) describes step by step:

- **What the receiver sent**, a
  [decode fixture](Core/Protocols/fixtures/README.md#from-a-users-recording-to-a-regression-test). Decode the
  recording with `--replay`, cut the fewest messages that show the problem, and add them as a fixture that the
  decode goldens or `GPSCaptureDecodeTest` read.
- **What the driver and the receiver said to each other**, a
  [dialogue fixture](Core/Protocols/fixtures/README.md#a-dialogue-as-a-regression-test). The recording goes to
  `Core/Protocols/fixtures/dialogues`, which does not exist yet: `GPSRecordingDialogueTest` skips that function
  until the first one is added.

No run against a receiver is recorded for any family: see
[hardware qualification](Core/Protocols/fixtures/README.md#hardware-qualification).

## Commands

```bash
LABELS=GPS just test                                              # every GPS test of the application's build
ctest --test-dir build -N -L GPS                                  # list them
ctest --test-dir build -R FemtoProtocolTest --output-on-failure   # one suite

# The standalone tree, with the libFuzzer harnesses, which need Clang
cmake -S test/GPS/Standalone -B build-gps -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=clang++ \
    -DQGC_GPS_LIBFUZZER=ON
cmake --build build-gps
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-gps          # 8 tests, 7 without QGC_GPS_LIBFUZZER

# One fuzzer for a minute: the inputs that reach new code go to the first directory
cd build-gps && ProtocolFuzz/QGCGPSProtocolFuzz -max_total_time=60 -timeout=25 -max_len=8192 \
    -create_missing_dirs=1 fuzz-corpus/protocol ProtocolFuzz/corpus ProtocolFuzz/fixture-seeds
```

The GPS Standalone workflow (`.github/workflows/gps-standalone.yml`) has the command of each of the five fuzzers.

- **The standalone tree is configured apart.** Give it `-DCMAKE_PREFIX_PATH` with the Qt directory when CMake
  finds no Qt. Its first configure downloads GeographicLib, which `QGCGPSCore` links
  (`src/Utilities/Geo/GeographicLib.cmake`). Build it optimized: the fuzzer's smoke run is slow otherwise.
- **Panel suites run through `ctest`.** CTest gives each test the offscreen platform, the software Quick backend
  and a temporary directory of its own (`cmake/QGCTest.cmake`); the program run by hand has the desktop's.
  `GPSSettingsPagesTest` is registered standalone, because it binds the application's services to the settings for
  the rest of its process.
- **A new suite in an old build** can fail to link with `undefined symbol: vtable for <Suite>`. Delete
  `build/test/QGCUnitTests_autogen/timestamp` and build again.
- **Several checkouts at once.** A test process keeps its settings and caches under the name of its suite in the
  user's configuration and cache directories, clears the settings when it starts and removes both when it ends
  (`src/QGCApplication.cc`). Two checkouts that run the same suite at the same time share them: give each run its
  own `XDG_CONFIG_HOME` and `XDG_CACHE_HOME`.
- **Not the live Bluetooth suite.** `just test` without `LABELS` also runs `BluetoothLiveAdapterTest`
  (`test/Comms/Bluetooth`), which starts a scan on a machine that has an adapter. It does not have the label
  `GPS`; for more than that label, run `ctest` with `-E BluetoothLiveAdapterTest`.

## Rules of the suites

- **The log check is strict.** A warning that a test triggers fails it, unless the test expects it with
  `expectLogMessage()` and `verifyExpectedLogMessage()`
  ([test/README.md](../README.md#debugging-test-failures)). A suite of `GPSProtocolTestBase` can read the protocol
  warnings instead: creating a `GPSProtocolLogCapture` accepts the `GPS.Protocols.*` warnings for the rest of the
  test function, and the scenario asserts what `warnings()` returns. Only a scenario that warns by design creates
  one. It does not cover another category, such as `GPS.GPSDriver`.
- **A model's complaint fails the test.** A receiver model that is sent what its receiver would not accept records
  it in `ModelViolations`. `GPSProtocolTestBase` then fails the test function, and the protocol fuzzer aborts.
- **No fixed waits.** The `check-no-fixed-qwait` hook rejects `QTest::qWait()` with a number. Advance the clock of
  the layer, or wait for a condition with a `TestTimeout`.
- **Goldens are explained.** A commit that changes a golden says in its body what changed in each and why. A
  refactor changes none.
- **Names in a unity build.** CI compiles in unity batches. `qgc_add_gps_suites()` keeps every suite's source out
  of them, so each protocol suite can have its `SCENARIOS`. The libraries `QGCGPSProtocolTestSupport` and
  `QGCGPSTestTransports` are batched: a file-local name there must not repeat in another file, as `FEMTO_NAK` and
  `SBF_NAK` of the scenario files do not. `PipeTransportChecks.cc` is excluded instead.
- **The support libraries build without the application.** The standalone tree builds both, so their sources
  include neither the application nor the unit test framework. `Core/Protocols/Support/GPSProtocolTestBase.cc`,
  which needs the framework, is a source of the application's tests instead.
