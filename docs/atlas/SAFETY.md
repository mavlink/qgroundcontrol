# Safety and operational limitations

> **ATLAS Video Control v0.12.2** · Safety analysis for the **current development baseline**
> This document identifies software and integration limitations; it is **not** an airworthiness assessment, certification, or authorization to fly.

ATLAS is a custom QGroundControl-based video and payload application. Its visual interface is optimized for the camera operator, but its inherited communication infrastructure includes capabilities outside camera operation. **Do not equate what is visible on screen with what is technically possible over the network.**

## 1. Scope and status

| Area | State in v0.12.2 | Operational consequence |
| --- | --- | --- |
| SIYI camera/gimbal control | Implemented via Direct A8 UDP backend | Commands intentionally actuate payload hardware |
| MAVLink telemetry | Successfully received from a tested Pixhawk through HM30 | Receiving telemetry does **not** make the link receive-only |
| MAVLink outbound command filter | **Not implemented** | ATLAS/QGC can send MAVLink to an attached vehicle |
| KML overlays | Display-only 2D/3D import | They do not prevent flight into a displayed region |
| Payload route | Manual Direct A8 / Jetson choice | There is no automatic failover |
| Jetson control/AI interface | TCP/JSON client and mock integration | Real inference/tracking not established by mock tests |
| Video | SIYI RTSP over HM30; workarounds applied | Link loss, latency and Linux rendering issues remain possible |

## 2. MAVLink receive-only is **not** implemented

### 2.1 Why the current link can transmit

The stock QGroundControl communications stack supports two-way links. An ATLAS UDP connection to the HM30 can both receive messages and write outgoing data. The underlying software can generate MAVLink messages for vehicle discovery, telemetry requests, parameter transactions, missions, mode changes and other actions. Removing Takeoff/RTL controls from the *custom Fly View* does **not** disable those code paths.

The working network configuration is approximately:

```text
Pixhawk UART ↔ HM30 Air ↔ HM30 RF ↔ HM30 Ground ↔ Ethernet ↔ ATLAS/QGC
```

In this diagram, arrows are bidirectional unless an actual physical/software barrier has been installed and verified. The QGroundControl/ATLAS user interface is **not** such a barrier.

Changing `SYSID_THISMAV`, the displayed vehicle ID, the UDP port or button visibility **does not reliably prevent commands**. QGroundControl can discover the new system ID from MAVLink heartbeats, and some MAVLink requests allow broadcast addressing. These settings are for identification and transport routing, not authorization.

### 2.2 Hardware-level receive-only option

If the HM30 UART carries *only* Pixhawk telemetry and flight-command isolation is required, one possible physical topology is:

```text
Pixhawk TX  ─────► HM30 Air RX      MAVLink telemetry towards ATLAS
Pixhawk GND ───── HM30 Air GND      Common signal reference
Pixhawk RX  X──── HM30 Air TX      Return data path disconnected
```

This prevents that HM30 UART link from delivering serial bytes **into the Pixhawk RX pin**, subject to actual wiring and any other available interfaces. The ground-side ATLAS computer may still transmit packets to the HM30; the physical UART return path is interrupted. Camera Ethernet/SIYI UDP control normally follows a separate IP path.

**Limitations:** This is a wiring design proposal, **not a guarantee of whole-aircraft command isolation**. Verify the flight controller's actual UART connector and signal direction, voltage levels, common ground, all alternate command paths and any possible MAVLink routing or bridge. Some configurations require a two-way link to request streams or maintain telemetry; removing a return line can affect initialization or message rates. Do not disconnect/reconnect wiring on an energized aircraft, and test only on the bench with propulsion made safe.

If Mission Planner uses a **different** direct USB/radio/UART link, it may remain bidirectional. If it uses the **same** HM30 return path, it will lose that command channel too. Inspect the exact wiring and endpoint topology; the application name does not determine link isolation.

### 2.3 Software filtering: future work, not a current capability

