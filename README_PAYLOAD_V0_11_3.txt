ATLAS VIDEO CONTROL v0.11.3

SCREEN REC encoder compatibility fix
------------------------------------
This release is built directly on v0.11.2 and preserves the validated UI,
manual DIRECT A8 / JETSON routing, 2D/3D map, A8 REC, VIDEO REC and SCREEN REC.

The v0.11.2 screen-capture path itself was correct: it captured the rendered
ATLAS Qt Quick window independently of RTSP/GStreamer. On the test laptop,
however, Qt auto-selected HEVC through VAAPI. The VAAPI driver rejected the
requested HEVC profile and QMediaRecorder stopped immediately.

v0.11.3 fixes that by explicitly using:

    MP4 container
    MPEG-4 Part 2 video codec
    20 fps
    ~8 Mbit/s average video bitrate

MPEG-4 Part 2 uses FFmpeg's native software encoder, avoiding the failing
HEVC/VAAPI path. SCREEN REC remains fully independent of the payload stream:
it can start and continue with no A8, no Jetson, no RTSP and while GStreamer
reports a video-link failure.

A failed encoder session is no longer logged as "Saved". Its incomplete output
file is removed and the actual recorder error remains visible in the ATLAS UI
and terminal.

Recording roles
---------------
A8 REC
    Recording performed by the camera/payload.

VIDEO REC
    Local recording of the active QGroundControl video stream. Requires a live
    decoded stream. Does not include the ATLAS UI.

SCREEN REC
    Local recording of the rendered ATLAS application window. Independent of
    the RTSP stream and payload route. Includes the operator UI, map, telemetry,
    overlays and any visible video/no-video state.

Output
------
SCREEN REC: ~/Videos/ATLAS/ATLAS_Screen_YYYYMMDD_HHMMSS.mp4

Apply
-----
    cd ~/ATLAS/ATLAS-QGroundControl
    unzip -o ~/Downloads/ATLAS_Payload_Mode_v0_11_3.zip -d .
    python3 tools/atlas/verify_payload_bundle.py
    ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2 && ./build/Release/ATLAS-QGroundControl

No qt-cmake reconfigure is required when upgrading from v0.11.2 because this
release does not add source files.

Expected terminal messages
--------------------------
When starting SCREEN REC:
    [ATLAS SCREEN REC] Starting ... codec "MPEG-4" container "MPEG-4"
    [ATLAS SCREEN REC] Recording ...

When stopping successfully:
    [ATLAS SCREEN REC] Saved ...

If the encoder fails, the terminal reports the error and the incomplete file is
removed instead of being reported as a successful recording.
