ATLAS Video Control v0.7
========================

Pre-deployment UI refinement based on the approved video-focused concept.

Main changes:
- version reset to v0.7: this project is still pre-deployment
- AI panel gets a dedicated ATLAS AI SVG icon
- mini-map has no top title/icon/header bar at all
- Telemetry, Events and System Status typography enlarged
- System Status keeps Jetson/HM30/A8 focus but adds useful compact detail:
  Jetson CPU/GPU/temperature, HM30 RSSI/loss/latency, A8 zoom/yaw/pitch
- top right status pills enlarged with better dot/text alignment
- toolbar gradient adjusted to the approved blue -> dark -> orange reference
- video profile selector fully restyled (no native up/down arrows)
- fullscreen control sits directly beside the profile selector
- fresh v0.7 settings namespace avoids carrying bad collapsed state forward

Because ai.svg was added to custom/atlas/CMakeLists.txt, run qt-cmake configure once
before building this version.
