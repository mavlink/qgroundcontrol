# Maps

Centralized map configuration — provider selection, offline tile management, API tokens, custom tile servers, and cache settings.

## Map Provider

- **Provider** — select the map tile provider (e.g., Bing, Google, Mapbox, Esri, Eniro, VWorld)
- **Type** — map type for the selected provider (e.g., Hybrid, Satellite, Street, Terrain). Not shown for a provider whose map has no type, such as OpenAIP
- **Elevation Provider** — source for terrain elevation data (e.g., Copernicus)

The labels of the Google and Bing maps that have them are in the [language](general.md#language) of QGroundControl. A change of the language applies to the maps that are open.

## Offline Maps

Download map tiles for use without an internet connection. You can create multiple tile sets for different locations.

- **Add New Set** — opens a full-screen map where you drag to the area of interest, set the name and zoom range, then click **Download** to cache the tiles
- **Tile set list** — shows all downloaded tile sets with name, tile count, size, and status
- **Import** — load a previously exported tile set file (append to or replace existing tiles). A file whose tile cache format is too old, or newer than the one of the running version, is refused and the cache stays as it was
- **Export** — save selected tile sets to a file for transfer to another device

With **Fetch elevation data** checked in **Add New Set**, the download saves two more sets for the same area, each a set of its own in the list: **_name_ Elevation**, with the heights that mission planning uses, and **_name_ Terrain**, with the relief of the 3D-capable map.

The first set of the list is the cache itself, with the tiles that the maps saved as you looked at them. In its view, **Clear Cache** deletes those tiles and leaves your tile sets as they are, and **Delete All** deletes every tile and every tile set.

**Cancel Download** stops a running download at once, and **Resume Download** fetches the tiles that are still missing. A place for which the provider has no tile is not an error: the set completes without it.

A tile set of a Google or Bing map with labels has them in the language that QGroundControl had when the set was downloaded, and is used only while QGroundControl is in that language.

### MBTiles Files

**Import** also takes an [MBTiles](https://github.com/mapbox/mbtiles-spec) file (`.mbtiles`), the format in which QGIS, MapTiler, MOBAC and other programs save the tiles of a map:

- The file becomes a tile set of its own in the list, named after the file. With **Replace existing sets** selected, it takes the place of every other set and of all cached tiles. A file that cannot be imported leaves them as they were.
- Its tiles are shown by the map with the provider **MBTiles** and the type **Imported**. That map has the tiles of every file that was imported and asks no server, so it is empty where no file has tiles. Where two files have a tile of the same place, it shows the tile of the file that was imported last.
- **Add New Set** does not offer that map, as there is nothing to download for it.
- The tiles must be pictures (PNG, JPEG, GIF or WebP) in the Web Mercator tile scheme that web maps use. A file with vector tiles is refused.
- Deleting the set removes its tiles, apart from those that the set of another file has as well.

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

The **Custom** map type of Mapbox shows this style. A change of either setting applies to the maps that are open.

## Custom Map URL

Configure a custom tile server using a URL template with `{x}`, `{y}`, and `{z}` (or `{zoom}`) substitutions:

- **Server URL** — tile server URL template (e.g., `https://tiles.example.com/{z}/{x}/{y}.png`)

A change of the URL applies to the maps that are open. The tiles of each URL are cached on their own, so a map does not show tiles of an earlier URL. The server must send PNG, JPEG, GIF or WebP tiles.

## Tile Cache

- **Max disk cache** — maximum disk space for cached map tiles (default: 1024 MB, range: 1–262144 MB). When the cache is full, the tiles that were last used longest ago are removed first. The tiles of an offline tile set are never removed for this.
- **Max memory cache** — maximum RAM for in-memory tile cache (default: 128 MB desktop / 16 MB mobile, range: 1–1024 MB). Changes require a restart.
