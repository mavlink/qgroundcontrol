ATLAS Payload / Video Control v1.2
==================================

Main changes from v1.1:
- Smaller ATLAS logo.
- Toolbar subtitle renamed to "ATLAS Video Control".
- RAW / AI / RAW+AI video modes.
- Video profiles shown as FAST / BALANCED / STABLE while keeping the wire protocol LOW / BALANCED / ROBUST.
- FULLSCREEN video button. Fullscreen shows the currently selected RAW / AI / RAW+AI stream.
- Mouse wheel continues to command A8 zoom while video is fullscreen.
- Tracking is now a real ON/OFF mode. Tracking UI, target card and map target overlays stay hidden while OFF.
- Added tracking_mode command/status to AtlasJetsonClient protocol and mock server.
- Added gimbal mouse sensitivity slider (0.25x to 2.50x).
- Mouse wheel over normal video controls A8 zoom.
- Mini-map is interactive: drag to pan, mouse wheel to zoom, short click to expand to full Fly View.
- When the map is full, ATLAS payload panels are hidden so the map is unobstructed; use the VIDEO button to return.
- Video status now identifies RAW / AI / RAW+AI and FAST / BALANCED / STABLE more clearly.
- Includes the FlyView anchor fix from the v1.1 troubleshooting step.

Apply from the repository root:
  unzip -o ~/Downloads/ATLAS_Payload_Mode_v1_2.zip -d ~/ATLAS/ATLAS-QGroundControl

No new CMake resource entries were added compared with v1.1, so if v1.1 was already configured:
  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Run mock:
  python3 tools/atlas/mock_jetson_server.py

Run QGC:
  ./build/Release/ATLAS-QGroundControl

Useful mock commands:
  all on
  tracking on
  tracking off
  target on
  target off
  demo start
  demo stop
  hot on
  hot off
  link bad
  link good
  freeze on
  freeze off

Offline checks:
  python3 tools/atlas/verify_payload_bundle.py

Final Qt/QML/C++ validation must still be done with the project's Qt 6.11.1 build on the development machine.
