ATLAS Video Control v1.6
========================

Visual correction pass based on the approved video-focused concept.

Main changes:
- custom ATLAS SVG icon set (map, telemetry, attitude, gimbal, events, system,
  payload rail controls, tracking and fullscreen)
- bottom cards use title case and concept-matching icons
- dark Map header restored (not the old blue Map tab)
- AI defaults expanded on first v1.6 launch; tracking remains explicit-toggle only
- Gimbal card reworked for balanced yaw/pitch + dial + sensitivity + presets
- recording timer uses HH:MM:SS
- top status cards enlarged and aligned; labels use Jetson/Video casing
- richer blue -> dark -> orange toolbar gradient
- video profile mock defaults to BALANCED
- v1.6 layout state uses a new settings namespace so older collapsed v1.5 state
  does not distort the first v1.6 launch

Because custom/atlas/CMakeLists.txt now adds icon resources, run qt-cmake configure once
before building v1.6.
