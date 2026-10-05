ATLAS Video Control v1.5
========================

Visual/layout changes implemented from the approved concept:
- Bottom collapsed cards now stay attached to the bottom edge instead of floating.
- Bottom order: Telemetry | Attitude/Compass | Gimbal | Events | System Status.
- Gimbal card visually reworked to fit cleanly: yaw/pitch, circular dial, sensitivity slider, presets.
- Bottom section headers now use QGC image resources instead of text glyph icons.
- Top JETSON/HM30/A8/VIDEO/AI status pills enlarged and dot/text vertically aligned.
- RAW/AI/RAW+AI selector moved to the upper-left, directly above the vertical payload control rail.
- Top toolbar gradient strengthened blue -> dark -> orange while preserving the ATLAS style.
- Existing v1.4 functionality retained: map interaction, tracking gating, fullscreen video, scroll zoom,
  AI panel, telemetry values, exact three-row System Status, collapsible cards, mock Jetson API.

Apply from repository root:
  unzip -o ~/Downloads/ATLAS_Payload_Mode_v1_5.zip -d ~/ATLAS/ATLAS-QGroundControl

Verify:
  python3 tools/atlas/verify_payload_bundle.py

Build:
  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Run mock:
  python3 tools/atlas/mock_jetson_server.py

Run QGC:
  ./build/Release/ATLAS-QGroundControl

Final Qt/QML compilation still must be performed on the development machine with Qt 6.11.1.
