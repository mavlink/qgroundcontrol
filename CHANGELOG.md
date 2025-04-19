# QGroundControl Change Log

> **Note:** This file only contains high‑level features or important fixes.

## [5.0] – Daily Build

- **New UI**: Combined compass + attitude instrument for enhanced navigation.
- **Instrument Selection**: Click on desktop or long‑press on mobile to switch instruments.
- **MAVLink Actions**:
  - Fly View & Joystick custom actions renamed from “Custom Actions” to **Mavlink Actions** (move your JSON files accordingly).
  - Support for setting individual MAVLink message rates in the Inspector.
  - Enabled MAVLink 2 signing.
- **Battery Display**: Dynamic bars with configurable thresholds (100%, Config 1, Config 2, Low, Critical).

---

<details>
<summary><strong>4.1</strong></summary>

### [4.1.2] – Not yet released
- **Bugfix**: Radio setup – double‑send of `MAV_CMD_PREFLIGHT_CALIBRATION` causing “Unable to send command.”

### [4.1.1] – Stable
- **Fix**: TCP link communications.

### [4.1.0]
- **Camera**: Support simple cameras (only `DIGICAM_CONTROL`) in Photo/Video control.
- **Parameters**:
  - Load from file even if missing on vehicle.
  - Diff dialog + selective param upload.
- **Video Streaming**: Capture individual images from the stream.
- **Fly**: Long‑press arm = Force Arm; click again to arm.
- **VTOL**:
  - Transition‑distance setting for takeoff/landing patterns.
  - Improved VTOL support throughout.
- **Maps**: Zoom up to level 23 (even without tiles).
- **Settings/Mavlink**: Forward traffic to specified UDP port.
- **Terrain Protocol**: Query GCS for terrain data (`TERRAIN_FRAME`) in mission planning.
- **Plan**:
  - VTOL Landing Pattern.
  - KML export improvements for 3D verification.
  - Terrain Profile with collision indications.
- **Fly**: Rearchitected view & controls for custom builds.

</details>

---

<details>
<summary><strong>4.0</strong></summary>

### [4.0.9] – Not yet released
- Don’t auto‑connect to second Cube Orange/Yellow composite port.
- **Plan**: Fix mission commands with altitude but no lat/lon.
- Fix view switching break after altitude‑mode warning.

### [4.0.8] – Stable
- **iOS**: Update file storage for Files app.
- **Mobile**: Fix Log Replay status‑bar file selection.

### [4.0.7] – Stable
- Fix video page sizing.
- **Virtual Joystick**:
  - Right‑stick centering fix.
  - Rover/sub reverse‑throttle support.
- Fix display of multiple ADSB vehicles.

### [4.0.6] – Stable
- **Analyze/Log Download**: Fix mobile download.
- **Fly**: Continue Mission & Change Altitude now available after pause.
- **PX4 Flow**: Video display fix.

### [4.0.5] – Stable
- **Solo**: Fix mission upload failures.
- **Plan**: Crash fix for Create Plan → Survey (fixed‑wing).

### [4.0.4]
- **Mobile File Save**: Incorrect extension fix.
- **Radio Setup**: Spektrum bind fix.
- **Plan/Fly**: Restore waypoint number display.

### [4.0.3]
- **Plan**:
  - Optional takeoff item.
  - Enforce takeoff before other items.
- **Video**: Low‑latency mode option.
- **ArduPilot**: Firmware list generation fix.

### [4.0.2]
- Fix MAVLink V2 negotiation via capability bits.
- Fix `AUTOPILOT_VERSION` response wait.
- **ArduPilot**: More reliable fence/rally support.

### [4.0.1]
- Fix ArduPilot mission‑item tracking in Fly view.
- Fix ADSB display.
- Fix Plan view map positioning.
- Fix Windows `0xCC000007B` startup error (VC++ runtimes).

### [4.0.0]
- **Flight**: ROI option + Cancel ROI toggle + ROI‑affected path color.
- **Windows**: 64‑bit builds, Qt 5.12.5.
- **ADSB**: SBS server & USB SDR dongle support.
- **Toolbar**: Scrollable on small screens.
- **Plan View**: New initial‑plan UI.
- **Editing Tools**: Corridor & Polygon click‑trace.
- **Performance**: No mobile path‑length limit.
- **ArduPilot**:
  - Motor Test page.
  - Copter: Simple/Super‑Simple modes, advanced tuning, 3.5+ support.
  - Plane: 3.8+ support.
  - Rover: Frame setup & 3.4+ support.
  - Airframe UI overhaul.
- **Plan/Pattern**: Named presets (Survey).
- **ChibiOS**: Improved bootloader support.
- **Misc**:
  - Camera API open to all firmwares.
  - Configurable MAVLink stream rates.
  - Structure Scan rewrite (old plans must be recreated).
  - Object‑avoidance, joystick action modes.
  - UDP RTP H.265, English‑only Linux TTS.
  - Automated Crowdin localization.
  - Korean & Chinese font improvements.
  - New QtQuick MAVLink Inspector.

</details>

---

<details>
<summary><strong>3.5</strong></summary>

#### [3.5.5]
- Fix MAVLink `memset` causing wrong ArduPilot GotoLocation commands.
- Disable Pause when fixed‑wing is landing.

#### [3.5.4]
- Update Windows drivers.
- Add FMUK66 flashing support.
- Guard against null GStreamer geometry.
- `.apj` file‑selection for custom flash.

#### [3.5.3]
- RTK Survey‑In limit → 0.01 m.
- Windows driver‑detection logic fix.
- GeoFence vertex crash fix.
- **PX4:** Add `MC_YAW_FF` to PID Tuning.
- **ArduPilot:** Fix bad chars in param‑file save.

#### [3.5.2]
- Fix Ubuntu AppImage startup.

#### [3.5.1]
- Update Windows USB drivers.
- Add CubeBlack Service‑Bulletin check.
- Fix PX4/ArduPilot logo in toolbar.
- OfflineMaps tile‑set count fix.

#### [3.5.0]
- **Plan GeoFence:** Fix loading from 3.4.
- **Structure Scan:** Height loading fix.
- **ArduPilot:** Home‑position fix (Issue #6840).
- Multi‑component param loading fix.
- Mobile file‑dialog delete fix.
- Fixed RTK station setting.
- Airmap integration.
- Add `ESTIMATOR_STATUS` FactGroup.
- Chinese/Turkish + partial German localization.
- Distance‑to‑GCS & Heading‑to‑Home instruments.
- Position dialog on polygon vertices.
- **Fixed‑Wing Landing**: Stop photo/video support.
- SHP polygon loading.
- Settings version bump (reset defaults).
- Orbit rotation direction toggle.
- Taisync 2.4 GHz ViUlinx HD link.
- NMEA GPS UDP port option.

</details>

---

<details>
<summary><strong>3.4</strong></summary>

#### [3.4.4]
- Notify desktop if newer version available.
- Fix Multi‑Vehicle Start/Pause (Issue #6864).

#### [3.4.3]
- Resume Mission display fix (Issue #6835).
- Home‑Position altitude fix (Issue #6846).

#### [3.4.2]
- Fix new mission items altitude = 0 bug (Issue #6823).

#### [3.4.1]
- Crash on quick terrain‑follow move fix.
- Terrain‑follow rate fields swapped fix.

</details>
