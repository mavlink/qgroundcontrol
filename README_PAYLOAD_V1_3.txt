ATLAS Video Control v1.3
========================

UI/UX refactor focused on maximising useful video area.

Main changes from v1.2:
- Toolbar title is now "ATLAS Video Control | v1.3".
- Smaller ATLAS logo.
- Top status bar is minimal: JETSON / HM30 / A8 / VIDEO / AI show only colour state.
  Full detail remains in SYSTEM STATUS.
- Removed the AI summary strip over the video.
- Removed the central "AI FPS LOW" video warning banner. FPS degradation is still
  visible in System Status / Event Log state.
- Left CAMERA rail no longer extends to the bottom of the screen and can itself
  be collapsed.
- Mini-map starts at the left edge and is wider. It is never collapsible.
- Removed the separate Targets tab. Target marker + UAV-target line appear on the
  normal map automatically only while Tracking Mode is enabled and a target is valid.
- AI / Detection, Tracking, Gimbal, Video Mode, Event Log, UAV Telemetry,
  Attitude/Compass and System Status are individually collapsible.
- Collapse/expand choices persist across restarts.
- VIDEO MODE was moved from the top of the video into the right sidebar between
  Gimbal and Event Log.
- Event Log is much smaller by default and can be collapsed.
- Confidence, AI FPS and gimbal sensitivity sliders use a custom ATLAS style.
- Gimbal sensitivity layout was rebuilt to avoid widget overlap.
- Tracking does NOT auto-enable when selecting the TRACK mission profile. It only
  becomes active through the explicit Tracking Mode toggle.
- Target card is hidden when Tracking Mode is off or the Tracking panel is collapsed.
- Added OPERATOR / CONFIG / RESET UI presets inside System Status:
    OPERATOR -> minimum panels for payload operation
    CONFIG   -> expand all configuration panels
    RESET UI -> restore recommended default layout
- Existing RAW / AI / RAW+AI and fullscreen behaviour is preserved.
- Existing mouse-wheel A8 zoom and interactive mini-map behaviour are preserved.

Apply from the repository root (after v1.2):
  unzip -o ~/Downloads/ATLAS_Payload_Mode_v1_3.zip -d ~/ATLAS/ATLAS-QGroundControl

No new CMake resource entries are added compared with v1.2, so normally only build:
  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Offline verification:
  python3 tools/atlas/verify_payload_bundle.py

Run mock:
  python3 tools/atlas/mock_jetson_server.py

Run QGC:
  ./build/Release/ATLAS-QGroundControl

Useful mock scenario:
  demo start

Final Qt/QML/C++ validation still has to be run with the project's Qt 6.11.1 toolchain.
