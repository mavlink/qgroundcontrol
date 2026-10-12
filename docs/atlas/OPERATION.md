# Operator guide

> **ATLAS Video Control v0.12.2** · Operator-facing controls and expected behaviour
> Reference: [`atlas-main`](https://github.com/franciscomariquitos/ATLAS-QGroundControl/tree/atlas-main), baseline commit [`3af7c625b`](https://github.com/franciscomariquitos/ATLAS-QGroundControl/commit/3af7c625b517bf3e192c5a8d3abfc80e33b6fbde)
> Scope: the current Direct A8 workflow and the software interface for the optional Jetson route

ATLAS Video Control is a **video-centric UAV operator interface** built on QGroundControl. This guide explains what the controls do, how the three recording paths differ, how to use operational KML layers, and how to interpret connection and status indications. Hardware installation and network diagnostics belong in [Setup](SETUP.md); protocol and source-code design belong in [Architecture](ARCHITECTURE.md).

> [!IMPORTANT]
> **ATLAS is not a certified flight-control isolation system.** Although its custom Fly View is payload-focused, the underlying QGroundControl MAVLink transport can transmit commands to a connected flight controller. Do not infer receive-only operation from the absence of flight-control buttons. Read [Safety and limitations](SAFETY.md) before using a live Pixhawk.

## 1. Capability and validation status

| Capability | Current v0.12.2 status | Operator implication |
| --- | --- | --- |
| Direct A8 camera/gimbal | Implemented; used with the SIYI A8 Mini/HM30 | Available without Jetson |
| A8 live RTSP video | Implemented; hardware link exercised | Separate from camera-command connectivity |
| Pixhawk MAVLink telemetry | Received successfully through HM30 on the tested setup | Link is **not** receive-only by default |
| A8 REC, VIDEO REC, SCREEN REC | Three separate application controls | Different recording locations and dependencies |
| 2D/3D maps and KML overlay | Implemented in the ATLAS Fly View | Visual reference, not aircraft geofencing |
| Jetson TCP/JSON controls | Implemented with a development mock | Onboard Jetson deployment has not been hardware-validated here |
| AI inference, depth and tracking on a real Jetson | Integration interfaces exist | Behaviour depends on an external Jetson service; do not assume real inference is active |
| Automatic payload-route failover | **Not implemented** | Select the intended route manually |
| MAVLink transmit block / telemetry-only software mode | **Not implemented** | A separate safety measure is required for true receive-only operations |

## 2. Start an operator session

1. Power and connect the selected payload and ground link according to [Setup](SETUP.md).
2. Start the ATLAS application. On the validated development laptop:

   ```bash
   cd ~/ATLAS/ATLAS-QGroundControl
   ./build/Release/ATLAS-QGroundControl
   ```

3. Open **PAYLOAD** in the top toolbar. Confirm the selected route; it is stored between sessions and may initially be **JETSON** on a fresh installation.
4. For the current A8/HM30 setup, select **DIRECT A8**. Confirm the stored A8 host, UDP port and RTSP URL, and press **APPLY A8** if you make changes.
5. Check independently that the **camera-control connection**, **live video** and, when intentionally connected, **MAVLink telemetry** are functioning. A successful indication in one path does not prove the others.
6. Before recording an operational session, make a short recording test and check where the file was saved.

### 2.1 Direct A8 profile

The known working/default parameters are:

| Field in PAYLOAD | Value |
| --- | --- |
| A8 IP | `192.168.144.25` |
| A8 UDP control port | `37260` |
| Direct A8 RTSP URL | `rtsp://192.168.144.25:8554/main.264` |

These values correspond to the current hardware setup, not to every SIYI deployment. The selected route and endpoint profiles are saved by ATLAS using Qt settings.

### 2.2 Jetson profile

The **JETSON** route has its own control host, TCP port and processed-video RTSP URL. The development mock defaults to `127.0.0.1:45900` for control; it is **not** an onboard Jetson address. When connecting to a real Jetson, specify the actual reachable host, listening port and stream URL before selecting or testing that route.

Changing routes disconnects the previously selected payload backend and updates the active video source. **No automatic fallback** occurs if the selected backend stops responding.

## 3. Read the main interface

| Interface area | Purpose |
| --- | --- |
| **Top toolbar / PAYLOAD** | Shows the selected route, opens route configuration, and displays compact status indications |
| **Video selector** (upper-left overlay) | RAW/AI/RAW+AI, video profile where available, local recording, screen recording, media settings and fullscreen |
| **Left camera rail** | Gimbal interaction, zoom, recenter, photo and A8 camera recording |
| **Mini-map** (lower-left) | Aircraft/map context, 2D/3D selection, KML access and navigation |
| **AI / Tracking** (right) | Optional Jetson processing and target selection; hidden or unavailable in Direct A8 |
| **Bottom panels** | Vehicle Telemetry, Attitude/Compass, Gimbal, Events and System Status |

Panel headers can be collapsed to recover more video space. The interface saves most expand/collapse preferences between launches. The payload mode selection is saved independently.

### 3.1 Connection indications are not equivalent

**A8 online** refers to the corresponding camera-control/backend status. **Video** requires an active decoded stream. **MAVLink telemetry** requires QGroundControl to have received vehicle data through a separate link. They may fail independently.

In **Direct A8**, the HM30 indication may report **`IP path OK`** because the A8 control path is responding. **This is not a measured HM30 RF quality value**: live RSSI, packet loss and RF latency are not exposed by that Direct A8 implementation. In this route the Jetson indicator is **BYPASSED**, not defective. AI features are unavailable by design.

In **Jetson** mode, the client can display Jetson/HM30 metrics provided by the external service. Their validity depends on the real Jetson backend reporting them; the mock produces simulated status.

## 4. Operate the A8 camera and gimbal

The following controls apply when the selected backend reports the A8 as connected.

| Control | Effect / behaviour |
| --- | --- |
| **GIMBAL** | Enables or disables drag-based gimbal rate control over the video image |
| **Drag with GIMBAL ON** | Commands yaw or pitch according to the dominant drag direction and sensitivity |
| **Release the mouse** | Stops the active gimbal rate command |
| **Double-click with GIMBAL ON** | Stops gimbal movement and sends recenter |
| **ZOOM + / ZOOM −** | Commands incremental zoom in the indicated direction |
| **Mouse wheel over video** | Commands zoom while a camera backend is connected; also supported over fullscreen video |
| **CENTER** | Stops current movement, then sends gimbal-center command |
| **PHOTO** | Requests a still image from the A8 |
| **REC** (left rail) | Starts/stops **A8 camera recording**, not PC-side recording |
| **Gimbal presets** | **FWD**, **DOWN**, **LEFT**, **RIGHT**; **TGT** is enabled only with a valid Jetson tracking target |
| **Sensitivity** | Adjusts mouse-driven gimbal input scaling (0.25×–2.50×) |

The Gimbal panel in the bottom strip displays reported **yaw/pitch** values and a compact directional indicator. A command being sent does not by itself prove that the camera physically reached the requested orientation. Stop gimbal interaction before switching routes or closing the session.

### 4.1 Camera settings

Open the **gear icon beside the recording controls** to access **MEDIA & CAMERA SETTINGS**, then locate **SIYI A8 CAMERA**. In **DIRECT A8** mode:

1. Press **Refresh camera** to request live codec information.
2. Verify the displayed **MAIN** and **RECORD** stream codec, dimensions, bitrate and reported FPS.
3. For **MAIN**, choose H.264/H.265, 720p/1080p and a supported bitrate in kbps, then press **Apply MAIN**.
4. For **RECORD**, choose 720p/1080p and bitrate, then press **Apply REC**. The currently reported recording codec is preserved.
5. Change **Gimbal mode** using FOLLOW, LOCK or FPV; press its **Apply** control.
6. Change **Video output** using ETHERNET, HDMI or CVBS; press **Apply**. A camera restart may be required before an output change is effective.
7. Refresh codec/status again to confirm what the camera actually accepted. Change one parameter at a time.

**Documented API limits:** the SIYI SDK status query reports FPS, but the verified stream-write command does not offer a writable FPS parameter. Likewise, the current Direct A8 implementation does **not** expose independently verified 2K/4K recording-mode writes, recording-codec selection, Boot Record, ISO, shutter or white-balance commands. The presence of those settings in SIYI UniGCS does not mean ATLAS can safely send them.

## 5. Video display and stream selection

The video toolbar behaves differently depending on payload route:

| Mode | Available selection | Meaning |
| --- | --- | --- |
| **DIRECT A8** | **RAW** | Uses the A8 live stream; no Jetson processing is selected |
| **JETSON** | **RAW**, **AI**, **RAW+AI** | Requests the corresponding mode from the Jetson service; the actual rendered content depends on that service |
| **JETSON** | **FAST**, **BALANCED**, **STABLE** | UI labels for Jetson video-profile requests; not proof of a measured RF bandwidth/latency improvement |

**Fullscreen** enlarges the current video display. If the selected route has no available video, fullscreen may be unavailable. Use the application's video/fullscreen controls to return to the operator layout; do not assume that toggling fullscreen changes the camera source.

**RAW+AI** maps to the payload API's `PIP` stream mode. In Jetson mode it is a *requested output composition*, not local ATLAS object detection: the processed stream and inference must be produced by the external service.

## 6. Recording: choose the correct path

| Control | Recording location | What it captures | Principal dependency |
| --- | --- | --- | --- |
| **A8 REC** | Camera/payload storage | Camera recording | A8 connection and camera recording/storage functionality |
| **VIDEO REC** | Operator PC, QGroundControl video-save directory | Incoming video stream | Active decoded video |
| **SCREEN REC** | Operator PC, ATLAS screen-recording directory | Rendered ATLAS window, including visible UI | ATLAS Qt Quick window and working local encoder |

These three recordings are independent. Starting one does **not** automatically start the others.

### 6.1 A8 REC

Press **REC** in the left camera rail. The button reflects the recording state reported through the payload client; a running counter is shown while active. Press it again to stop. Before relying on evidence collection, verify the resulting file on the camera's storage and ensure that enough capacity is available.

### 6.2 VIDEO REC

Press **VIDEO REC** in the upper video toolbar while decoded video is available. During startup the button may show **VIDEO...**. While recording, it displays the elapsed time. Press it again to stop.

If no decoded stream exists, the button indicates **VIDEO WAIT** and cannot start a new local stream recording. This is a different condition from the camera-control endpoint responding to pings or SIYI status requests.

In **MEDIA & CAMERA SETTINGS → VIDEO RECORDING (LOCAL STREAM)**, set the container (**MP4 / MOV / MKV**), optional storage limit and maximum storage in MB. The active save directory is displayed there. Source FPS/bitrate are retained; changing SCREEN REC FPS has no effect on VIDEO REC.

### 6.3 SCREEN REC

Press **SCREEN REC** in the upper video toolbar. The button indicates **SCREEN...** during encoder startup and a red recording state with elapsed time once the recorder confirms it is active. Press again to stop.

Screen recording can continue showing the ATLAS interface even without a live A8/Jetson video stream. On the tested Linux configuration it writes files to the system **Movies** directory under `ATLAS/`, normally:

```text
~/Videos/ATLAS/ATLAS_Screen_YYYYMMDD_HHMMSS.mp4
```

In **MEDIA & CAMERA SETTINGS → SCREEN RECORDING**:

| Setting | Choices | Notes |
| --- | --- | --- |
| Frame rate | 20, 30, **60** (default), 120, 144 fps | Requested capture rate; effective FPS is displayed while recording |
| Resolution | Native, 1080p, 720p | Output capture-size selection |
| Bitrate | 8, 12, **16** (default), 20, 40, 60 Mbps | Software-encoder target bitrate |

The settings are disabled during recording. High requested capture rates do not guarantee the same **effective FPS**, since window readback and encoding depend on hardware. This release uses a software MPEG-4 Part 2 video encoder to avoid an observed HEVC/VAAPI compatibility failure. If the encoder fails, the application reports the error rather than presenting the failed session as saved.

**End-of-session check:** stop each recording explicitly, wait for file finalization, locate the output and play a short section to confirm it is readable. A button state alone is not a file-integrity test.

## 7. Maps and airspace KML

### 7.1 Mini-map and expanded map

The mini-map is interactive while video is primary:

| Action | Effect |
| --- | --- |
| Short left click on map | Expand map / swap video into the smaller view |
| Left drag | Pan map |
| Mouse wheel | Zoom |
| Right drag in 3D mode | Rotate/orbit/adjust view |
| **2D / 3D** | Switch the map presentation mode |
| **Center** | Recenter on the active vehicle when vehicle position is available |
| **+ / −** | Zoom in/out |
| **KML** | Open the airspace layer manager |

In the expanded map, use **← VIDEO** to return video to the main view. Both miniature and expanded maps have 2D/3D controls. Terrain and map imagery quality depend on QGroundControl's map engine, chosen map provider and available cached/downloaded tiles.

### 7.2 Import and manage KML

1. Open **KML** from the mini-map or expanded map.
2. Press **Import KML** (or **Import another**) and select a local `.kml` file.
3. Use **Focus** to move the map toward the imported geometry.
4. Use **MASTER ON / MASTER OFF** to control visibility of the loaded overlay without clearing it.
5. Use **All ON / All OFF** for bulk layer visibility.
6. Expand/collapse folders in the layer tree and use each checkbox to control specific folders or placemarks.
7. Use **Clear** to remove the loaded KML data from the current session.

The layer tree follows the KML Document/Folder/Placemark structure. Rendered geometry can include polygons, lines and points; styles, fill/outline, altitude modes and extrusion are derived from the KML file, not from a default invented height. An unfilled polygon can appear as wireframe rather than a solid wall. The same operational layers can appear in the 2D and 3D map representations.

> [!WARNING]
> **KML is display-only.** It does not upload a fence, validate airspace permission, control the Pixhawk or prevent entry into a restricted area. Visual alignment, elevation sources and file contents must be independently checked before operational use.

## 8. Telemetry, attitude, events and system status

The **Telemetry** panel reads data from QGroundControl's active MAVLink vehicle. In the current UI it shows:

| Field | Source/interpretation |
| --- | --- |
| **Alt** | Vehicle relative altitude |
| **Speed** | Vehicle ground speed |
| **Heading** | Vehicle heading |
| **Dist to Home** | Vehicle distance-to-home estimate |
| **Battery** | **Voltage**, not battery percentage |
| **GPS** | Reported satellite count |

Values such as `--` indicate that the corresponding vehicle data are not available to that UI field. The **Attitude / Compass** panel presents orientation information from the active vehicle. The **Events** panel presents recent payload/UI events; these are not a replacement for formal flight logs.

The **System Status** panel shows Jetson, HM30 and A8 information. Interpret the values according to the selected route, particularly the Direct A8 HM30 **`IP path OK` / `RF stats unavailable`** distinction described above.

**MAVLink warning:** receiving telemetry through the HM30 does not prevent ATLAS/QGroundControl from transmitting MAVLink. The verified baud rate for this specific HM30↔Pixhawk UART was **57600**; this is not a general rule for other installations. See [Setup](SETUP.md) and [Safety](SAFETY.md).

## 9. Optional Jetson / AI controls

When **JETSON** is selected, the right-hand interface exposes **AI** and (when explicitly enabled) **Tracking** sections. The software currently provides controls for:

| Control | Intended Jetson-facing request |
| --- | --- |
| **YOLO** | Enable/disable detector processing |
| **SAHI** | Enable/disable slicing-based processing |
| **Depth / Distance** | Enable/disable corresponding depth/distance processing |
| **Confidence** | Adjust requested detector confidence threshold |
| **AI FPS** | Adjust requested inference rate |
| **Tracking mode** | Expose tracking controls and accept target selection |
| **Click** | Select a point on the video as a tracking target |
| **ROI** | Drag a region of interest for tracking |
| **Auto** | Request automatic gimbal tracking, when a valid target exists |
| **Clear** | Clear an active target/tracking request |

A target card may show class, ID, confidence, distance, bearing and geographic position **only when the Jetson backend supplies valid target data**. The UI does not derive these measurements by itself. A development mock can demonstrate the interface without producing real detection results. In **DIRECT A8**, the Jetson AI panels are not available.

For integration/testing of the mock server, refer to [Setup](SETUP.md) and [Development](DEVELOPMENT.md). Do not enable autonomous gimbal tracking near people or obstacles based solely on mock behaviour.

## 10. Session checklist

### Before use

- [ ] Select and confirm the intended PAYLOAD route.
- [ ] Confirm A8 control, video and telemetry **independently**.
- [ ] Confirm the flight-controller communications safety arrangement (see [Safety](SAFETY.md)).
- [ ] Confirm the map location and any imported KML layers against independent references.
- [ ] Verify recording destinations, free space and a short test clip.
- [ ] If using Jetson features, confirm real backend connectivity and distinguish genuine sensor data from mock values.

### After use

- [ ] Stop gimbal control and all three recording paths that were used.
- [ ] Verify recording files exist and can be played.
- [ ] Preserve relevant evidence/logs outside temporary storage.
- [ ] Disconnect the payload and any MAVLink link as required by the operating procedure.

## 11. First-response troubleshooting

| Symptom | First distinction | Next reference |
| --- | --- | --- |
| Camera responds but video is black | SIYI UDP control does not prove that RTSP/GStreamer is decoding frames | [Setup — video tests](SETUP.md) |
| A8 shows offline | Confirm route, IP network, camera power, SIYI control endpoint | [Setup — network](SETUP.md) |
| Video reports `VIDEO WAIT` | No active decoded stream; VIDEO REC cannot start | [Setup — GStreamer](SETUP.md) |
| SCREEN REC never becomes active | Local Qt capture or encoder problem; inspect terminal error | [Setup — recording](SETUP.md) |
| No telemetry but camera works | MAVLink/HM30 datalink is distinct from A8 Ethernet traffic | [Setup — MAVLink](SETUP.md) |
| HM30 says `IP path OK` but RF quality is unknown | Direct A8 indicator measures IP-camera reachability, not RSSI/loss | [Architecture](ARCHITECTURE.md) |
| KML height or outline looks wrong | Inspect file altitude mode, extrusion, styles and map terrain; avoid adding an artificial height | [Architecture](ARCHITECTURE.md) |
| AI controls inactive in Direct A8 | Jetson functions are deliberately unavailable in this route | [Architecture](ARCHITECTURE.md) |

## Related documentation

- [Project overview](../../README.md)
- [Architecture](ARCHITECTURE.md)
- [Setup and configuration](SETUP.md)
- [Development](DEVELOPMENT.md)
- [Safety and limitations](SAFETY.md)
- [ATLAS changelog](CHANGELOG.md)

---

*This guide documents the actual v0.12.2 operator UI and code paths, not a certification or guarantee of end-to-end flight readiness. Source of truth for UI controls: `custom/atlas/src/FlyViewCustomLayer.qml`, `FlyView.qml`, `FlyViewToolBar.qml`, `AtlasPayloadRouter.*`, `AtlasSiyiClient.*`, `AtlasScreenRecorder.*` and the current QGroundControl video/vehicle interfaces.*
