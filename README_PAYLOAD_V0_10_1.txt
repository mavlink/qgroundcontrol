ATLAS VIDEO CONTROL v0.10.1
=========================

Base
----
This package is built directly on top of the validated v0.9 interface.
The v0.9 2D/3D miniature/full-map implementation and the visual layout are
preserved. v0.10.1 adds a manual payload-routing layer; it is not a redesign.

Payload routes
--------------
ATLAS-QGroundControl now supports two explicit operator-selected routes:

1) DIRECT A8
   ATLAS-QGC -> HM30 network -> A8 Mini

   Control backend:
       SIYI SDK over UDP
       Default A8 IP:      192.168.144.25
       Default A8 port:    37260

   Video backend:
       Default RTSP: rtsp://192.168.144.25:8554/main.264

   Available:
       RAW video
       gimbal rate + stop
       center
       FWD / DOWN / LEFT / RIGHT presets
       zoom
       photo
       recording
       A8 gimbal/zoom/recording status
       QGC MAVLink telemetry/map, independently of payload route

   Unavailable by design:
       Jetson AI / YOLO / SAHI / depth / AI tracking
       Jetson health
       HM30 RF metrics (the current direct implementation only knows whether
       the A8 IP control path is responding; it does NOT invent RSSI/loss data)

2) JETSON
   Existing v0.9 Jetson architecture is preserved:
       ATLAS-QGC -> Jetson TCP/JSON control
       processed RTSP -> QGC
       AI / tracking / depth / Jetson and HM30 telemetry

Manual-only safety rule
-----------------------
The route NEVER changes automatically.

- No Jetson auto-detection.
- No Direct-A8 auto-detection used to select the route.
- No automatic fallback from Jetson to A8.
- No automatic fallback from A8 to Jetson.
- A disconnected selected route remains selected and is shown as disconnected.
- The operator changes it explicitly from the PAYLOAD control in the top bar.

Both route profiles are persisted independently in QSettings:
    ATLAS/Payload/mode
    ATLAS/Payload/directA8Host
    ATLAS/Payload/directA8Port
    ATLAS/Payload/directRtspUrl
    ATLAS/Payload/jetsonHost
    ATLAS/Payload/jetsonPort
    ATLAS/Payload/jetsonRtspUrl

The first-run mode remains JETSON for backward compatibility with v0.9. Once
the operator selects DIRECT A8 or JETSON, that manual selection persists.

Jetson RTSP migration
---------------------
v0.9 used QGC's global RTSP URL. On first v0.10.1 start, if no Jetson RTSP profile
has been saved yet, a non-empty existing QGC RTSP URL is copied into the Jetson
profile (unless it is the known Direct-A8 URL). No Jetson IP/RTSP is guessed.

QML architecture
----------------
The UI now talks only to:

    atlasPayload

AtlasPayloadRouter routes common commands to either:

    AtlasSiyiClient      (DIRECT A8)
    AtlasJetsonClient    (JETSON)

This keeps FlyView controls independent of the selected backend.

Video switching
---------------
Changing the manual route changes QGC's configured RTSP URL and RTSP source.
The router also explicitly starts/stops QGC's existing VideoManager after the
settings change, so switching from a disabled/unconfigured route cannot leave
the previous route's picture running or require a second settings change.

DIRECT A8 UI behaviour
----------------------
- AI side panel is hidden to give the video more room.
- Video selector becomes compact RAW-only + fullscreen + collapse.
- Jetson is shown as BYPASSED, not failed.
- HM30 shows IP PATH OK / NO A8 LINK and explicitly says RF stats unavailable.
- AI is shown as N/A.
- A8 uses live SIYI replies for zoom, yaw/pitch, and recording state.
- TGT gimbal preset remains unavailable because target tracking is Jetson-only.

SIYI implementation
-------------------
The Direct A8 backend implements the SIYI gimbal/camera SDK frame used by the
A8 Mini over UDP, including CRC16/XMODEM and the command subset needed by ATLAS:

    0x01 firmware/liveness
    0x05 manual zoom
    0x07 gimbal rotation rate
    0x08 center
    0x0A configuration / recording state
    0x0B function feedback
    0x0C photo / recording
    0x0D gimbal attitude
    0x0E absolute yaw/pitch preset
    0x18 current zoom

The implementation was checked against SIYI A8 Mini documentation / external
SDK material. The known SIYI SDK CRC example
    55 66 01 01 00 00 00 05 01 -> CRC 0x648D -> bytes 8D 64
is included in the verifier.

Offline test helpers
--------------------
Jetson mock:
    python3 tools/atlas/mock_jetson_server.py

Direct A8/SIYI UDP mock:
    python3 tools/atlas/mock_siyi_camera.py

The SIYI mock defaults to 127.0.0.1:37260. For a local Direct-mode control test,
set the Direct A8 host temporarily to 127.0.0.1 in the PAYLOAD popup. The video
URL can stay separate; the mock only simulates SIYI camera/gimbal control.

Apply / verify / build
----------------------
From the QGroundControl repository root:

    unzip -o ~/Downloads/ATLAS_Payload_Mode_v0_10_1.zip -d ~/ATLAS/ATLAS-QGroundControl

    cd ~/ATLAS/ATLAS-QGroundControl
    python3 tools/atlas/verify_payload_bundle.py

v0.10.1 adds new C++ source files, therefore CMake MUST be reconfigured:

    PATH="$HOME/Qt/Tools/CMake/bin:$PATH" ~/Qt/6.11.1/gcc_64/bin/qt-cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

    ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Run:

    ./build/Release/ATLAS-QGroundControl

Current hardware test
---------------------
For the user's current A8 + HM30 setup:

1. Open PAYLOAD in the top bar.
2. Select DIRECT A8 manually.
3. Defaults should already be:
       192.168.144.25
       37260
       rtsp://192.168.144.25:8554/main.264
4. Do not enable any automatic mode selection; none is implemented.
5. Verify video, then gimbal, center, zoom, photo and REC one at a time.

Notes
-----
- A full Qt 6.11.1 C++/QML compilation cannot be performed by the offline bundle
  verifier. Run the configure/build commands above on the ATLAS development PC.
- HM30 RF RSSI/loss in DIRECT A8 mode is intentionally not fabricated. The
  application only reports end-to-end A8 SIYI control reachability in that mode.


PATCH v0.10.1
------------
Fixes the C++ build blocker in AtlasSiyiClient::connectToHost where a local variable named endpointChanged hid the Qt endpointChanged() signal. The variable is now endpointHasChanged. No payload-routing behavior or v0.9 UI/map behavior was changed.
