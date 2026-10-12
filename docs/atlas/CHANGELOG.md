# ATLAS changelog

> Consolidated ATLAS Video Control development history.
> **Current documented baseline:** v0.12.2 + SIYI video-pipeline checkpoint, commit `3af7c625b`.
> **Status:** pre-deployment development checkpoint, **not** a certified flight release.

This document records the **ATLAS-specific** evolution. The root-level [`CHANGELOG.md`](../../CHANGELOG.md) belongs to upstream QGroundControl and is intentionally maintained separately.

## How to read the version numbers

Early interface prototypes were labelled **v1.0–v1.6**. Afterwards, the project intentionally **reset its pre-deployment numbering to v0.7** and continued through **v0.12.2**. Thus, *v1.6 is historically older than v0.7* in this project's development sequence. These names describe successive internal packages/prototypes, **not a semantic-versioning claim** that the early `v1.x` release has a higher maturity level.

Some older notes describe **superseded implementation details** — for example, early SCREEN REC used `QWindowCapture`, and early KML versions included an optional 120 m fallback extrusion. The v0.12.2 source code and the current [Architecture](ARCHITECTURE.md), [Operation](OPERATION.md) and [Safety](SAFETY.md) guides take precedence over older descriptions.

Git history is the long-term archive. Historic documents were previously named `README_PAYLOAD_V*.txt` and `README_SIYI_VIDEO_PIPELINE.txt`. Their content can still be accessed in the checkpoint commit after removing the files from the active source tree:

```bash
git show 3af7c625b:README_PAYLOAD_V0_12_2.txt
git show 3af7c625b:README_SIYI_VIDEO_PIPELINE.txt
```

## Current baseline

### v0.12.2 — SIYI Video Full checkpoint

**Git checkpoint:** `3af7c625b` (`checkpoint: ATLAS payload v0.12.2 SIYI video full`).
**Stable integration branch:** `atlas-main`.
**Source branch at checkpoint:** `feature/payload-mode-v1`.

**Camera and recording**

- Configurable SCREEN REC: requested 20, 30, 60, 120 and 144 fps; Native/1080p/720p output; 8/12/16/20/40/60 Mbps options. The UI reports effective capture FPS, which can be lower than requested.
- Preserved screen recording of the ATLAS window independently of payload video and selected Direct A8/Jetson route.
- Preserved compatibility workaround using MP4 / MPEG-4 Part 2 software recording after a Linux HEVC/VAAPI failure.
- Exposed native QGC VIDEO REC controls for MP4, MOV and MKV, storage-limit options and video-save location.
- Added SIYI A8 main/record stream status queries including codec, resolution, bitrate and camera-reported FPS.
- Added the documented SIYI A8 codec settings subset: main-stream H.264/H.265, 720p/1080p and bitrate; recording-stream resolution/bitrate while preserving its current codec.
- Added gimbal FOLLOW / LOCK / FPV modes and selectable Ethernet/HDMI/CVBS video output.
- Deliberately **did not** add guessed/writable FPS, recording codec or undocumented 2K/4K/Boot Record/ISO/shutter/white-balance commands.

**KML final fidelity implementation**

- Hierarchical Document / Folder / Placemark layer tree with individual visibility controls.
- Polygon, LineString, Point and MultiGeometry handling.
- Shared Style / StyleMap lookup, KML colors, fill/outline, line widths and point labels/icons.
- Preservation of `absolute` (AMSL), `relativeToGround` (AGL) and `clampToGround` altitude interpretation.
- Extrusion only when the source KML requests it; terrain-aware 3D walls for filled features and wireframe edges for unfilled extruded geometry.
- Removed the earlier fallback rule that could create an arbitrary 120 m extrusion for a flat zone.
- Addressed Qt 6.11 AUTOMOC meta-type requirements for GeoMap scene/surface classes.
- KML import **remains visual-only**; no Pixhawk fence upload or aircraft enforcement was added.

**SIYI-focused live-video patch**

