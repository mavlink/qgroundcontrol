# Maps

Centralized map configuration — provider selection, offline tile management, API tokens, custom tile servers, and cache settings.

## Map Provider

- **Provider** — select the map tile provider (e.g., Bing, Google, Stamen, Mapbox, Esri, Eniro, VWorld)
- **Type** — map type for the selected provider (e.g., Hybrid, Satellite, Street, Terrain)
- **Elevation Provider** — source for terrain elevation data (e.g., Copernicus)

## Offline Maps

Download map tiles for use without an internet connection. You can create multiple tile sets for different locations.

- **Add New Set** — opens a full-screen map where you drag to the area of interest, set the name and zoom range, then click **Download** to cache the tiles
- **Tile set list** — shows all downloaded tile sets with name, tile count, size, and status
- **Import** — load a previously exported tile set file (append to or replace existing tiles)
- **Export** — save selected tile sets to a file for transfer to another device

## Tokens

API tokens for accessing additional map providers:

- **TianDiTu** — token for TianDiTu maps
- **Mapbox** — access token for Mapbox
- **Esri** — token for Esri/ArcGIS maps
- **VWorld** — token for VWorld maps (South Korea)
- **OpenAIP** — token for OpenAIP aeronautical data

## Mapbox Login

Required when using Mapbox as the map provider:

- **Account** — Mapbox account name
- **Map Style** — custom Mapbox style URL

## Custom Map URL

Configure a custom tile server using a URL template with `{x}`, `{y}`, and `{z}` (or `{zoom}`) substitutions:

- **Server URL** — tile server URL template (e.g., `https://tiles.example.com/{z}/{x}/{y}.png`)

## Tile Cache

- **Max disk cache** — maximum disk space for cached map tiles (default: 1024 MB, range: 1–262144 MB)
- **Max memory cache** — maximum RAM for in-memory tile cache (default: 128 MB desktop / 16 MB mobile, range: 1–1024 MB). Changes require a restart.