A potential future `telemetry-only` mode would need a deliberately implemented outbound policy applied at **every outgoing MAVLink path**, including automatic protocol traffic, forwarding and multiple links. A UI-only toggle or filtering a single button is insufficient. Requirements would include:

- inventory of all transmit paths used by the deployed QGroundControl build
- explicit drop/log policy for vehicle-targeted outgoing MAVLink
- confirmation that video/SIYI control stays independent
- startup/reconnect/failover and multi-vehicle testing
- external packet capture proving the effective policy under abnormal as well as normal states
- regression tests after upstream QGroundControl changes

An allowlist including `HEARTBEAT` is a **design choice**, not inherently receive-only. It still transmits data, and it must be assessed against the user's operational security requirement. **No such filter is part of v0.12.2.**

### 2.4 Safer bench verification

Before connecting ATLAS to a real flight controller for a test, remove propellers or otherwise make propulsion physically safe. Use a dedicated test environment. Verify the expected incoming MAVLink messages with a passive packet capture. If testing a proposed receive-only configuration, capture **outgoing traffic as well** and independently confirm that the Pixhawk cannot act on any command arriving through the relevant path. Do not attempt ARM/DISARM or mode-change tests on a ready-to-fly aircraft to prove a block.

## 3. Payload actuation remains intentional

Even with the Pixhawk's MAVLink receive line isolated, ATLAS may still command the SIYI A8 Mini over Ethernet/UDP:

- yaw/pitch motion and stop
- zoom
- photographs and recording
- camera-stream configuration
- gimbal working mode and output selection

These commands can change operator awareness, framing, available video and recording. Stop gimbal motion before changing route; verify the correct camera endpoint before issuing a command. The gimbal is independent of flight-control command isolation, not intrinsically harmless.

A reply from the SIYI endpoint indicates protocol-level communication; it does not by itself prove the requested mechanical state was reached or that camera storage accepted a recording.

## 4. KML is visualization, not geofencing

The 2D/3D KML layer manager can display polygons, lines, points, styles, hierarchical folders and altitude/extrusion. It **does not**:

- send a geofence to the Pixhawk
- instruct ArduPilot to stop at a boundary
- validate airspace restrictions against a current authoritative source
- prove that the imported KML includes every relevant operating limitation

Altitude handling is particularly important:

| KML mode | Meaning in current renderer | Risk |
| --- | --- | --- |
| `absolute` | Coordinate altitude interpreted in metres above mean sea level | Confusing AMSL with AGL shifts the displayed volume |
| `relativeToGround` | Altitude relative to modeled terrain | Elevation data errors influence visualization |
| `clampToGround` | Geometry follows the surface | A zero/ground altitude is not a clearance permission |
| `extrude` | Respects source KML extrusion flag | Lack of walls does not mean unrestricted flight |

The final v0.12.2 renderer deliberately **does not invent a 120 m extrusion height** for flat polygons. Earlier README notes describing that fallback are historical and superseded. Check the original KML, authoritative airspace data, coordinate/altitude reference and actual terrain coverage before relying on visualization for planning.

## 5. HM30 RF and network status

HM30 radio signal, camera IP reachability, RTSP video and Pixhawk MAVLink are related but **different** health signals:

| Observation | What it proves | What it does not prove |
| --- | --- | --- |
| Ground/Air radio indicator present | Some radio link is established | Camera data and flight telemetry are all valid |
| Ping to `192.168.144.25` | A8 IP endpoint replies | RTSP is decoding; MAVLink is valid |
| SIYI UDP control responds | Camera control endpoint communicates | RF RSSI/packet-loss figures are known |
| Decoded video visible | A video pipeline delivers frames | MAVLink telemetry and controller command policy are safe |
| QGC active vehicle telemetry | MAVLink is being parsed | Return-path commands are prevented |

