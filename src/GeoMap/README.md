# GeoMap

A custom 2D/3D map engine built on Qt Quick 3D, currently an experimental preview
alternative to QtLocation's `Map`. The ground is a mesh shaped by real elevation
data and draped with standard map tile imagery, viewed through a gesture-driven
camera that smoothly transitions between a classic top-down 2D map and a tilted 3D
view. Used by the FlyView (`FlyViewGeo.qml` / `FlyViewGeoMap.qml`).

## The big picture

The visible ground is made of square **patches** — slippy-map tiles chosen from a
quadtree so that patches near the camera are high-detail and distant ones are coarse.
Each patch needs two things:

- **Shape** (elevation): a grid of heights that displaces the patch mesh vertically.
- **Appearance** (imagery): a standard map tile image draped over the mesh.

Both are fetched cache-first through QGC's existing shared tile cache (network only
on a miss), so tiles downloaded for GeoMap are cached alongside normal map tiles and
vice versa.

As the camera moves, the set of patches is continuously refined: new detail is added
where the camera looks, off-screen patches are dropped, and everything happens
incrementally so the frame rate stays stable. The 2D↔3D switch is just an animation:
tilting the camera while scaling terrain displacement between 0 (flat 2D) and 1
(full 3D) — no geometry is rebuilt.

## Elevation: where the terrain shape comes from

Elevation data is the **AWS Open Data Terrain Tiles** dataset — worldwide elevation
encoded as "terrarium" PNGs, addressed exactly like map tiles (z/x/y, available up to
zoom 15; requests above z15 are satisfied from the z15 ancestor, so imagery keeps
refining past the elevation ceiling). `TerrariumTileFetcher` fetches them
cache-first: it checks QGC's shared tile database, falls back to the network on a
miss, and stores fetched tiles back into the cache.

Decoded tiles land in the `HeightField`, which presents **one continuous
heightfield**: it can answer "how high is the ground here?" for *any* position —
real data where a tile is present, an interpolated estimate from a coarser ancestor
tile elsewhere, and zero where nothing is known yet. This means a patch can always
be meshed immediately with the best current estimate; when better data arrives the
affected patches are re-meshed. There is never a hole in the terrain, only
temporarily-coarser terrain. Internally the `HeightField` keeps its tiles in an
`ElevationTilePyramid`, an in-memory LRU working set that resolves ancestor lookups
and pins tiles backing on-screen patches against eviction.

`TerrariumTileFetcher` implements the abstract `HeightSource` interface, which also
has synthetic implementations: `FlatHeightSource` (z = 0 everywhere, used when
terrain is disabled) and `DebugHeightSource` (analytic hills for verifying mesh
displacement and LOD offline).

## Imagery: where the map picture comes from

Patch imagery is the same provider tiles QGC uses everywhere else (Google Satellite,
OpenStreetMap, ...). `TileImageSource` exists because QGC's normal tile fetching is
buried inside a QtLocation plugin that can only feed a QtLocation `Map`; GeoMap
needs raw `QImage`s to upload as Quick 3D textures. It reuses the shared pieces
(cache database, provider/URL registry) with the same cache-first policy.

While a patch's own image is loading, `SurfacePatchModel` composites a stand-in from
whatever is available — the patch's four child tiles when present (sharper), else a
cropped piece of the nearest ancestor tile — so the ground never flashes blank.
Images from removed patches are retired rather than dropped, keeping them available
as stand-in sources across LOD changes.

## The surface: turning data into a mesh

`SurfaceModel` decides *which* patches exist. On every camera change it estimates
the visible ground region, walks the tile quadtree subdividing any patch that would
appear larger than ~384 px on screen, and diffs the result against the current set.
Changes are budgeted (a few adds/removals per pass, with follow-up passes
self-scheduled) because creating render delegates is synchronous and costly; a fast
zoom therefore streams detail in over several frames instead of hitching.

`SurfacePatchModel` is the QML bridge and the pipeline owner: it constructs the
height source, `HeightField`, `SurfaceModel`, and `TileImageSource` based on its
properties (`terrain`, `mapType`, `debugHills`), and exposes the patch set as a
`QAbstractListModel` — one row per patch, updated incrementally — that a
`Repeater3D` in `GeoMap.qml` turns into Quick 3D models.

Each patch renders as a `Model` combining:

