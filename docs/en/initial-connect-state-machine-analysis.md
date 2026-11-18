# Initial Connect State Machine Analysis

## Scope

This document analyzes QGroundControl's vehicle initial connection pipeline implemented in:

- `src/Vehicle/InitialConnectStateMachine.h`
- `src/Vehicle/InitialConnectStateMachine.cc`

It also covers upstream/downstream dependencies that influence behavior:

- `src/Vehicle/Vehicle.cc`
- `src/Vehicle/ComponentInformation/ComponentInformationManager.*`
- `src/Utilities/StateMachine/*`
- `src/FirmwarePlugin/FirmwarePlugin.cc`
- `src/FactSystem/ParameterManager.cc`
- `src/MissionManager/GeoFenceManager.cc`
- `src/MissionManager/RallyPointManager.cc`
- `src/API/QGCOptions.h`

---

## 1) What the Initial Connect State Machine does

At first online vehicle connection, QGC runs a linear state machine to gather critical vehicle metadata and synchronize mission-related state.

### State order

1. **Request AUTOPILOT_VERSION** (`RetryableRequestMessageState`)
2. **Request Standard Modes** (`AsyncFunctionState`)
3. **Request Component Information** (`AsyncFunctionState`)
4. **Request Parameters** (`SkippableAsyncState`)
5. **Request Mission** (`SkippableAsyncState`)
6. **Request Geofence** (`SkippableAsyncState`)
7. **Request Rally Points** (`SkippableAsyncState`)
8. **Signal Complete** (`FunctionState`)
9. **Final** (`QGCFinalState`)

The state machine is started from `Vehicle` construction for normal vehicles.

---

## 2) Progress model

The machine uses weighted progress:

- AutopilotVersion: 1
- StandardModes: 1
- ComponentInfo: 5
- Parameters: 5
- Mission: 2
- GeoFence: 1
- RallyPoints: 1
- Complete: 1

This means parameter and component-info stages dominate progress reporting (as intended), while mission/fence/rally are lighter-weight contributors.

Sub-progress is wired from managers during long operations:

- `ComponentInformationManager::progressUpdate`
- `ParameterManager::loadProgressChanged`
- `MissionManager::progressPctChanged`
- `GeoFenceManager::progressPctChanged`
- `RallyPointManager::progressPctChanged`

---

## 3) Retries/timeouts and failure behavior

### Which states have a timeout

Only the AUTOPILOT_VERSION request has a timeout in this machine: 5000 ms for each attempt, with one retry (`_autopilotVersionMaxRetries = 1`).

The other states have none, because how long they take varies too much with the link speed and the amount of data. Each of them waits for a manager that handles its own timeouts and retries and always reports when it is done:

| State | Completed by |
| --- | --- |
| StandardModes | `StandardModes::requestCompleted` |
| ComponentInfo | `ComponentInformationManager::requestAllComplete` |
| Parameters | `ParameterManager::parametersReadyChanged(true)`, `initialParametersRequestFailed`, or `cacheCheckOnlyFailed` for the cache check of an armed vehicle |
| Mission | `MissionManager::newMissionItemsAvailable` |
| GeoFence | `GeoFenceManager::loadComplete` |
| RallyPoints | `RallyPointManager::loadComplete` |

### Important semantic detail

The machine uses no `RetryTransition`. A manager that gives up still reports that it is done, and the machine goes on to the next state.

A step that fails is therefore **graceful degradation** (advance), not hard stop, for this machine.

### AUTOPILOT_VERSION state behavior

AUTOPILOT_VERSION uses `RetryableRequestMessageState`:

- An attempt fails when `Vehicle` reports that the request failed, or when 5000 ms pass without an answer.
- After a timeout the request is still outstanding in `Vehicle`, which refuses a second request for the same message. The retry is sent once `Vehicle` gives the first request up, and no timeout runs while the state waits for that. If the first request is answered after all, the answer is used.
- A request that `Vehicle` refuses, because another request for AUTOPILOT_VERSION is outstanding, ends at its timeout. The retry is sent then.
- When the retry fails too, the state invokes its failure handler, which sets assumed capabilities (see section 5), and advances (`failOnMaxRetries` is not enabled here).

---

## 4) Exactly what is skipped, and when

