ATLAS VIDEO CONTROL v0.11.1
=========================

Base
----
v0.11.1 is built directly on the validated v0.10.1/v0.9 interface. The manual
DIRECT A8 / JETSON payload routing, the 2D/3D map, compact panels, AI behaviour,
and the existing A8 camera REC control are preserved.

v0.11.1 recording-state fix (built on v0.11)
---------------------------------------------
1) A8 REC
   Existing payload-camera recording command.
   - DIRECT A8: command goes directly to the A8 via SIYI UDP.
   - JETSON: command is routed through AtlasJetsonClient.
   - Recording is performed by the payload/camera path, not by the local PC.

2) VIDEO REC
   Local recording of the active QGroundControl video receiver.
   - Uses QGroundControl's existing VideoManager startRecording/stopRecording.
   - DIRECT A8: records the active A8 RTSP stream.
   - JETSON: records the currently active Jetson-delivered stream.
   - The recording does not include the ATLAS toolbar/map/panels.
   - The QGC video save path must already be configured in Settings.

3) SCREEN REC
   Local recording of the ATLAS-QGroundControl application window.
   - Captures the operator view: video + ATLAS overlays + map + telemetry + UI.
   - Independent of DIRECT A8 / JETSON payload routing.
   - Uses Qt 6.11 QWindowCapture + QMediaRecorder with the FFmpeg backend.
   - Video-only capture: microphone/system audio is intentionally not recorded.
   - Output directory: ~/Videos/ATLAS when the platform Movies folder is
     available; otherwise ~/ATLAS is used through the platform fallback path.
   - Filename pattern: ATLAS_Screen_YYYYMMDD_HHMMSS.mp4

Recording controls
------------------
The compact video selector now contains:

    RAW | AI | RAW+AI | PROFILE | VIDEO REC | SCREEN REC | FULLSCREEN | collapse

In DIRECT A8 mode the AI-specific controls remain hidden, so the strip is:

    RAW | VIDEO REC | SCREEN REC | FULLSCREEN | collapse

Both local recording buttons turn red while active and show an elapsed timer.
VIDEO REC and SCREEN REC are independent and may run at the same time. A8 REC
is also independent, so all three recording paths may be active simultaneously.

Payload routes
--------------
The operator still selects the route manually from the PAYLOAD control:

DIRECT A8
    Default control IP:   192.168.144.25
    Default SIYI port:    37260
    Default RTSP:         rtsp://192.168.144.25:8554/main.264

JETSON
    Uses its own independently persisted Jetson control endpoint and RTSP URL.

There is still:
- no Jetson auto-detection,
- no automatic route selection,
- no automatic fallback between routes.

Architecture
------------
QML payload commands continue to use:

    atlasPayload

which routes to AtlasSiyiClient or AtlasJetsonClient.

The new local screen recorder is exposed separately as:

    atlasScreenRecorder

This separation is intentional: screen recording is a ground-station function
and must not depend on the selected payload route.

Files added in v0.11.1
--------------------
    custom/atlas/src/AtlasScreenRecorder.h
    custom/atlas/src/AtlasScreenRecorder.cc

The screen recorder is registered by AtlasCustomPlugin and uses the currently
focused/visible ATLAS top-level window as its capture target. It never selects
an unrelated application window as a fallback.

Apply / verify / build
----------------------
From the QGroundControl repository root:

    unzip -o ~/Downloads/ATLAS_Payload_Mode_v0_11.zip -d ~/ATLAS/ATLAS-QGroundControl

    cd ~/ATLAS/ATLAS-QGroundControl
    python3 tools/atlas/verify_payload_bundle.py

v0.11.1 does not add new source files relative to v0.11. If v0.11 already built on this checkout, a normal rebuild is enough:

    PATH="$HOME/Qt/Tools/CMake/bin:$PATH" ~/Qt/6.11.1/gcc_64/bin/qt-cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

    ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Run only if the build finishes successfully:

    ./build/Release/ATLAS-QGroundControl

Recommended test order
----------------------
1. Verify that v0.11.1 opens and the PAYLOAD selector still works.
2. Confirm DIRECT A8 / JETSON routing is unchanged.
3. With a live stream, press VIDEO REC for 5-10 seconds, stop it, and confirm
   the recording appears in QGC's configured video-save directory.
4. Press SCREEN REC for 5-10 seconds while opening/closing an ATLAS panel,
   stop it, then confirm the MP4 in the ATLAS Movies folder.
5. Run VIDEO REC + SCREEN REC together.
6. Only after local recording is validated, combine either one with A8 REC.

Notes
-----
- QWindowCapture is available in the Qt 6.11.1 toolchain used by this project.
- A full Qt/C++/QML build must still be run on the ATLAS development PC.
- If SCREEN REC cannot start, the error is surfaced in the ATLAS Events panel
  instead of silently pretending to record.