- **`PatchGeometry`** — the grid mesh, displaced by the patch heights, with edge
  skirts and edge-vertex stitching so borders between different LOD levels never
  show cracks.
- **`PatchTextureData`** — the patch's tile image as a Quick 3D texture
  (`CheckerboardTextureData` is a procedural debug substitute).

`SurfaceAnalysis` is a diagnostic pass over the live patch set (coverage holes,
boundary seams, camera-below-surface), invoked via
`SurfacePatchModel::analyzeSurface()`.

## Scene and camera

`GeoScene` defines the shared render space: scene coordinates are Web Mercator
meters relative to a movable `sceneOrigin`, which re-anchors to the camera whenever
it drifts more than 50 km so single-precision GPU floats stay accurate anywhere on
Earth. It also owns the two global scale factors — `verticalScale` (Mercator
latitude correction, so terrain isn't squashed away from the equator) and
`terrainScale` (the animated 0–1 factor behind the 2D↔3D transition). Everything
renderable positions itself through `GeoScene`, so a re-anchor shifts the whole
scene consistently.

`GeoMapCamera` is the camera pose model: center coordinate, heading, tilt, and
distance, with 2D/3D modes. Gestures mirror Google Earth: pan, orbit (right,
middle, or Shift+left drag), and zoom are cursor-anchored — the ground point
under the cursor at gesture start stays pinned — plus first-person look about a
fixed camera position (Ctrl+drag). Zoom levels match
QtLocation's for familiar behavior. Its outputs drive the `View3D`'s
`PerspectiveCamera`, and its projection math (`screenToGround`, `worldToScreen`)
underpins visibility estimation and overlay placement.

`GeoMap.qml` composes all of the above into the top-level map control: camera,
scene, patch model, `View3D`, gesture handlers, fog haze over the distant LOD ring,
and the 2D↔3D mode animation.

## Overlays: putting things on the map

`GeoMapItem` is the counterpart of QtLocation's `MapQuickItem`: it anchors QML
content at a geo-coordinate by projecting through the scene camera into the 2D layer
above the `View3D`. It supports clamp-to-ground or absolute altitude, and an
optional `delegate3D` — a Quick 3D node placed *inside* the scene that crossfades
with the 2D content during mode transitions. Built on it:

- **`GeoMapPin.qml`** — teardrop marker.
- **`GeoMapVehicleItem.qml`** — vehicle marker: 2D icon in map mode, a
  `PaperPlaneGeometry` dart with full attitude in 3D.
- **`GeoMapFlightPath.qml`** — the flown-trail ribbon (`FlightPathGeometry`, a
  shader-billboarded strip with constant on-screen width), flattening back to the
  classic 2D trail as `terrainScale` animates to 0.

Vehicle-related items apply a **home terrain bias** — the difference between the
elevation dataset's height at the home position and the vehicle's reported home
altitude — so a landed vehicle sits exactly on the rendered ground even when the
DEM and GPS altitudes disagree.

## Coordinate systems

| Space | Definition | Where conversion happens |
|---|---|---|
| **Geodetic** | `QGeoCoordinate` (WGS84 lat/lon/alt AMSL) | `TileMath::geoToWorld` / `worldToGeo` |
| **World** | Web Mercator (EPSG:3857) meters; x east, y north, z up | `TileMath` only — the single conversion point |
| **Tile** | Slippy z/x/y (`TileMath::TileKey`), (0,0) NW | `TileMath::tileForWorld` / `tileMinCorner` |
| **Scene** | World minus `GeoScene::sceneOrigin`; z scaled by `verticalScale` (`terrainScale` is applied separately by render transforms) | `GeoScene::scenePositionFor` |
| **Patch-local** | Origin at patch center, ±span/2 | `PatchGeometry` |
| **Screen** | Pixels through the camera projection | `GeoMapCamera::worldToScreen` / `screenToGround` |

World-coordinate and projection math is double precision; elevation samples and
render-boundary data are stored as floats.

## Threading

Effectively single-threaded (GUI thread) by design. All asynchrony is event-loop
asynchrony: height and image sources always deliver results through queued
invocations, even when the answer is immediately available, so consumers see one
consistent flow. Real concurrency is delegated to existing infrastructure —
tile-cache lookups run on the tile engine's worker thread, network I/O on
`QNetworkAccessManager`, GPU upload on Qt Quick 3D's render thread.