## A) Entire machine skipped

In unit tests, the machine is **not started** only for synthetic generic vehicles:

- Condition: `qgcApp()->runningUnitTests() && _vehicleType == MAV_TYPE_GENERIC`

All real vehicle types still run initial connect during tests.

## B) AUTOPILOT_VERSION request skipped

State skip predicate returns true if:

1. No primary link exists, **or**
2. Primary link is high-latency (`LinkConfiguration::isHighLatency()`), **or**
3. Primary link is log replay (`LinkInterface::isLogReplay()` true, e.g. `LogReplayLink`)

When skipped, the state completes immediately and transitions to StandardModes.

## C) Parameter download skipped

The parameter and plan states look at one setting: **Skip param/plan download if vehicle is armed on connect** (`MavlinkSettings::noInitialDownloadWhenArmed`, off by default). In this document, "armed skip" means that the setting is enabled and the vehicle is armed when the state is entered.

With the armed skip:

- For firmware other than PX4, the Parameters state is skipped and `ParameterManager::parameterDownloadSkipped` is set.
- For PX4, the state runs but asks only whether the cached parameters still match (`ParameterManager::tryHashCheckCacheLoad()`). A match loads the parameters from the cache. Otherwise the state advances without parameters, and `parameterDownloadSkipped` is set.

Without the armed skip the state always runs. On a high-latency or log replay link `ParameterManager` cuts the download short itself.

## D) Mission load skipped

Mission state is skipped if any of:

1. Armed skip (see C), **or**
2. No primary link, **or**
3. Link type should be skipped (`high latency` or `log replay`)

## E) Geofence load skipped

Geofence state is skipped if any of:

1. Armed skip (see C), **or**
2. No primary link, **or**
3. Link type should be skipped (`high latency` or `log replay`), **or**
4. `GeoFenceManager::supported()` is false
   - `supported()` is capability-driven: requires `MAV_PROTOCOL_CAPABILITY_MISSION_FENCE`

## F) Rally points load skipped

Rally state is skipped if any of:

1. Armed skip (see C), **or**
2. No primary link, **or**
3. Link type should be skipped (`high latency` or `log replay`), **or**
4. `RallyPointManager::supported()` is false
   - `supported()` requires `MAV_PROTOCOL_CAPABILITY_MISSION_RALLY`

When rally is skipped, QGC still marks:

- `_initialPlanRequestComplete = true`
- emits `initialPlanRequestCompleteChanged(true)`

so downstream UI logic does not stall waiting for rally load.

## G) Component information sub-states skipped (inside ComponentInformationManager)

`ComponentInformationManager` itself has internal skippable phases:

- Parameter metadata skipped if `GENERAL` metadata says parameter metadata unsupported
- Events metadata skipped if unsupported
- Actuators metadata skipped if unsupported

Support checks are based on `CompInfoGeneral::isMetaDataTypeSupported(type)`.

---

## 5) What data is captured during initial connect, and why

## A) AUTOPILOT_VERSION data captured

On successful AUTOPILOT_VERSION response, QGC captures:

1. **Vehicle UID** (`uid`)
   - Used as stable identity and surfaced via `vehicleUIDChanged`.

2. **Firmware board vendor/product IDs**
   - Hardware identification for board-specific behavior and diagnostics.

3. **Flight software semantic version** (`flight_sw_version` packed fields)
   - Parsed into major/minor/patch/version-type and stored in `Vehicle`.

4. **Firmware custom version / git hash**
   - PX4: decoded from binary bytes (reverse-order formatting)
   - APM: decoded as 8-char ASCII
   - Exposed via `gitHashChanged`.

5. **MAVLink capability bits**
   - Stored via `_setCapabilities(...)`
   - Drives feature gating such as mission-int, command-int, geofence, rally support.

If AUTOPILOT_VERSION fails, QGC sets **assumed capabilities**:

- Always assume MAVLink2
- For PX4/APM, also assume mission-int/command-int/fence/rally support

This allows connect flow to continue even without that message.

## B) Standard modes captured

QGC requests available standard modes one-by-one (`AVAILABLE_MODES`) and updates firmware plugin mode mapping.

Purpose:

- Provide accurate, vehicle-reported mode names/availability.
- Refresh flight mode UI mapping even if HEARTBEAT arrives earlier.

