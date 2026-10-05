ATLAS Payload Mode v1
=====================

Apply from the root of ~/ATLAS/ATLAS-QGroundControl by extracting this archive over the repository.

Main changes:
- Payload-first FlyView overlay.
- Video forced as main view when available; map remains PiP.
- RAW / AI / PIP stream mode commands.
- LOW / BALANCED / ROBUST video profile command.
- Mouse gimbal control + wheel zoom + CENTER/PHOTO/confirmed REC.
- Gimbal presets FORWARD/DOWN/LEFT/RIGHT/TARGET.
- YOLO / SAHI / Depth controls.
- AI model, FAST/BALANCED/ACCURATE, confidence, FPS, resolution, SAHI slice and classes.
- SEARCH / TRACK / IDENTIFY profiles.
- Click-to-track, ROI tracking, auto-gimbal and clear tracking.
- UAV MAVLink telemetry panel using QGC's active Vehicle facts.
- Jetson/HM30/A8/VIDEO/AI health/status panels.
- Event log and alarms.
- Target marker + UAV-target line on the QGC map when target geolocation is supplied.
- Expanded mock Jetson server for testing without hardware.

After extracting, CMake must be reconfigured because custom/atlas/CMakeLists.txt changes.

Configure:
PATH="$HOME/Qt/Tools/CMake/bin:$PATH" ~/Qt/6.11.1/gcc_64/bin/qt-cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

Build:
~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Test terminal 1:
python3 tools/atlas/mock_jetson_server.py

Test terminal 2:
./build/Release/ATLAS-QGroundControl

Useful mock commands:
all on
all off
target on
target off
hot on
hot off
link bad
link good
freeze on
freeze off
status

Notes:
- The low/balanced/robust setting is already transported through the ATLAS TCP protocol, but the final GStreamer parameter values should be tuned on the real HM30/Jetson link.
- RAW/AI/PIP are implemented as semantic Jetson stream modes so the real Jetson service can keep one stable RTSP endpoint for QGC.
- MAVLink telemetry is read-only in this Payload UI. This bundle does not yet change ArduPilot stream-rate parameters; that should be validated with the real HM30 + Pixhawk link so the pilot's independent SiK/RFD link is not affected.
