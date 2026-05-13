# 00 — Sprig GCS Architecture Overview

**Status:** Phase 0 baseline. Synthesized from focused surveys of six QGC subsystems against the Sprig fork plan. No knowledge graph yet — see `docs/knowledge-graph/baseline/README.md` for why that was deferred.

**Upstream baseline commit:** `b59e58994` (local `master`, tracking `mavlink/qgroundcontrol` main).

---

## 1. What we forked

QGroundControl is a Qt 6 / QML ground station for MAVLink vehicles. The codebase splits cleanly into:

- **Transport** — `src/Comms/` (link types, byte-level MAVLink parse), `src/MAVLink/` (dialect plumbing, FTP, signing, status text).
- **Vehicle model** — `src/Vehicle/` (per-vehicle state, message dispatch, FactGroup demux).
- **Firmware abstraction** — `src/FirmwarePlugin/` (PX4, APM behavior) and `src/AutoPilotPlugins/` (vehicle setup UI).
- **Mission/plan** — `src/MissionManager/` (mission item hierarchy, controllers, transect math).
- **UI shell** — `src/UI/`, `src/QmlControls/`, `src/FlyView/`, `src/PlanView/` (QML modules, controls library, fly/plan view chrome).
- **Media** — `src/VideoManager/` (GStreamer pipeline + Qt Multimedia fallback).
- **Customer skin** — `custom-example/` (template for downstream forks; rename to `custom/` and the top-level CMake auto-picks it up).

The Fact System (`src/FactSystem/`) is the parameter substrate threaded through every subsystem — vehicle parameters, telemetry values, and settings are all `Fact*` objects with rawValue/cookedValue binding into QML.

---

## 2. The override architecture, and where it leaks

QGC has one strong override seam and several hardcoded ones. Knowing where each seam is determines whether a Sprig change lives in `custom/` (clean) or requires an entry in `UPSTREAM_PATCHES.md` (carried debt).

### 2.1 The clean seam: `CorePlugin` substitution

Top-level `CMakeLists.txt:48–53` detects `custom/` and sets `QGC_CUSTOM_BUILD=ON`. The mechanism is **compile-time class substitution**, not Qt plugin loading:

- `custom/CMakeLists.txt` defines `CUSTOMHEADER="CustomPlugin.h"` and `CUSTOMCLASS=CustomPlugin`.
- `src/API/QGCCorePlugin.cc:32–34` `#include CUSTOMHEADER`.
- `src/API/QGCCorePlugin.cc:61–67` returns `CUSTOMCLASS::instance()` from `QGCCorePlugin::instance()` when `QGC_CUSTOM_BUILD` is defined.

This gives a custom build a complete drop-in `QGCCorePlugin` subclass. From there, `CorePlugin` exposes a wide surface of `virtual` hooks: palette override, settings visibility, plan creators, save/load hooks, MAVLink message preview, complex-mission-item dropdown, QML import paths.

**`custom-example/` also subclasses `PX4FirmwarePlugin` and `PX4AutoPilotPlugin`** via a `CustomFirmwarePluginFactory` registered through a static global (`src/FirmwarePlugin/FirmwarePluginFactory.cc:16`, `custom-example/src/FirmwarePlugin/CustomFirmwarePluginFactory.cc:4`). The factory registry pattern is well-trodden — Sprig's vehicle-behavior customizations fit here without any upstream patches.

### 2.2 The QML override mechanism (less obvious than expected)

QML overrides do **not** work via import-path priority shadowing. Each module is registered with `qt_add_qml_module()` and lives at `qrc:/qml/QGroundControl/<Module>/`. The custom-build mechanism is a **URL interceptor**:

- `custom-example/src/CustomPlugin.cc:255–264` installs `CustomOverrideInterceptor` on the QML engine.
- The interceptor (lines ~275–296) sees every QML load. If a request for `qrc:/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml` is made and a custom version exists at `qrc:/Custom/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml`, the URL is rewritten.
- `custom.qrc` declares the `/Custom/qml` resource prefix that the interceptor points to.

This is powerful — any QML file that ships in a QGC module can be shadowed — but it's **per-file replacement, not partial override**. Restyling `QGCButton.qml` means owning every line of it.

### 2.3 The hardcoded seams (where patches will live)