## C) Component Information captured

`ComponentInformationManager` requests metadata in this sequence:

1. `GENERAL` metadata
2. URI update pass (maps/propagates discovered metadata URIs)
3. `PARAMETER` metadata (if supported)
4. `EVENTS` metadata (if supported)
5. `ACTUATORS` metadata (if supported)

### Why this matters

- **Parameter metadata** (`CompInfoParam`) feeds `ParameterManager` metadata lookups (`factMetaDataForName`), affecting parameter typing/metadata behavior.
- **Events metadata** supports event schema/decoding ecosystem.
- **Actuators metadata** is used by PX4 actuator configuration UX:
  - If actuator metadata is absent or actuator conditions fail, PX4 plugin falls back to legacy Motor page.
  - If present and valid (`show-ui-if` etc.), Actuators page is enabled.

So component info is both a data-quality improvement and a feature-availability gate.

## D) Parameters stage captured

- Requests full parameter refresh (`refreshAllParameters()`)
- Waits for `parametersReadyChanged(true)`
- On ready:
  - Sends GCS time to vehicle twice (reliability)
  - Sets up auto-disarm signalling

This stage is foundational for most setup/calibration UI and many capability/behavior branches.

## E) Plan data captured

- Mission items from vehicle
- Geofence data (if supported and not skipped)
- Rally points (if supported and not skipped)

On rally completion (or rally skip), QGC sets initial plan load complete.

---

## 6) Where "check latest stable firmware" fits

The latest-stable firmware check is triggered as a **side-effect** of AUTOPILOT_VERSION success:

- Condition: `QGCOptions::checkFirmwareVersion()` is true and `_checkLatestStableFWDone` is false.
- Action: `FirmwarePlugin::checkIfIsLatestStable(vehicle)`.

This is asynchronous (downloads and parses a version file), and **does not gate** state machine transitions. Initial connect continues regardless of result.

It is skipped during unit tests in `FirmwarePlugin::checkIfIsLatestStable`.

---

## 7) Options/configurations that affect behavior

1. **Link configuration high-latency flag**
   - `LinkConfiguration.highLatency`
   - Causes skipping of AUTOPILOT_VERSION and plan-load states.

2. **Log replay link type**
   - `LinkInterface::isLogReplay()` true
   - Same skip effect as high-latency.

3. **Firmware/version check option**
   - `QGCOptions::checkFirmwareVersion()` (virtual, custom-build overridable)
   - Controls only latest-stable check side-effect, not transition graph.

4. **Capability bits**
   - From AUTOPILOT_VERSION or assumed fallback.
   - Gate geofence/rally support and therefore skip decisions.

5. **Metadata support flags from CompInfoGeneral**
   - Gate internal component-info sub-requests (param/events/actuators metadata).

6. **Skip param/plan download if vehicle is armed on connect**
   - `MavlinkSettings::noInitialDownloadWhenArmed`, off by default
   - For a vehicle that is armed, skips the parameter download (PX4 still tries its parameter cache) and the plan-load states.

---

## 8) Practical implications

- The machine is designed for **robust connect**, not strict fail-fast.
- Network/link quality and link type strongly influence which expensive operations are skipped.
- Component information is a key bridge between firmware-reported metadata and dynamic UI behavior (especially parameters and actuators).
- Missing AUTOPILOT_VERSION does not block connect; QGC degrades gracefully with assumptions.

---

## 9) Quick skip matrix

| State | Skip trigger(s) | Outcome |
| --- | --- | --- |
| AUTOPILOT_VERSION | no primary link OR high-latency OR log replay | immediate complete/advance |
| StandardModes | none explicit | advances when `StandardModes` reports completion |
| ComponentInfo | none explicit | advances when `ComponentInformationManager` reports completion |
| Parameters | armed skip, for firmware other than PX4 | skipped -> Mission; otherwise advances when `ParameterManager` reports the parameters ready, the request failed, or the cache check of an armed PX4 vehicle failed |
| Mission | armed skip OR high-latency OR log replay OR no primary link | skipped -> GeoFence |
| GeoFence | mission skip conditions OR !fence capability | skipped -> Rally |
| Rally | mission skip conditions OR !rally capability | skipped -> Complete (+mark plan complete) |
| Complete | never skipped | emits `initialConnectComplete` at machine finish |

