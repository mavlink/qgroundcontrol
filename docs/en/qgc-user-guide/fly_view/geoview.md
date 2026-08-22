# GeoMap Engine (Tech Preview)

:::warning
The GeoMap engine is an experimental technology preview and is under active development. Expect missing features and rough edges.
It is too early to report issues against this feature — there is still too much work left to do.
:::

The GeoMap engine is a new map engine for QGroundControl that renders a single, seamless 2D/3D map: satellite/street imagery draped over real terrain elevation, with Google Earth-style camera controls. When enabled it replaces the map in the [Fly View](fly_view.md), displaying your vehicles, the planned mission (with waypoint markers, connecting path, and drop lines showing each waypoint's height above the terrain), the flown flight path, the launch location, and the ground station position — all in true 3D.

:::info
Map interaction (click-to-goto, orbit, ROI, and other map click actions) is available in 2D mode only; 3D mode is currently view-only. All other Fly View controls (arm, takeoff, guided actions, instruments, and so on) work as usual.
:::

The long-term goal is for the GeoMap engine to eventually replace all map usage throughout QGroundControl, including the Plan view, providing one consistent 2D/3D map experience everywhere.

## Enabling the GeoMap Engine

The GeoMap engine is disabled by default. To enable it, go to **Application Settings > Fly View** and turn on **Use GeoMap engine (preview)**, then restart QGroundControl. After the restart the Fly View map is rendered by the GeoMap engine.

## View Controls

Camera gestures follow Google Earth semantics: the ground point under the cursor when a gesture starts stays pinned under the cursor throughout the gesture.

- **Mouse:**
  - **Pan**: Left-drag. The terrain point you grabbed follows the cursor.
  - **Orbit (rotate)**: Right-drag or middle-drag. The view rotates around the ground point you pressed on (marked by a pivot ring): left/right changes heading, up/down changes tilt between top-down and near-horizontal.
  - **Zoom**: Scroll wheel or trackpad scroll. Zooms toward/away from the point under the cursor.
- **Touch:**
  - **Pan**: Single-finger drag.
  - **Zoom**: Two-finger pinch, centered on the pinch point.
  - **Rotate**: Two-finger twist.

### Keyboard Support

Keyboard modifiers provide alternatives to the multi-button mouse gestures (useful with one-button mice and trackpads):

- **Shift + left-drag**: Orbit, same as right/middle-drag.
- **Ctrl + left-drag**: First-person look. The camera stays in place and the view rotates, like turning your head — drag toward where you want to look. On macOS both **Ctrl** and **Cmd** work.

### 2D/3D Modes

- The **3D/2D** button in the top-right switches between modes. Switching animates the camera tilt and flattens/raises the terrain; 2D mode behaves like a traditional top-down map (tilt is locked).
- The **compass** button rotates with the current heading; click it to animate the view back to north-up.

## Imagery and Terrain

The GeoMap engine drapes the same map imagery you have selected in **Application Settings > Maps** over its terrain surface. Terrain elevation comes from the [Mapzen/Tilezen Terrain Tiles](https://registry.opendata.aws/terrain-tiles/) dataset, which combines the following sources:

- ArcticDEM terrain data DEM(s) were created from DigitalGlobe, Inc., imagery and funded under National Science Foundation awards 1043681, 1559691, and 1542736
- Australia terrain data © Commonwealth of Australia (Geoscience Australia) 2017
- Austria terrain data © offene Daten Österreichs – Digitales Geländemodell (DGM) Österreich
- Canada terrain data contains information licensed under the Open Government Licence – Canada
- Europe terrain data produced using Copernicus data and information funded by the European Union – EU-DEM layers
- Global ETOPO1 terrain data U.S. National Oceanic and Atmospheric Administration
- Mexico terrain data source: INEGI, Continental relief, 2016
- New Zealand terrain data Copyright 2011 Crown copyright (c) Land Information New Zealand and the New Zealand Government (All rights reserved)
- Norway terrain data © Kartverket
- United Kingdom terrain data © Environment Agency copyright and/or database right 2015. All rights reserved
- United States 3DEP (formerly NED) and global GMTED2010 and SRTM terrain data courtesy of the U.S. Geological Survey