Three subsystems have load-bearing logic that cannot be reached from `CorePlugin` and will require `UPSTREAM_PATCHES.md` entries:

1. **Complex mission item registration** — `src/MissionManager/MissionController.cc:394–413` (insert by canonical name) and `:717–755` (load by JSON type value) are hardcoded `if/else` chains over `Survey`/`CorridorScan`/`StructureScan`. Adding Sprig's racetrack/teardrop/trombone/headland items requires patches at both sites. The UI dropdown (`QGCCorePlugin::complexMissionItemNames()`) is virtual and **does not** need patching — that's clean.

2. **Hardcoded chrome references in main view QML** — `src/FlyView/FlyView.qml:174–178` and `src/PlanView/PlanView.qml:228–232` instantiate the toolbar by type, not via `Loader`. The URL interceptor can replace the toolbar QML file wholesale, but if Sprig wants the original toolbar to remain available as an option (e.g., a settings toggle), we need a `Loader` patch.

3. **Link types** — `src/Comms/LinkConfiguration.h:70–82` enumerates Serial/UDP/TCP/Bluetooth/Mock/LogReplay statically. Not on Sprig's roadmap, noted for completeness.

### 2.4 The MAVLink seam — actually clean, but easy to misuse

The dispatch path is: `LinkManager` → `MAVLinkProtocol::receiveBytes()` (`src/Comms/MAVLinkProtocol.cc:102`) → emits `messageReceived` (`:288`) → `Vehicle::_mavlinkMessageReceived()` (`src/Vehicle/Vehicle.cc:519`).

`Vehicle.cc` has a giant per-message-ID switch at `:588–749`. **Do not patch it.** Two virtual hooks run *before* the switch and cover Sprig's needs:

- `_firmwarePlugin->adjustIncomingMavlinkMessage()` (`:558`) — can intercept/transform messages per autopilot.
- A FactGroup loop at `:582–584` calls `handleMessage()` on every FactGroup the firmware plugin registered.

Custom telemetry (MaxxECU, AXM3, ENNOID-BMS, DTI HV-850, Volz, flow controllers) gets a `SprigVendorFactGroup` per subsystem, registered through the custom `PX4FirmwarePlugin` subclass's `factGroups()` override. Zero patches to `Vehicle.cc` required.

### 2.5 The dialect seam — fork mavlink, point CMake

`src/MAVLink/CMakeLists.txt:33–45` uses CPM to fetch `mavlink/mavlink` at a pinned commit, then runs `tools/generators/mavlink_enums.py` to produce the Qt-friendly enum wrappers. A Sprig dialect lives in our `mavlink` fork as `sprig.xml`. `cmake/CustomOptions.cmake:97` (`QGC_MAVLINK_DIALECT`) and the matching `QGC_MAVLINK_GIT_REPO`/`QGC_MAVLINK_GIT_TAG` settings are overridable from `custom/cmake/CustomOverrides.cmake`. No source patches.

---

## 3. Subsystem-by-subsystem reading guide

Six subsystems. For each: what it does, the load-bearing files, the Sprig touchpoint.

### 3.1 MissionManager — Phase 3 target

**Hierarchy:**
```
VisualMissionItem (abstract)
├── SimpleMissionItem
│   └── TakeoffMissionItem
├── ComplexMissionItem (abstract)
│   ├── TransectStyleComplexItem (abstract)   ← Sprig hangs new turn patterns here
│   │   ├── SurveyComplexItem
│   │   ├── CorridorScanComplexItem
│   │   └── StructureScanComplexItem
│   ├── FixedWingLandingComplexItem
│   └── VTOLLandingComplexItem
└── MissionSettingsItem
```

**The pure virtual that defines a pattern:** `TransectStyleComplexItem::_rebuildTransectsPhase1()` (`src/MissionManager/TransectStyleComplexItem.h:127`). Subclasses compute the transect coordinate list; the base class then emits `MAV_CMD_NAV_WAYPOINT` (+ camera triggers, condition gates, hover-and-capture image captures) via `_buildAndAppendMissionItems()` at `TransectStyleComplexItem.cc:1228`.

**QML editor wiring is per-instance, not per-class:** each subclass sets `_editorQml = "qrc:/qml/..."` in its constructor (e.g., `SurveyComplexItem.cc:28`). `VisualMissionItem::editorQml` is a `Q_PROPERTY` that the plan-view `Loader` binds to. **No registry friction here.**

