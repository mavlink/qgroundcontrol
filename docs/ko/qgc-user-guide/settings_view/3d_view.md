# 3D View Settings

:::warning
The entire 3D View feature is a work in progress. You may hit various issues while using it.
:::

:::info
The 3D View is not available on Android, so these settings are hidden there.
:::

Settings for the optional 3D map view in the Fly View.

## General

- **Enable the 3D viewer** — turn the 3D view on or off (disabled by default)
- **3D map data provider** — select the map data source (currently OpenStreetMap)

## Data

These settings are available when the 3D viewer is enabled:

- **Path to the OSM file** — local `.osm` file to use for 3D building and terrain data. If no file is loaded, buildings are not shown; the 3D view falls back to a map-tile ground patch centered on the vehicle's home position.
- **Average Height for each level of the buildings** — estimated floor height used to calculate building heights from level counts (default: 3 m, range: 0.5–20 m)
- **Altitude bias** — vertical offset applied to vehicles in the 3D view to correct for altitude datum differences (default: 0 m, range: -1000 to 1000 m)
