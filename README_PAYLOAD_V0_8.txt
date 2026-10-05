ATLAS Video Control v0.8
========================

Pre-deployment UI refinement based on the approved video-focused concept.

Main changes:
- version reset to v0.8: this project is still pre-deployment
- AI panel gets a dedicated ATLAS AI SVG icon
- mini-map has no top title/icon/header bar at all
- Telemetry, Events and System Status typography enlarged
- System Status keeps Jetson/HM30/A8 focus but adds useful compact detail:
  Jetson CPU/GPU/temperature, HM30 RSSI/loss/latency, A8 zoom/yaw/pitch
- top right status pills enlarged with better dot/text alignment
- toolbar gradient adjusted to the approved blue -> dark -> orange reference
- video profile selector fully restyled (no native up/down arrows)
- fullscreen control sits directly beside the profile selector
- fresh v0.8 settings namespace avoids carrying bad collapsed state forward

Because ai.svg was added to custom/atlas/CMakeLists.txt, run qt-cmake configure once
before building this version.

Additional v0.8 changes:
- restored retractable RAW / AI / RAW+AI video-mode toolbar
- extended orange region of the ATLAS top gradient
- vertically centered Jetson/HM30/A8/Video/AI status pills
- mini-map zoom now allows QGC's maximum zoom level 23

Map imagery note:
- ATLAS does not force a satellite provider in v0.8.
- The QGC map engine supports Bing, Esri World Satellite, Google Satellite (when enabled), and Mapbox Satellite.
- v0.8 allows manual mini-map zoom up to QGC zoom level 23.
- Actual imagery sharpness depends mainly on the selected tile provider and the imagery available for that area.