**Controllers:** `MissionController` owns `_visualItems` (the live list); `PlanMasterController` owns `MissionController` + `GeoFenceController` + `RallyPointController` and handles file save/load and vehicle upload.

**Sprig path:** subclass `TransectStyleComplexItem` for each ag turn pattern (racetrack, teardrop, trombone, headland). Set `_editorQml` and `mapVisualQML()` to QML files dropped under `custom/res/Custom/qml/.../PlanView/`. Patch `MissionController.cc` at the two `if/else` sites (PATCH-001). Override `QGCCorePlugin::complexMissionItemNames()` in `SprigCorePlugin` to expose the items in the UI dropdown.

### 3.2 MAVLink + Vehicle + Comms — Phase 4 target

**Layered files:**
- `src/Comms/MAVLinkProtocol.{h,cc}` — byte parser.
- `src/Comms/LinkManager.{h,cc}` + `LinkConfiguration.h` + `SerialLink/`, `UDPLink`, `TCPLink`, `Bluetooth/` — link types (hardcoded enum at `LinkConfiguration.h:70–82`).
- `src/Vehicle/Vehicle.{h,cc}` — per-vehicle state, message dispatch.
- `src/Vehicle/FactGroups/` — per-subsystem demux into Facts (`VehicleGPSFactGroup`, `VehicleWindFactGroup`, etc.).
- `src/MAVLink/` — dialect plumbing, FTP, signing.

**Dispatch path & extension points:** see §2.4.

**`NAMED_VALUE_FLOAT` is not surfaced as a Fact in core QGC** — only `ArduSubFirmwarePlugin` decodes it. For Sprig that means **don't lean on `NAMED_VALUE_FLOAT` as a shortcut**; the right path is the proper Sprig dialect with a `SprigVendorFactGroup` per subsystem. The dialect cost is one mavlink fork + CMake setting; the FactGroup cost is small per subsystem and isolated.

### 3.3 FirmwarePlugin + AutoPilotPlugin — Phase 4–6 target

- `FirmwarePlugin` (`src/FirmwarePlugin/FirmwarePlugin.h:70`) = **vehicle behavior** (flight modes, guided commands, MAVLink message pre-processing, FactGroup registration, supported mission commands).
- `AutoPilotPlugin` (`src/AutoPilotPlugins/AutoPilotPlugin.h:16`) = **setup UI** (radio cal, sensors, parameters, airframe selection — exposed as `VehicleComponent` objects with QML panels).
- `FirmwarePluginFactory` is a static-global-constructor registry (`FirmwarePluginFactory.cc:16`). `FirmwarePluginManager::instance()` (Q_GLOBAL_STATIC) looks up by `MAV_AUTOPILOT` type.

**Airframe selection** is owned by `AutoPilotPlugin::vehicleComponents()` → `AirframeComponent` → `AirframeComponentController` reading the `SYS_AUTOSTART` Fact. Sprig's S1000 (`SYS_AUTOSTART=24001`) registration needs a `SprigPX4AutoPilotPlugin` subclass and likely an `AirframeComponent` override — **CorePlugin alone is not enough** for vehicle setup customization.

**The boundary, restated:** CorePlugin = customer skin + global gates. FirmwarePlugin = vehicle behavior. AutoPilotPlugin = setup UI. Sprig will subclass all three.

### 3.4 Custom build system — Phase 1 foundation

See §2.1–2.2. The mechanism:

- `CMakeLists.txt:48–53` auto-detects `custom/`.
- Preprocessor substitution: `QGCCorePlugin.cc:61–67` returns `CUSTOMCLASS::instance()` defined in `custom/CMakeLists.txt`.
- QML overrides via `CustomOverrideInterceptor` URL rewriter at engine startup (`custom-example/src/CustomPlugin.cc:255–264`).
- Asset overrides via `custom.qrc` resource prefixes (`/Custom/qml`, `/Custom/res`, `/custom/img`).
- Branding via CMake variables in `custom/cmake/CustomOverrides.cmake`.

**`custom/` is intended to be gitignored downstream** — `custom-example/` is the in-tree template. The convention is: rename `custom-example/` → `custom/` for setup. For Sprig we'll instead populate `custom/` as the live fork-specific folder and keep `custom-example/` upstream-clean.

