# Terrain Tiles

`tools/terrain/` builds QGC's elevation tiles, served at
`https://terrain.assets.qgroundcontrol.com/terrarium/{z}/{x}/{y}.png`
([#15090](https://github.com/mavlink/qgroundcontrol/issues/15090)). The entry point is
[`../terrain_tiles.py`](../terrain_tiles.py).

The tiles serve two consumers, and the format decisions below are driven by the second:

- **GeoMap display:** the 3D terrain surface, through `TerrariumTileFetcher`.
- **Vehicle terrain (planned):** `TerrainProtocolHandler` answers ArduPilot's `TERRAIN_REQUEST`
  with `TERRAIN_DATA` sampled from these tiles, replacing the Auterion source
  ([#15252](https://github.com/mavlink/qgroundcontrol/issues/15252)). The vehicle also caches
  that data on its SD card, so this is the only way these tiles reach a vehicle. QGC never writes
  ArduPilot `.DAT` files.

## Output Format

| Property | Value |
| -------- | ----- |
| Projection | Web Mercator (EPSG:3857) slippy tiles, `y` increasing southward |
| Zooms | z0–z13; a z13 pixel is 19 m at the equator, 13.5 m at 45° |
| Tile | 256×256 RGB PNG, terrarium encoding: `height = R*256 + G + B/256 - 32768` m |
| Pixel convention | Pixel `(px, py)` covers `[px, px+1) / 256` of its tile; its value is the height at the pixel center, matching `TerrariumTileFetcher` |
| Height step | 0.25 m: heights are rounded to the nearest quarter meter |
| Heights | Copernicus DSM (surface incl. canopy, buildings, snow and ice) in meters above the EGM2008 geoid |
| Ocean | Exactly 0 m; all-ocean tiles share one byte-identical 564-byte PNG |
| Metadata | `tilejson.json` (TileJSON 3.0.0 with the data `version` and an update `notice`, plus `encoding`, `height_step_m`, `vertical_datum`, `surface`) and `ATTRIBUTION.txt`, next to `terrarium/` |

The tiles decode with today's terrarium client, but switching QGC to them takes more than a new
URL; see [Required QGC Client Changes](#required-qgc-client-changes).

## Required QGC Client Changes

None of these exist yet: QGC still reads Tilezen (`TerrariumElevationProvider::kProviderURL` in
`src/QtLocationPlugin/Providers/ElevationMapProvider.h`). They must ship together with the URL
switch, otherwise fixes published here never reach users.

1. **URL and zoom.** Point the provider at
   `https://terrain.assets.qgroundcontrol.com/terrarium/{z}/{x}/{y}.png` and lower
   `TerrariumTileFetcher::kMaxTileZoom` from 15 to 13. The fetcher already derives deeper
   zooms from their z13 ancestor.
2. **A cache identity of its own.** QGC's tile cache keys tiles by provider type and x/y/z
   (`UrlFactory::getTileHash`), ignores HTTP cache headers, and never expires a tile; only the
   cache size limit evicts them. Reusing the `Terrarium` provider key would mix users' cached
   Tilezen tiles with these. Give the new source its own provider key, or clear the old one's
   tiles on upgrade.
3. **Follow the data version, and tell the user.** Tiles are updated in place when a generator
   bug is fixed (see [Publishing and Updates](#publishing-and-updates)), and the tile cache
   would otherwise keep the old ones indefinitely.
   - Fetch `tilejson.json` at startup and again periodically while running (it is cached for
     five minutes), so long sessions see an update too.
   - If its `version` differs from the version last stored in settings, delete this provider's
     cached tiles, then store the new version. Store it only after the cache is cleared. On the
     first run (no stored version) just store it.
   - Never discard tiles silently: show the `notice` text (`{"severity", "text"}`). An `info`
     notice can be a non-blocking message; a `critical` one must stay until the user
     acknowledges it, because data that already left the cache (saved plans with terrain
     altitudes, missions on a vehicle, vehicle SD card terrain) is not fixed by re-downloading.
     If a newer version arrives without a notice, still say that terrain data was updated.
   - If the fetch fails, keep the cache and the stored version. Users flying offline keep the
     old tiles until QGC next starts online.
4. **403 is a failure, not ocean.** Every key exists in a full upload, and missing keys return
   403 (not 404) because CloudFront cannot list the bucket. A 403 must not be cached or decoded
   as sea level.
5. **Attribution.** Show the Copernicus notice (`tilejson.json` `attribution`, or
   `ATTRIBUTION.txt`) wherever this terrain is surfaced, replacing the Tilezen attribution.
6. **Vertical datum.** Heights are above EGM2008 (`vertical_datum` in `tilejson.json`). Anything
   that compares them with vehicle or GPS altitudes must use the same datum.
7. **Vehicle terrain (planned, [#15252](https://github.com/mavlink/qgroundcontrol/issues/15252)).**
   If `TerrainProtocolHandler` answers `TERRAIN_REQUEST` from these tiles:
   - Round heights to the nearest meter when converting to `int16`; truncating reports ground
     up to 1 m low, the unsafe direction.
   - Unit-test what is sent the way the vehicle samples it, as terraingen's `slice_graph.py`
     does for its files: grid point positions against `AP_Terrain`'s single flat-earth offset
     (QGC chains geodesic steps today), and rounded heights on a synthetic linear ramp. The
     generator's validation checks the tiles, not the points QGC samples from them. Details in
     [#15252](https://github.com/mavlink/qgroundcontrol/issues/15252#issuecomment-6255256391).
   - The vehicle caches `TERRAIN_DATA` on its SD card, stamping each block with the firmware's
     `TERRAIN_VERSION_MINOR_MIN`. Updated tiles never reach those caches: only ArduPilot raising
     that minimum, or users clearing `/APM/TERRAIN`, replaces them. A bug that affects vehicle
     terrain needs coordinating with ArduPilot.
   - Keep QGC out of writing `.DAT` files. Doing so would mean tracking ArduPilot's block
     format and its `version_minor` stamp.

## Design Decisions

Each decision with the evidence for it. Changing one changes the tile format, which older QGC
builds would misread, so it cannot be published in place: it needs a new path (e.g.
`terrarium2/`) and a QGC release.

### Source: Copernicus only, no lidar overlay

Copernicus GLO-30 everywhere, with GLO-90 for the 25 cells GLO-30 omits. Tilezen's
higher-resolution lidar (US, parts of the EU, NZ) is not overlaid: the same single dataset then
feeds both the map and the vehicle, there are no mixed-source seams, and Tilezen is frozen around
2017.

### Max zoom: z13

The source is ~30 m everywhere. z12 pixels (38 m at the equator, 33 m at 30°) undersample it
below ~37° latitude, which also matters for vehicles using `TERRAIN_SPACING = 30`. z13 (19 m at
the equator) holds the full source resolution at every latitude.

### Height step: 0.25 m

ArduPilot stores and transmits terrain as whole meters (`int16`) on a `TERRAIN_SPACING` grid
(default 100 m, 30 m with SRTM1 data). Planned `TERRAIN_DATA` samples these tiles bilinearly
and rounds to whole meters, so the stored step and that rounding stack. Simulating that path on
~2.6M land points of the sample areas:

| Stored step | `TERRAIN_DATA` values changed by 1 m | Worst error vs exact | Mean bias | Mean land z12 tile |
| ----------- | ------------------------------------ | -------------------- | --------- | ------------------ |
| 1/256 m (full precision) | 0% | 0.50 m | 0 | 113 KB |
| 1/16 m | 1% | 0.53 m | 0 | 75 KB |
| **0.25 m** | **4%** | **0.63 m** | **0** | **53 KB** |
| 0.5 m | 8% | 0.75 m | 0 | 42 KB |
| 1 m | 14% | 0.98 m | 0 | 31 KB |

"Changed" means the whole-meter value differs from rounding the unrounded height. Rounding is to
the nearest step, so no option biases terrain low, the unsafe direction. 0.25 m adds at most
12.5 cm, small against Copernicus's own accuracy (< 4 m absolute, < 2 m relative on gentle
slopes, 90% of points) and the 2–3 m bias between Copernicus and ArduPilot's data. It removes
noise digits that make up about half of a full-precision tile. 1 m was rejected because the
double rounding reaches ~1 m. A 0.25 m step is also too small to show as banding in GeoMap's
lighting.

### Vertical datum: EGM2008, unconverted

Copernicus heights are above the EGM2008 geoid; ArduPilot's SRTM/AW3D30 data uses EGM96.
Measured differences: median 0.2 m globally, 95% within 1.1 m, 99% within 2.5 m, at most ~9.5 m
(Antarctica); 0.1–2.3 m at the sample areas. ArduPilot's arming correction (`TERRAIN_OFS_MAX`)
absorbs a constant local offset, GPS receivers use various geoid models anyway, and QGC already
sends EGM2008 heights through the Auterion source. Recorded in `tilejson.json`.

### Surface model

Copernicus is a DSM, as is ArduPilot's AW3D30 and today's Auterion source. Trees, buildings and
snow are part of the surface.

## Source Data

[Copernicus DEM](https://registry.opendata.aws/copernicus-dem/) on AWS Open Data, read directly
over HTTPS as Cloud Optimized GeoTIFFs:

- **GLO-30** (`copernicus-dem-30m`, eu-central-1): 26,450 1°×1° cells.
- **GLO-90** (`copernicus-dem-90m`) fills the 25 cells GLO-30 omits (Armenia, Azerbaijan and
  their borders). Without it those mountains would be written as sea level.
- Cells absent from both `tileList.txt` files are ocean. Inside a cell, ocean is stored as 0 m,
  not nodata.

Each cell's pixel centers sit on whole multiples of its resolution (GeoTIFF
`AREA_OR_POINT=Point`; GDAL reports the half-pixel area overhang). A pixel centered on a whole
degree is the top row (or first column) of the cell south (or east) of it. Latitude spacing is
1″ (30.9 m) everywhere in GLO-30; longitude spacing widens with latitude so ground spacing stays
near 30 m:

| Latitude | 0–50° | 50–60° | 60–70° | 70–80° | 80–85° | 85–90° |
| -------- | ----- | ------ | ------ | ------ | ------ | ------ |
| Longitude spacing | 1″ | 1.5″ | 2″ | 3″ | 5″ | 10″ |

GLO-90 cells are 3″×3″. Where a GLO-90 cell lies north of a GLO-30 cell, their pixel areas leave
a one-row gap; the reverse overlaps.

## Pipeline

The unit of work is one **z9 job tile** (16×16 z13 tiles, 4096×4096 px); 262,144 jobs cover the
globe and run in a process pool. Per job ([`generate.py`](generate.py)):

1. **Select cells** intersecting the job plus a small buffer ([`copernicus.py`](copernicus.py)).
   Longitudes wrap at the antimeridian: a job ending at 180°E also reads the cell at 180°W,
   placed 360° east, so its last pixels interpolate across the line instead of becoming sea
   level. No cells: write the shared sea PNG for all 341 tiles and stop.
2. **Group** the cells by source lattice (a longitude band, or GLO-90) and merge each group into
   a grid on its own lattice, padded past the job edge by the 100-pixel void-fill radius and
   four of its pixels. A grid's extent depends only on its resolution, so every job around a
   cell sees the same data.
   - Cells sharing a resolution are merged by exact pixel copy (same lattice), which keeps
     bilinear interpolation continuous across 1° cell edges instead of clamping at each file's
     edge.
   - Different lattices are never merged directly: rasterio rounds coarser source windows to
     whole pixels, shifting that data by up to a pixel (~23 m at 60°N).
   - A group's missing nodes (the neighboring band or GLO-90 cell) are resampled from the other
     groups with plain bilinear, so the cell blends with its neighbors only within a pixel of
     their shared edge.
3. **Fill voids.** Any node of a group inside one of its own cells (by whole 1° cells, not file
   extents) that still has no height is a void: source nodata. Voids within 100 source pixels
   (~3 km) of data are interpolated. A larger void **fails the job** rather than being written
   as sea level. Ocean (no cell) is never filled.
4. **Warp** each group to the 4096×4096 Web Mercator grid with exact z13 pixel bounds, bilinear,
   and take every pixel from the group whose cell contains it. Each pixel is therefore
   interpolated once, from native data, whatever its neighbors read: resampling all cells onto
   one lattice would interpolate a cell twice in a job that also reads a finer neighbor, and
   not in the job next to it.
5. **Cut** z13 tiles from the grid, then build z12..z9 by 2×2 averaging the float heights (never
   the encoded RGB, whose encoding is not linear). Heights are rounded to the 0.25 m step only
   when each tile is encoded.
6. **Mark** the job done in `<out>/_done/` with its status (`ocean` or the cell count), so
   reruns resume.

`overviews` then builds z8..z0 from the z9 tiles by decoding, averaging and re-encoding; missing
children count as sea level.

## Publishing and Updates

There is one production tile tree, at the root of the private `qgc-terrain-assets` bucket
(us-west-2, served only through the CloudFront distribution `E2AZ33YAOTUR6`, no ACLs). It is
updated in place: a generator fix overwrites the affected tiles rather than publishing a second
copy of the ~1.8 TB tree. Test uploads go under a folder instead (`--prefix validation`), which
serves at `https://terrain.assets.qgroundcontrol.com/validation/terrarium/...`.

`upload` (`--production` or `--prefix NAME`, plus a required `--data-version`) runs three steps,
stopping at the first failure:

1. **Tiles:** `aws s3 sync` with a one-day max-age. Remote tiles missing locally are deleted only
   with `--mirror`, which is refused for production: a full build writes every key, and a partial
   one would wipe the rest.
2. **CloudFront invalidation** of `<target>/terrarium/*`, waiting until it completes.
3. **Metadata:** `ATTRIBUTION.txt`, then `tilejson.json` with `version` set to `--data-version`
   and the `notice`, both with a five-minute max-age. `tilejson.json` goes last, so the new
   version appears only once everything else is in place.

The order is the safeguard. QGC clears its cached tiles when the data version changes (see
[Required QGC Client Changes](#required-qgc-client-changes)). If CloudFront could still serve
an old tile after the new version is visible, a client would fetch and cache it under the new
version for good. If the invalidation fails or its wait times out, the metadata is not uploaded;
rerun `upload` to finish.

Before anything is uploaded to production, `upload` also refuses:

- **A partial build:** every z9 job must have a done marker, and the z0 tile must be newer than
  the newest one. Overviews of a partial build would replace the world's z0–z8 tiles with mostly
  sea level, and overviews older than the jobs would publish stale low zooms.
- **A version that does not rise:** `--data-version` must be higher than the `version` in the
  live `tilejson.json` (read from S3, not CloudFront). Reusing a version would skip every
  client's cache refresh. Rerunning after a failed invalidation is fine: that run never
  published its version.
- **An unchecked build:** a passing [whole-build check](#whole-build-check) must be newer than
  the overviews, so every tile was checked after the last change.

**Data version:** `MAJOR.MINOR.PATCH` in TileJSON's `version` field, compared numerically. QGC
only checks it for equality. Format changes are not updates (see
[Design Decisions](#design-decisions)).

**Notice:** production uploads require `--notice TEXT`, which QGC shows when it discards its
cached tiles for the new version. `--notice-severity critical` is for errors users must act on:
say what was wrong, where and since when, and what to do (re-plan terrain-following missions;
ArduPilot users clear `/APM/TERRAIN`, since updated tiles never replace vehicle terrain caches).
The default `info` is for routine improvements. Each upload replaces the notice, so a client
that skips a release never sees the skipped notice: restate any guidance users must still act
on in later critical notices.

**Publishing a fix:**

1. Fix the generator and add a regression test (see [Unit Tests](#unit-tests)).
2. Regenerate, run `overviews`, then `check`, and `validate` the affected areas and a random
   sample.
3. `diff` the new build against the previous one to confirm the fix changed only what it should
   (see [Comparing Builds](#comparing-builds)).
4. Upload to a test prefix and inspect it, then `upload --production` with a higher
   `--data-version` and a `critical` notice.

Missing keys return **403**, not 404: CloudFront's access to the bucket excludes listing.

### Modules

| Module | Purpose |
| ------ | ------- |
| [`tilemath.py`](tilemath.py) | Web Mercator tile bounds and lon/lat conversions; `MAX_ZOOM`, `JOB_ZOOM` (pure Python) |
| [`heights.py`](heights.py) | Sea level and the 0.25 m height step, shared by the encoder and the metadata (pure Python) |
| [`copernicus.py`](copernicus.py) | Cell naming, tile lists, GLO-30/GLO-90 cell map, cell selection |
| [`terrarium.py`](terrarium.py) | Terrarium encode/decode, 0.25 m rounding, height averaging, PNG I/O |
| [`generate.py`](generate.py) | Per-lattice grids, void fill, warp, pyramid, overviews, process pool |
| [`jobs.py`](jobs.py) | Job done markers, bounded job scheduling and validation job selection (pure Python) |
| [`validate.py`](validate.py) | Source, ArduPilot and Tilezen checks; tile size report |
| [`check.py`](check.py) | Whole-build check of every tile: structure, seams, height jumps |
| [`compare.py`](compare.py) | Tile-by-tile comparison of two builds |
| [`fetch.py`](fetch.py) | HTTP GET retrying transient failures, for the ArduPilot and Tilezen references (pure Python) |
| [`ardupilot.py`](ardupilot.py) | Independent numpy reader for ArduPilot's SRTM1 `.hgt` cells |
| [`metadata.py`](metadata.py) | `tilejson.json` and `ATTRIBUTION.txt` contents |
| [`upload.py`](upload.py) | `aws` commands for tile sync, CloudFront invalidation and metadata; cache policy |
| [`cli.py`](cli.py) | `generate`, `overviews`, `validate`, `stats`, `check`, `diff`, `upload` subcommands |

## Safeguards Against Known Terrain Bugs

ArduPilot's terraingen shipped several conversion bugs that put wrong terrain on vehicles
([#15252](https://github.com/mavlink/qgroundcontrol/issues/15252)). Each class and how this
pipeline handles it, plus one class this pipeline had itself (the antimeridian):

| Bug class | Risk here | Prevention | Caught by |
| -------------- | --------- | ---------- | --------- |
| Land written as 0 m after an ocean tile (state leaking between tiles) | Missing cells or gaps read as ocean | Each job starts from fresh arrays in its own process; GLO-90 fills GLO-30's gaps; coverage is by whole cells | Unit tests; source check; ArduPilot land-as-sea count; `check` seams on every tile |
| Voids written as 0 m | Source nodata becomes sea level | Voids interpolated; unfillable voids fail the job | Unit tests (small void filled, large void fails, ocean untouched); `check` seams on every tile |
| Wrong interpolation at 1° cell edges | Clamping at each file's edge | Cells of one lattice merged into one grid per job | Unit test (exact ramp across borders); source check samples cell borders |
| Land written as 0 m along the antimeridian (found in this pipeline, fixed) | The cell across 180° never read, so the last pixels had no data on one side | Cell selection wraps longitudes; that cell is placed 360° away | Unit test (exact ramp up to 180°E); `taveuni-180e`/`taveuni-180w` sample areas; `check` seams across 180° |
| Horizontal drift from two-step offset math | Misregistered pixels, worse at high latitude | GDAL does all georeferencing from exact tile bounds; mixed lattices warped, not merged | Unit tests (exact ramp across a longitude band); source check; ArduPilot offset |
| Old bad data indistinguishable from fixed data | Stale tiles in CloudFront and client caches | Data version in `tilejson.json`, required to rise and published only after the CloudFront invalidation completes; QGC clears its tile cache when the version changes and shows the required notice | `upload` order, version and notice tests |

## Validation

`terrain_tiles.py validate` checks completed jobs and exits nonzero on any problem. Run it on
the sample areas before a global run, and on a random sample of jobs before uploading to
production:

- No option validates every completed job; `--sample-jobs N` validates N random land jobs
  (reproducible with `--seed`; ocean jobs have nothing to compare); `--job Z/X/Y` (repeatable)
  validates specific jobs.
- Jobs are decoded and checked one at a time and only the result rows are kept, so memory stays
  around 1–1.5 GB per run regardless of job count.
- A job whose tiles cannot be read, or a source or ArduPilot check that fails outright (e.g. a
  download that outlasts its retries), is reported as a problem for that job and the run
  continues; a failed Tilezen check only prints a warning. Downloads retry connection errors,
  timeouts and 5xx responses.

1. **Copernicus source (correctness of our pipeline).** Decoded z13 pixels against a
   point-bilinear sampler written independently of the generator: it reads the 2×2 source
   pixels around each pixel center directly, crossing into the east/south neighbor cell at
   edges. Each job gets exactly `--samples` samples (default 400): half within two pixels of
   the 1° cell borders it crosses, the rest uniform (all uniform if it crosses none). z13 pixels
   are finer than the source everywhere, so the expected error is the rounding alone: mean
   ~0.06 m, max 0.125 m. Jobs on a spacing band or GLO-90 border show a few meters, up to ~15 m,
   because the sampler leaves out neighbors on a different lattice. **Fails above 50 m**, which
   only a pipeline bug produces.
2. **ArduPilot AW3D30 (independent dataset).** Every pixel of the job against ArduPilot's SRTM1
   cells from `terrain.ardupilot.org` (JAXA AW3D30 repackaged as `.hgt`), read by a plain-numpy
   pixel-is-point sampler that shares no georeferencing code with the generator. Reports bias,
   land written as sea, pixels more than 50 m too low (the unsafe direction), and a least-squares
   horizontal offset in meters, reported as `n/a` on terrain too planar to tell a shift from a
   vertical bias. Too-low pixels are only judged where the slope is under 0.5; on
   steeper ground the datasets' horizontal offset alone produces such differences. **Fails** if
   land-as-sea exceeds 1% of land, too-low exceeds 5% of gentle land, or the offset exceeds
   40 m.
3. **Tilezen (informational).** Sub-pixel offset of today's client dataset on the most rugged
   z13 tiles, to see how far terrain moves when the client switches.

### Whole-build Check

`validate` measures accuracy on a sample of jobs. `check` reads **every** tile, so a fault in a
job the sample missed cannot reach an upload; a production upload requires a passing check newer
than the overviews. Modeled on the tools ArduPilot added after the terraingen bugs
(`offline_check.py`, `find_steep.py`), it reports:

- **Structure:** each tile of a completed job and every parent of an existing tile exists, is a
  readable 256×256 RGB PNG, has heights on the 0.25 m steps within −500..9000 m, and is the
  shared sea PNG if it is all sea.
- **Seams:** at every tile boundary, including between independently generated jobs and across
  the antimeridian, the mean jump across the boundary must stay within 3× the mean step beside it
  (+0.5 m for rounding). Real terrain is as steep across a boundary as beside it; a misplaced,
  stale or zeroed tile is not.
- **Jumps:** neighboring pixels more than 1,000 m apart (or 2× the pixel's ground size at coarse
  zooms, where pixels average large areas).

Overview tiles with missing children (a partial build, filled with sea level by design) get the
structural checks only. Each job reads the edge tiles of its east and south neighbors, so memory
stays per job. It costs about 1.5 ms of CPU per tile (measured on the sample areas), so roughly
40 minutes on 64 vCPUs for a full build. On the sample areas the check passes, and it caught each
of three faults planted in real tiles (two swapped z13 tiles, a z13 tile zeroed on land, a z12
tile copied from another continent) on all four sides.

### Comparing Builds

`diff --out NEW --against OLD` compares two local builds tile by tile: per zoom, how many tiles
are identical, changed (with the largest height change), added, removed or unreadable, then the
largest changes. An unreadable tile counts as changed instead of stopping the run; `check` says
why. `--list FILE` streams the changed and added tile paths to a file, which is what an upload
of only the changes would send (see [Open Items](#open-items)). Both builds must be local; the
previous one is gone once its instance is, so keep its volume or sync it down before
regenerating.

### Expected Differences Between Datasets

- **~8–20 m horizontal offset, mostly north–south.** Copernicus sits about 0.3–0.45 of a 1″
  pixel north of both AW3D30 and Tilezen. Comparing the raw Copernicus cells to AW3D30 on their
  native grids, with none of this code involved, shows the same offset, so it is a property of
  the datasets, not the pipeline.
- **A few meters of bias.** Different sensors and epochs (TanDEM-X radar 2011–2015, JAXA
  optical stereo 2006–2011).
- **Tens of meters on cliffs, coastlines, snow and ice.** Raw Copernicus is more than 50 m below
  AW3D30 on up to 3.3% of gentle land above 5,000 m near Everest (radar penetration into snow,
  glacier change, AW3D30 void fill), but under 0.2% below 5,000 m. Hence the 5% limit for
  too-low pixels; real failures are far above it.

Run against tiles from the first version of this pipeline, which treated GLO-30's missing cells
as ocean, the validator flagged the Aragats job: 97.6% of its land written as sea.

### Sample Areas

`generate --area all` builds 17 jobs covering:

| Area | Covers |
| ---- | ------ |
| `alps`, `everest`, `aconcagua` | High relief, each hemisphere, high-altitude snow and ice |
| `sf-coast` | Coastline and open ocean |
| `kansas` | Flat plains |
| `jotunheimen`, `svalbard` | 2″ and 3″ longitude bands, fjords, glaciers |
| `oslo-60n` | A job spanning the 1.5″/2″ band boundary |
| `aragats` | Inside a GLO-90 gap-fill cell |
| `armenia-40n` | GLO-90 north of GLO-30, where the pixel areas leave a gap to fill |
| `caucasus-42n` | GLO-30 north of GLO-90, where the pixel areas overlap |
| `taveuni-180e`, `taveuni-180w` | Land crossed by the antimeridian: the jobs either side of 180° |

## Size and Cost Estimates

For a full z0–z13 build. These are planning estimates for the infrastructure sign-off in
[#15090](https://github.com/mavlink/qgroundcontrol/issues/15090); measure a trial run on the
real instance before relying on the run time.

### Size

Mean tile sizes measured on the sample areas (`stats`). The samples are land-heavy and
mountainous, so these are upper estimates; flat land and ice compress better.

| Zoom | 13 | 12 | 11 | 10 | 9 |
| ---- | -- | -- | -- | -- | - |
| Mean land tile | 37 KB | 48 KB | 61 KB | 74 KB | 84 KB |
| Land tiles (approx.) | 32.5M | 8.1M | 2.0M | 0.5M | 0.13M |
| Total | 1,190 GB | 390 GB | 125 GB | 37 GB | 11 GB |

z13 tiles compress best because they upsample the ~30 m source. "Land tiles" are tiles touching
a Copernicus cell (8.1M at z12, counted; the other zooms scale by 4). Web Mercator stretches
Greenland and Antarctica, so this is about half of each zoom.

| | Amount |
| --- | ------ |
| Objects, z0–z13 | 89.5M (every key exists, ocean included) |
| Land tiles | ~1.75 TB |
| Ocean tiles | ~46M × 564 B ≈ 26 GB |
| Total | **~1.8 TB** |

### Cost

AWS list prices assumed (checked October 2026; check again before sign-off): S3 Standard
us-west-2 $0.023/GB-month and $0.005 per 1,000 PUT; inter-region transfer from eu-central-1
$0.02/GB; c7gd.16xlarge in eu-central-1 ~$3.36/hour (64 vCPU, 128 GiB, 2×1.9 TB NVMe);
CloudFront $0.085/GB and $0.01 per 10,000 HTTPS requests beyond the always-free 1 TB and 10M
requests a month.

| One-time | Basis | Cost |
| -------- | ----- | ---- |
| S3 PUT requests | 89.5M objects | ~$450 |
| Transfer eu-central-1 → us-west-2 | ~1.8 TB | ~$36 |
| Reading Copernicus | Same-region AWS Open Data | $0 |
| EC2 c7gd.16xlarge | ~1–2 days, see below | ~$80–160 |
| **Total** | | **~$570–650** |

| Ongoing | Basis | Cost |
| ------- | ----- | ---- |
| S3 storage | ~1.8 TB | ~$41/month |
| CloudFront within the free tier | ≤ 1 TB and ≤ 10M requests a month | $0 |
| CloudFront beyond the free tier | Per 1M z13 tile requests (~37 GB) | ~$4 |

Requests run out before bytes: 10M free requests is about 370 GB of z13 tiles.

**Run time.** Generation needs roughly 10 CPU-seconds per land job and much less per ocean job,
so about 400 CPU-hours for the ~127k land jobs: about 6 hours on 64 vCPUs. Uploading 89.5M
objects dominates. The upload runs from eu-central-1 to us-west-2, where a request takes about
150 ms, so the AWS CLI's default of 10 concurrent requests manages under a hundred PUTs a second:
days to weeks for a full upload. Raise it, e.g.
`aws configure set s3.max_concurrent_requests 256 --profile <profile>`, for about 1,000–3,000
PUTs a second, roughly 8–25 hours.

**Disk.** Tiles are written locally, then uploaded, so the run needs ~1.8 TB of scratch.
A c7gd.8xlarge (1.9 TB NVMe, ~$1.68/hour) is too tight; use a c7gd.16xlarge. Uploading in
batches and deleting is not an option: a production upload takes one complete local build.

For comparison, the original #15090 estimate (z12, full precision) was 22.4M objects, ~$112 in
PUT fees and ~400 GB.

## Unit Tests

[`../tests/test_terrain.py`](../tests/test_terrain.py) (stdlib only) covers tile math, cell
naming and selection, GLO-90 gap filling in the cell map, bounded job scheduling, download
retries, the CLI's argument checks, and the upload targets, cache policy, publish order and
production checks. [`../tests/test_terrain_pipeline.py`](../tests/test_terrain_pipeline.py) needs the
`terrain` group and skips without it. Its core tests write small synthetic Copernicus-style
cells holding a **linear height ramp** in lon/lat: bilinear interpolation reproduces a linear
field exactly, so every generated pixel must equal the analytic ramp at its center. That checks
mosaic, lattice alignment, warp and Mercator math together, with no network. Cases:

- Same-resolution cells: exact across 1° borders.
- Cells in two longitude bands: exact across the band boundary, and a job's heights do not
  change when the cells it buffers include another lattice (checked on a nonlinear field, which
  a ramp cannot do).
- GLO-90 beside GLO-30: the lattice gap is filled, never sea.
- Missing cells stay sea level, and void filling never spreads land into them.
- Small voids are interpolated; unfillable voids fail the job.
- Rounding: nearest 0.25 m step, error at most half a step, no bias; near-zero tiles become sea.
- The validation sampler interpolates across east and south cell edges; source samples match the
  requested count, and a job with none to compare is reported instead of crashing.
- A broken worker pool counts every unfinished job as failed.
- The antimeridian: a job ending at 180°E matches the ramp exactly up to its last pixel, the
  validation sampler interpolates across 180°, and `check` finds a seam planted across it.
- The whole-build check: each structural fault is reported; seams are flagged only when the jump
  dwarfs the steps beside it (not on steep real slopes); planted faults (missing tile, seam
  inside a job, seam between jobs, jump, missing parent) are found and block the passed marker;
  partial-build overviews are not flagged. `diff` counts identical, changed, added, removed and
  unreadable tiles and streams the changed and added ones to the list file.
- Encoding round trip, clamping, averaging heights not RGB, pyramid placement, overviews,
  TileJSON contents, offset fit recovery, and `.hgt` parsing.

```bash
cd tools && uv run --group test --group terrain pytest -q tests/test_terrain.py tests/test_terrain_pipeline.py
```

## Running

```bash
TT="uv run --project tools --group terrain python tools/terrain_tiles.py"
$TT generate --out build/terrain --area all
$TT validate --out build/terrain
$TT validate --out build/terrain --sample-jobs 200   # After a global run
$TT overviews --out build/terrain
$TT stats --out build/terrain
$TT check --out build/terrain                         # Every tile; required for production
$TT diff --out build/terrain --against build/terrain-previous --list changed.txt
$TT upload --out build/terrain --prefix validation --data-version 0.1.0 --profile qgc-terrain --dryrun
$TT upload --out build/terrain --production --data-version 1.0.0 --notice "Initial terrain data" --profile qgc-terrain --dryrun
```

- Jobs are bound by source reads. Run the global `generate --all` on EC2 in eu-central-1, next
  to the source buckets; from elsewhere a job takes 10–100 s.
- GDAL's block cache is capped at 512 MB per process (`GDAL_ENV`); its default, 5% of physical
  RAM, would be multiplied by every pool worker.
- Generation is resumable: rerunning skips jobs with a done marker (`--force` regenerates).
  Failed jobs leave no marker and are retried on the next run.
- `validate` caches tile lists in `<out>/_source/` and ArduPilot cells (~10 MB each) in
  `<out>/_reference/ardupilot/`.
- Always `--dryrun` an upload first: it lists the tiles to upload and, with `--mirror`, the
  tiles to delete. A dry run skips the CloudFront invalidation.
- For a full upload, raise the CLI's S3 concurrency first (see
  [Size and Cost Estimates](#size-and-cost-estimates)).

## Open Items

- Confirm the Copernicus licence attribution wording (`metadata.ATTRIBUTION_NOTICES`).
- Infrastructure sign-off for the global run: EC2 access in eu-central-1, an instance role for
  the bucket, and the costs above.
- Global-run disk: ~1.8 TB of output needs a ≥ 3.8 TB instance (c7gd.16xlarge), since a
  production upload takes one complete local build.
- The validation sampler leaves out neighbors on a different lattice at band and GLO-90 edges
  (renormalizing the rest), so it is approximate within one source pixel of those lines.
- Upload only changed tiles. Regenerated tiles are newer than their S3 copies, so `aws s3 sync`
  re-uploads every one of them, even byte-identical ones: ~$450 in PUTs for a full
  regeneration. `diff --list` already produces the changed and added tiles against a local copy
  of the previous build; what remains is uploading from that list (or comparing local MD5s with
  S3 ETags, which needs no previous build).
- Enable S3 versioning on the bucket, with previous versions expiring after ~30 days, so an
  update can be rolled back. Only objects an update changed keep an extra copy.
- Confirm the uploading identity (and the EC2 instance role) may create CloudFront invalidations
  on `E2AZ33YAOTUR6`.
- Make the [Required QGC Client Changes](#required-qgc-client-changes) before switching QGC to
  these tiles.
