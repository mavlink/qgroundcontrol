# What's New

This page highlights user-facing changes since the last stable release (V5.1).

## GeoMap Engine (Tech Preview)

A new experimental map engine that renders a single, seamless 2D/3D map: your selected map imagery draped over real terrain elevation, with Google Earth-style camera controls.
When enabled it replaces the Fly View map, displaying vehicles, the planned mission, the flown flight path, launch and ground station locations in true 3D.
Map click actions are available in 2D mode only (3D is view-only for now), and the goal is for the engine to eventually replace all map usage in QGroundControl.
It is disabled by default — see [GeoMap Engine (Tech Preview)](../fly_view/geoview.md) for how to enable it and full details.

## Fly View

### Click to ROI Altitude

The **ROI at location** map click action now supports setting the ROI altitude.
An altitude slider (meters above home, defaulting to 0 = ground level) is shown before you confirm the ROI, so the vehicle can point at an elevated target instead of only ground points.