### 3.5 QML layering + Fly/Plan view chrome — Phase 2 target

**QML modules:** `qt_add_qml_module()` per directory, resource prefix `/qml`, URIs like `QGroundControl.Controls`, `QGroundControl.Toolbar`, `QGroundControl.FlyView`. Import path added at `src/API/QGCCorePlugin.cc:278`.

**Theme/palette:** `QGCPalette` (`src/QmlControls/QGCPalette.h`) is a C++ Q_OBJECT with QML_ELEMENT and a static global `_theme`. Theme swap is driven by `appSettings.indoorPaletteName` plus the `CorePlugin::paletteOverride()` virtual hook. Sprig palette = `SprigCorePlugin::paletteOverride()` returning custom colors keyed by `Theme` × `ColorGroup` × name. Pure CorePlugin work.

**`ScreenTools`** = QML singleton + C++ `ScreenToolsController`. All sizing in QGC references `ScreenTools.defaultFontPixel{Height,Width}` and `toolbarHeight`. Sprig restyle must continue to use these to stay DPI-correct on Android tablets.

**Chrome seams in FlyView/PlanView:**
- `FlyView.qml:174–178` and `PlanView.qml:228–232` instantiate the toolbar by type. URL interceptor can shadow the QML file; cannot inject a `Loader` choice without patching.
- `FlyViewCustomLayer.qml` is a built-in **empty placeholder overlay** — first-class extension point for Sprig fly-view chrome with zero upstream patching.
- `FlyViewInstrumentPanel.qml` uses `SelectableControl` driven by `flyViewSettings.instrumentQmlFile2` Fact — instruments are already runtime-swappable via settings.

**Phase 2 truth:** the toolbar and right-panel chrome do not have Loader-based seams. We either (a) shadow the QML file entirely (URL interceptor — Sprig owns the whole toolbar QML), or (b) patch the two view QML files to add `Loader` choices (PATCH-002, PATCH-003). Option (a) is cheaper for now; (b) becomes worthwhile if we want operator-toggleable layouts.

### 3.6 VideoManager — Phase 4 target

- GStreamer-primary, Qt Multimedia fallback. GStreamer fetched via custom CMake finder (`FindQGCGStreamer.cmake`) — system pkg-config, vendored SDK, or macOS framework.
- `VideoManager` owns N `VideoReceiver`s (currently hardcoded as `videoContent` + `thermalVideo`). `GstVideoReceiver` builds a `source → tee → [decoder → sink] + [recorder → fileSink]` pipeline.
- `GstAppSinkAdapter` bridges GStreamer to `QVideoSink` with platform-specific GPU zero-copy (DMABuf/GL/D3D11/12/IOSurface/AHardwareBuffer).
- QML rendering via Qt Multimedia `VideoOutput`.
- Sources: RTSP, UDP H.264/H.265, MPEG-TS (UDP + TCP), plus hardcoded vendor presets (3DR Solo, Parrot, Yuneec, Herelink) in `VideoSettings.h:53–64` and `VideoManager.cc:505–709`.
- Recording: MKV/MOV/MP4 muxer chains + `SubtitleWriter` writing telemetry SRT.

**Sprig Phase 1 risks:** hardcoded vendor names in the source enum need to be hidden (settings dropdown leaks "3DR Solo"). **Sprig Phase 4 path:** add a third receiver (sprayer-cam) — modest patch to `VideoManager` (~200 LOC, the 2-stream assumption is shallow). Side-by-side display needs a new QML container.

---

## 4. The Fact System (mentioned everywhere — read once)

`src/FactSystem/Fact.h` is the parameter substrate. A `Fact` carries a typed value (raw + cooked, with units/translations), metadata, and emits `rawValueChanged` for QML bindings.

- `FactGroup` (`src/FactSystem/FactGroup.h:48`) = a named collection of Facts. Override `handleMessage(mavlink_message_t*)` to demux MAVLink into Facts.
- `ParameterManager` (per vehicle) owns the autopilot parameter set.
- `SettingsManager` owns app settings as Facts (in `src/Settings/`).

**Why this matters for Sprig:** every Sprig telemetry point becomes a Fact on a custom FactGroup. QML widgets bind directly to `vehicle.sprigBmsFactGroup.packVoltage.value`. No bespoke property plumbing.

---

