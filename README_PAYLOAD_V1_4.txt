ATLAS Video Control v1.4
========================

This bundle refines the UI to the approved video-focused concept while preserving
all existing Jetson/mock protocol functionality from v1.3.

Requested UI changes implemented:
- Top toolbar keeps the ATLAS blue/dark/orange gradient.
- Header remains ATLAS Video Control and version is now v1.4.
- Top status pills show compact operational values:
    JETSON -> GPU %
    HM30   -> RSSI dBm
    A8     -> zoom x
    VIDEO  -> FPS
    AI     -> FPS
- Video mode selector is back over the upper-left of the video:
    RAW / AI / RAW+AI / FAST-BALANCED-STABLE / fullscreen.
  It can still be collapsed.
- AI / DETECTION is renamed simply AI and remains collapsible.
- AI panel is deliberately compact: YOLO, SAHI, Depth/Distance, Tracking Mode,
  Confidence and AI FPS. Advanced backend API remains available but no longer
  occupies permanent video space.
- Tracking controls appear only after the explicit Tracking Mode toggle is ON.
- Target card and map target are shown only while Tracking Mode is enabled and a
  valid target exists.
- The mini-map no longer has the blue MAP badge. It remains interactive:
    drag = pan, wheel = zoom, click = expand to the normal full map.
- Bottom deck now follows the approved concept:
    Telemetry | Attitude/Compass | Events | Gimbal | System Status
  Every bottom panel except the map is collapsible.
- Telemetry keeps the same compact height and contains exactly six values:
    Alt, Speed, Heading, Dist to Home, Battery, GPS.
- Battery is displayed as VOLTAGE ONLY, using the QGC battery voltage Fact and
  its voltage units (V). Battery percentage is intentionally not shown.
- Gimbal is now a critical bottom panel with yaw, pitch, zoom, position pad,
  mouse sensitivity slider and compact presets. It remains collapsible.
- System Status shows ONLY the requested three rows and no other system data:
    Jetson -> temperature | GPU
    HM30   -> RSSI | packet loss
    A8     -> Ready/Offline | zoom
  Video/model/CPU/RAM/storage/AI details are intentionally not shown there.
- Events moved to the compact bottom strip.
- The left camera rail no longer extends to the bottom and can be collapsed.
- Existing mouse-wheel A8 zoom, gimbal mouse control and video fullscreen remain.
- Existing ATLAS styled sliders are retained.

Apply from repository root:
  unzip -o ~/Downloads/ATLAS_Payload_Mode_v1_4.zip -d ~/ATLAS/ATLAS-QGroundControl

Offline verification:
  python3 tools/atlas/verify_payload_bundle.py

No new CMake resources were added relative to v1.3, so normally only rebuild:
  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Run mock:
  python3 tools/atlas/mock_jetson_server.py

Run QGC:
  ./build/Release/ATLAS-QGroundControl

Useful mock scenario:
  demo start

The bundle has been structurally verified offline. Final Qt/QML/C++ validation
must still be run with the project's Qt 6.11.1 toolchain on the development PC.