- Adjusted the RTSP path toward the SIYI-QGroundControl pipeline semantics: `rtspsrc` TCP interleaved, latency 17 ms, connection timeout, default RTSP parser behavior, ordinary decoder queue, `decodebin3` decoder and QGC sink-timing convention.
- Removed risky custom `leaky=downstream` and `max-size-buffers=2` **pre-decoder** queue configuration that can discard predictive H.265 reference material.
- Removed UDP/TCP fallback and extra retransmission/drop tuning from the specified SIYI RTSP path.
- Preserved non-RTSP handling and existing ATLAS payload UI/control features.
- Added SIYI video source patch/verifier and versioned installer utilities.

**Validation status:** Development build and A8/HM30 operation were tested on the operator's Linux system. An unresolved small periodic video judder remains. The Qt6 `qml6glsink` experiment is **not** part of this checkpoint. No new MAVLink transmit blocker exists.

### v0.12.1 — KML fidelity iteration

- Iterated from the first KML overlay toward Google-Earth-like layer hierarchy, source styles and altitude handling.
- Focused on real airspace KML exports, including multiple geometries and altitude-mode semantics.
- Superseded by the final v0.12.2 KML fidelity and Qt AUTOMOC fixes; use the latest source rather than early fallback-height text from prior package notes.

### v0.12.0 — recording settings, camera settings and first KML overlay

- Added configurable screen FPS, resolution and bitrate, retaining the separate SCREEN REC engine.
- Expanded local video recording controls.
- Added SIYI codec/status reporting and documented stream settings to the operator UI.
- Introduced user-imported KML operational zones for map viewing, with first-stage 2D polygon and 3D wall support.
- Explicitly documented that imported zones were **not** Pixhawk geofences.

### v0.11.3 — screen recorder encoder compatibility

- Fixed a Qt/VAAPI failure where HEVC encoding was selected automatically but the system rejected the requested encoder profile.
- Selected MP4 with MPEG-4 Part 2 software encoding for SCREEN REC.
- Ensured failed encoder sessions were reported as failures and incomplete output was removed, rather than falsely displaying `Saved`.
- Retained the independence of A8 REC, VIDEO REC and SCREEN REC.

### v0.11.2 — independent Qt Quick window capture

- Replaced dependence on the desktop `QWindowCapture` API with ATLAS-window frame capture using `QQuickWindow::grabWindow()`, `QVideoFrameInput` and `QMediaRecorder`.
- Kept screen capture operational with no A8, no Jetson and no live RTSP.
- Used a conservative initial capture rate of 20 fps; subsequent configurable high-FPS support arrived in v0.12.

### v0.11.1 — recording state and UI feedback

- Refined the three separate recorder states and operator controls.
- VIDEO REC used QGroundControl's native `VideoManager` and required a valid decoded video stream.
- SCREEN REC remained a local application-window recording path, distinct from camera REC.
- Recording status and elapsed timers were integrated in the compact video toolbar.

### v0.11.0 — three recording paths introduced

- Differentiated **A8 REC** (on-camera recording), **VIDEO REC** (received stream recorded locally) and **SCREEN REC** (full application-window capture).
- Maintained manual Direct A8 / Jetson routing and existing camera/gimbal controls.
- Introduced the independent `AtlasScreenRecorder` service.

### v0.10.1 — Direct A8/Jetson routing follow-up

- Continued and refined the manually selected, route-aware payload implementation established in v0.10.
- Preserved dedicated settings for camera host/port/RTSP and Jetson host/port/RTSP.
- Continued use of a shared `atlasPayload` facade; separate backend clients handled concrete transport.
- The revision's historical notes substantially repeated the v0.10 architecture. Do not infer a distinct hardware feature that is not documented.

### v0.10.0 — Direct A8 route and payload facade

- Introduced explicit operator choice between **DIRECT A8** and **JETSON** without automatic detection or fallback.
- Added `AtlasPayloadRouter` and `AtlasSiyiClient` for SIYI UDP control, enabling Direct A8 without a Jetson.
- Exposed gimbal rate/stop/center/presets, zoom, photo and camera recording to the UI.
- Preserved separate route endpoints in `QSettings`, with known A8 defaults `192.168.144.25:37260` and `rtsp://192.168.144.25:8554/main.264`.
- Kept the Direct A8 UI RAW-only and hid Jetson-dependent AI panels.
- Marked HM30 Direct-mode RF metrics as **unavailable** rather than fabricating live RSSI/loss measurements.
- Added/extended local SIYI UDP mock and protocol/CRC checks.