## 5. The plan, restated against the architecture

| Phase | What it touches | Where it lives | Patch ledger risk |
|---|---|---|---|
| **0** | Architecture understanding, build green, ADR-001 | docs only | None |
| **1** | Branding, palette, logos, splash, app name | `custom/` only via `CorePlugin` subclass + `custom.qrc` + `CustomOverrides.cmake` | **Zero** — if it isn't zero, stop and refactor |
| **2** | Toolbar restyle, instruments, fly/plan chrome, control library | Mostly `custom/` via URL interceptor + QML shadowing. Toolbar/right-panel chrome may need PATCH-002/-003 in `FlyView.qml`/`PlanView.qml` if we want toggleable layouts. | Low-medium |
| **3** | Custom turn patterns | New `TransectStyleComplexItem` subclasses in `custom/src/MissionManager/`; PATCH-001 in `MissionController.cc` (insert + JSON load); CorePlugin override for UI dropdown | One unavoidable patch, propose upstream factory PR |
| **4** | Sprig telemetry overlays, Sprig MAVLink dialect, sprayer-cam | `SprigVendorFactGroup`s registered via `SprigPX4FirmwarePlugin::factGroups()` (clean). Mavlink fork pinned via CMake (clean). Sprayer-cam adds a third `VideoReceiver` (medium patch). | Low for telemetry, medium for video |
| **5** | Ag mission features (prescription maps, field boundaries, coverage reports) | Mostly new code in `custom/src/AgMission/`. Plan view overlays may need PATCH-004 if no Loader seam in `PlanView.qml`. | Medium |
| **6** | Avilogix sync, CropIntel pull, SSO, tail-number gating | New code in `custom/src/Sprig/`. Pre-flight arming gate hooks into `Vehicle::_armingFlagsChanged`-adjacent code — verify CorePlugin/FirmwarePlugin can express this without core patches. | TBD |
| **7** | Polish, mobile, release | Mostly build/packaging. | None |

The fork plan is sound. The two architectural surprises ahead of us are:

1. **QML overrides are URL-interception, not import-path priority.** This means we can shadow individual QML files but the granularity is whole-file. Plan accordingly — don't design Phase 2 around "subclass `QGCButton`"; design it around "Sprig owns `QGCButton.qml`."
2. **The major view QML files (`FlyView.qml`, `PlanView.qml`) hardcode their toolbar/panel children.** The URL interceptor can swap them, but we lose the upstream version. Worth one PR back to upstream to introduce Loader-based chrome.

---

## 6. What's deferred from Phase 0

- **Knowledge graph** — `/understand-anything` over a ~1,945-file Qt codebase exceeds the skill's design envelope. Deferred until after Phase 1 when the diff against upstream is smaller and a scoped graph (per-subsystem) is cheaper to maintain. See `docs/decisions/002-defer-knowledge-graph-baseline.md` (forthcoming).
- **Three-platform CMake configure-green** — needs Linux/Windows/Android Qt toolchains set up. Tracked as Phase 0 task.
- **Codename decision** — kickoff says "Avicontrol v2 (or pick something better)" but doesn't commit. Tracked as Phase 0 task.

---

## 7. Read-first files for any new task

Match the kickoff's "Critical Files" list, with the additions surfaced by the surveys:

1. `src/FactSystem/Fact.h`, `src/FactSystem/FactGroup.h` — parameter and telemetry substrate.
2. `src/Vehicle/Vehicle.h` + `Vehicle.cc:519–749` (message dispatch).
3. `src/FirmwarePlugin/FirmwarePlugin.h` + `FirmwarePluginFactory.h`.
4. `src/AutoPilotPlugins/AutoPilotPlugin.h` and `PX4/PX4AutoPilotPlugin.h`.
5. `src/API/QGCCorePlugin.h` — the customer-skin contract.
6. `src/MissionManager/VisualMissionItem.h` + `TransectStyleComplexItem.{h,cc}` + `MissionController.cc:394–755`.
7. `custom-example/src/CustomPlugin.{h,cc}` and `custom-example/CMakeLists.txt` — the override mechanism in full.
8. `CMakeLists.txt:48–53` and `cmake/CustomOptions.cmake` — the build-time fork point.

If a task touches any of these, expect to consider whether the change belongs in `custom/` or in `UPSTREAM_PATCHES.md`.