### Run matrix by (HighLatency, LogReplay, Armed)

Assumptions for this matrix:

- Primary link exists
- Geofence and rally are supported by capability bits
- State machine is started (not the `runningUnitTests && MAV_TYPE_GENERIC` bypass)
- `Armed=1` means the armed skip of section 4 C: the vehicle is armed and the setting is enabled

Legend:

- `Run` = state logic executes (it may still complete quickly)
- `Skip` = state is skipped by state skip predicate
- `Run*` = state executes but parameter download is short-circuited inside `ParameterManager` for high-latency/log-replay links
- `Cache` = state is skipped, except for PX4, where it executes and only tries the parameter cache

| State | HL=0 LR=0 Armed=0 | HL=0 LR=0 Armed=1 | HL=1 LR=0 Armed=0 | HL=1 LR=0 Armed=1 | HL=0 LR=1 Armed=0 | HL=0 LR=1 Armed=1 | HL=1 LR=1 Armed=0 | HL=1 LR=1 Armed=1 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| AUTOPILOT_VERSION | Run | Run | Skip | Skip | Skip | Skip | Skip | Skip |
| StandardModes | Run | Run | Run | Run | Run | Run | Run | Run |
| ComponentInfo | Run | Run | Run | Run | Run | Run | Run | Run |
| Parameters | Run | Cache | Run* | Cache | Run* | Cache | Run* | Cache |
| Mission | Run | Skip | Skip | Skip | Skip | Skip | Skip | Skip |
| GeoFence | Run | Skip | Skip | Skip | Skip | Skip | Skip | Skip |
| Rally | Run | Skip | Skip | Skip | Skip | Skip | Skip | Skip |
| Complete | Run | Run | Run | Run | Run | Run | Run | Run |

Whether the vehicle is flying plays no part in the skip predicates; only whether it is armed does.

### Unit-test readable matrix (request expectations)

This is the same compact matrix used by `_stateRunMatrix_data` in `InitialConnectTest`, which connects a PX4 mock vehicle and leaves log replay off.

```text
+----+-------+------------+------------+-----------+-----------------+----------------+------------------+
| HL | Armed | AP_VERSION | AVAIL_MODS | PARAM_REQ | HASH_CHECK_ONLY | PLAN_REQ_LISTS | PARAM_DL_SKIPPED |
+----+-------+------------+------------+-----------+-----------------+----------------+------------------+
| 0  | 0     | Run        | Run        | Run       | -               | Run            | no               |
| 1  | 0     | Skip       | Run        | -         | -               | Skip           | no               |
| 0  | 1     | Run        | Run        | -         | Run             | Skip           | yes              |
| 1  | 1     | Skip       | Run        | -         | -               | Skip           | yes              |
+----+-------+------------+------------+-----------+-----------------+----------------+------------------+
```

Where:

- `HL` = HighLatency
- `Armed` = vehicle armed, with the setting enabled
- `AP_VERSION` = `AUTOPILOT_VERSION` request expected
- `AVAIL_MODS` = `AVAILABLE_MODES` request expected
- `PARAM_REQ` = full parameter download expected (it starts with a `_HASH_CHECK` request)
- `HASH_CHECK_ONLY` = only the `_HASH_CHECK` request of the cache check expected
- `PLAN_REQ_LISTS` = mission/geofence/rally `MISSION_REQUEST_LIST` traffic expected
- `PARAM_DL_SKIPPED` = `ParameterManager::parameterDownloadSkipped()` expected to be set

Skip behavior basis: `skipForLinkType = (highLatency || logReplay)`, and the armed skip for the parameter and plan requests.

---

## 10) Notes on observability/debugging

Useful log categories for tracing behavior:

- `Vehicle.InitialConnectStateMachine`
- `ComponentInformation.ComponentInformationManager`
- `Vehicle.StandardModes`
- `Vehicle` (capability logs)
- `ActuatorsConfig` (for metadata-driven actuator UI decisions)

This combination lets you determine:

- Which states were entered/skipped/timed out
- Which metadata types were supported/downloaded
- Why actuator UI appeared or fell back to legacy motor UI