### v0.9 — interactive map in 2D and 3D

- Added independent mini-map and expanded-map 2D/3D switching.
- Used QGroundControl's native GeoMap terrain engine with aerial imagery and elevation tiles, not a simulated flat perspective.
- Preserved map gestures, zoom, orbit and centering; preferences persist.
- Online map detail depends on tile-provider coverage and available cache; this version did not imply uniformly high-resolution imagery everywhere.

### v0.8 — panel, toolbar and mini-map refinements

- Refined ATLAS panel typography, color gradient, status placement and video-profile control styling.
- Restored the retractable RAW / AI / RAW+AI video selector.
- Added mini-map zoom behavior up to the engine's configured maximum and continued use of available imagery providers.

### v0.7 — pre-deployment version-number reset

- Reset the label from the historical prototype `v1.x` sequence to `v0.7` because the project was not deployed.
- Refined the video-first interface, ATLAS AI icon, mini-map presentation and system-status panels.
- Used a new UI settings namespace to avoid inheriting incompatible saved panel-collapse states from earlier prototypes.

## Early UI prototypes (historical v1.x series)

These versions **precede v0.7**. Their features evolved into later versions, and some UI arrangements were subsequently revised.

| Prototype | Recorded change |
| --- | --- |
| **v1.0** | First payload-first Fly View: video as primary view, gimbal/zoom/camera controls, RAW/AI/PIP concepts, Jetson TCP/JSON protocol and mock; telemetry/events/target overlays |
| **v1.1** | Layout reorganization with larger mini-map, bottom telemetry/attitude/system panels, ATLAS custom toolbar override, demo-capable Jetson mock |
| **v1.2** | Video modes RAW/AI/RAW+AI, FAST/BALANCED/STABLE labels, fullscreen, mouse-wheel zoom and interactive map; explicit tracking ON/OFF |
| **v1.3** | Collapsible/persistent panels, clearer operator/config presets and less UI over the live video; manual tracking activation |
| **v1.4** | Compact AI settings, status chips, revised bottom dashboard, six telemetry readouts and voltage-only battery display |
| **v1.5** | Bottom-card layout, iconography and gimbal panel refinements; video selector repositioning |
| **v1.6** | Custom SVG icon set, timer formatting, visual balance and reset of stale layout preferences |

**Historical-version caution:** Early prototype mock outputs could show Jetson, HM30 or AI activity without attached hardware. Those demonstrations prove UI/protocol wiring, **not live detection or measured RF statistics**.

## Engineering notes beyond the checkpoint

The following were investigated and/or verified during integration but were **not software-release features added by a new commit after v0.12.2**:

- The live HM30-to-Pixhawk MAVLink setup was recovered with **57600 baud** on the tested UART. This is configuration evidence, not a generic SIYI/Pixhawk baud-rate specification.
- The actual MAVLink connection was found to remain capable of **outgoing transmissions**. A receive-only software gate is pending design/implementation/validation; merely changing `SYSID_THISMAV` is insufficient.
- Hardware/driver investigations isolated some Linux GPU-upload/video decoder corruption; software decoding and a more SIYI-like pipeline improved the baseline. The remaining periodic judder is unresolved.
- A future Qt6/GStreamer sink experiment should be carried out on a feature branch without rewriting the validated baseline.

## Maintaining this changelog

For future validated ATLAS revisions, add a new section **above** the prior release, noting:

1. version/tag/commit, and whether the change reached `atlas-main`
2. concrete changes that are actually committed
3. hardware and simulation environments used to verify them
4. remaining limitations and safe rollback reference

Avoid marketing claims of flight-ready behaviour, real Jetson detections or MAVLink isolation without direct evidence. Preserve QGroundControl's own root `CHANGELOG.md` separately.

---

**Related:** [Project README](../../README.md) · [Architecture](ARCHITECTURE.md) · [Setup](SETUP.md) · [Operation](OPERATION.md) · [Development](DEVELOPMENT.md) · [Safety](SAFETY.md)
