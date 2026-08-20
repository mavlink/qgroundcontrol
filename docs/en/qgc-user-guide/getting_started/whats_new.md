# What's New

This page highlights user-facing changes since the last stable release (V5.1).

## GeoView (Tech Preview)

A new experimental map engine that renders a single, seamless 2D/3D map: your selected map imagery draped over real terrain elevation, with Google Earth-style camera controls.
It displays vehicles, the planned mission, the flown flight path, launch and ground station locations in true 3D.
GeoView is currently display-only (vehicle control stays in the regular Fly View), but the goal is for it to eventually replace all map usage in QGroundControl.
It is disabled by default — see [GeoView (Tech Preview)](../fly_view/geoview.md) for how to enable it and full details.

## Fly View

### Click to ROI Altitude

The **ROI at location** map click action now supports setting the ROI altitude.
An altitude slider (meters above home, defaulting to 0 = ground level) is shown before you confirm the ROI, so the vehicle can point at an elevated target instead of only ground points.