In **Direct A8**, the current UI's HM30 `IP PATH OK` indicator reflects A8 control-path reachability. It is **not a measured HM30 RF-quality statistic**. Jetson RF metrics depend on an external backend and must not be confused with real onboard measurements when using the mock.

The working test system used HM30 Ground IP `192.168.144.12`, A8 IP `192.168.144.25` and MAVLink UDP port `19856`. The tested Pixhawk/HM30 serial bitrate was **57600 baud**; it is setup-specific. Never assume that an IP address, baud rate, antenna profile or `24KM` wireless-mode name guarantees operational range.

Changing HM30 radio modes, firmware or antenna arrangements can interrupt the link. Do not switch RF modes while airborne. Plan conservative range tests within applicable operating restrictions, maintain line of sight as required, and monitor the independent flight-control link and failsafe configuration.

## 6. Video and recording limitations

The SIYI RTSP stream uses GStreamer. A decoder can freeze, show corrupted frames or stop even while the camera IP responds. Timing and decoding are affected by packet delivery, codec, driver and Qt rendering path. The current Linux setup has shown periodic video judder; the experimental Qt6 renderer is not part of the stable baseline.

The three recorders have different failure domains:

- **A8 REC:** on-camera recording; check camera status and removable storage.
- **VIDEO REC:** local recording of the selected incoming video stream; depends on a valid video pipeline.
- **SCREEN REC:** records the rendered ATLAS window; can operate even without payload video, but may capture a black/frozen video panel and is limited by the actual capture/encoder throughput.

Before treating a file as evidence, confirm it exists, playback works and the recorded time span is correct. A recording button state alone is insufficient. A requested 120/144 fps capture rate is not a guarantee that the resulting file contains 120/144 unique frames per second.

## 7. Jetson / AI / tracking safety

The current code implements an optional Jetson TCP/JSON interface, model settings, AI/SAHI/depth/tracking controls and a simulated development backend. **Passing a mock-server demo is not validation of a real Jetson AI deployment.**

Target class, confidence, range, bearing and coordinates are dependent on the external source. They can be stale, missing, incorrectly registered or physically wrong. Automatic gimbal commands must not be activated based only on simulated detections, and should be bench-tested against bounded gimbal ranges and safe target conditions before field use.

The Direct A8 / Jetson route is manually selected. There is no automatic failover when a control socket or video stream fails. Verify the chosen endpoint after restarting the application; the selected route is persisted in settings.

## 8. Pre-operation minimum checks

Before field operation, the responsible operator should independently establish:

1. correct aircraft, Pixhawk serial port, HM30 Air/Ground and A8 physical wiring
2. expected control authority and whether the HM30 MAVLink path is physically/software isolated as required
3. working independent flight controller/RC link and appropriate aircraft failsafes
4. correct HM30 radio mode and antenna orientation, with no on-air configuration changes planned
5. correct Direct A8 or Jetson route and no stale RTSP endpoint
6. distinguishable camera-control, video and MAVLink telemetry status
7. map/KML altitude, zones and source validity checked against operational requirements
8. camera, VIDEO REC and SCREEN REC behaviour checked individually where required
9. available storage, power and link contingency procedures
10. compliance with applicable airspace and UAS operational rules

This checklist supplements, **not replaces**, aircraft manuals, approved procedures, safety analysis, flight permissions and trained operator judgment.

## 9. Known limitations and next engineering work

- A verified MAVLink egress block is absent; closing that gap requires implementation **and independent verification**.
- The UI deliberately hides conventional QGC flight controls but that is not a transport security mechanism.
- Real Jetson AI performance, tracking accuracy and latency still need hardware characterization.
- The Linux video renderer deserves further isolated evaluation before changing the stable video pipeline.
- KML accuracy depends on the source file, map terrain and coordinate reference.

---

**Related:** [README](../../README.md) · [Architecture](ARCHITECTURE.md) · [Setup](SETUP.md) · [Operation](OPERATION.md) · [Development](DEVELOPMENT.md) · [ATLAS Changelog](CHANGELOG.md)
