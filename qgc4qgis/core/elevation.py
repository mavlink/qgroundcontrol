"""Copernicus DEM (GLO-30) elevation provider pure core logic.

Data structures, URL formatting, tile grid partitioning, and response parsing
for the Copernicus DEM elevation service hosted at terrain-ce.suite.auterion.com.

GDAL and OSR (osgeo) are imported lazily, inside the writer functions, so
plugin loading does not depend on the GDAL bindings (fragile in environments
with NumPy 2.x shadowed). GDAL/OSR exceptions are enabled per module via
_enable_gdal_exceptions() to avoid importing osgeo.gdal_array.
"""

import json
import struct
from collections.abc import Sequence
from dataclasses import dataclass
from typing import TYPE_CHECKING

from qgis.core import QgsBlockingNetworkRequest
from qgis.PyQt.QtCore import QUrl
from qgis.PyQt.QtNetwork import QNetworkRequest

if TYPE_CHECKING:
    from osgeo import gdal

PROVIDER_URL: str = "https://terrain-ce.suite.auterion.com"
CARPET_PATH: str = "/api/v1/carpet"
TILE_SIZE_DEGREES: float = 0.01
MAX_TILES: int = 256
NODATA_VALUE: float = -9999.0
ATTRIBUTION: str = (
    "Copernicus DEM (GLO-30) — © Airbus Defence and Space GmbH, via terrain-ce.suite.auterion.com"
)


@dataclass
class CarpetTile:
    """Elevation carpet tile containing a 2D grid of height values.

    :param sw_lat: South-west latitude in degrees.
    :param sw_lon: South-west longitude in degrees.
    :param ne_lat: North-east latitude in degrees.
    :param ne_lon: North-east longitude in degrees.
    :param rows: 2D list of elevation values in meters.
    """

    sw_lat: float
    sw_lon: float
    ne_lat: float
    ne_lon: float
    rows: list[list[float]]

    @property
    def n_rows(self) -> int:
        """Return the number of rows in the carpet grid."""
        return len(self.rows)

    @property
    def n_cols(self) -> int:
        """Return the number of columns in the carpet grid."""
        return len(self.rows[0]) if self.rows else 0

    @property
    def lat_step(self) -> float:
        """Return latitude step size in degrees per grid cell."""
        if self.n_rows > 1:
            return (self.ne_lat - self.sw_lat) / (self.n_rows - 1)
        return 0.0

    @property
    def lon_step(self) -> float:
        """Return longitude step size in degrees per grid cell."""
        if self.n_cols > 1:
            return (self.ne_lon - self.sw_lon) / (self.n_cols - 1)
        return 0.0


def carpet_url(sw_lat: float, sw_lon: float, ne_lat: float, ne_lon: float) -> str:
    """Format API endpoint URL for carpet heights query.

    Formats coordinates with 10 decimal places matching
    TerrainQueryCopernicus::requestCarpetHeights.

    :param sw_lat: South-west latitude in degrees.
    :param sw_lon: South-west longitude in degrees.
    :param ne_lat: North-east latitude in degrees.
    :param ne_lon: North-east longitude in degrees.
    :return: Full URL string with query parameters.
    """
    points = f"{float(sw_lat):.10f},{float(sw_lon):.10f},{float(ne_lat):.10f},{float(ne_lon):.10f}"
    return f"{PROVIDER_URL}{CARPET_PATH}?points={points}"


def _axis_steps(sw: float, ne: float, step: float) -> list[tuple[float, float]]:
    if sw > ne:
        raise ValueError(f"South-west value {sw} cannot be greater than north-east value {ne}")
    if step <= 0:
        raise ValueError(f"tile_size must be positive, got {step}")

    ranges: list[tuple[float, float]] = []
    cur = sw
    eps = 1e-9
    while cur < ne - eps or (not ranges and cur == sw):
        nxt = cur + step
        ranges.append((round(cur, 10), round(nxt, 10)))
        cur = nxt
    return ranges


def tile_grid(
    sw_lat: float,
    sw_lon: float,
    ne_lat: float,
    ne_lon: float,
    tile_size: float = TILE_SIZE_DEGREES,
) -> list[tuple[float, float, float, float]]:
    """Generate a grid of tiles covering the bounding box.

    :param sw_lat: South-west latitude in degrees.
    :param sw_lon: South-west longitude in degrees.
    :param ne_lat: North-east latitude in degrees.
    :param ne_lon: North-east longitude in degrees.
    :param tile_size: Tile size in degrees (default TILE_SIZE_DEGREES).
    :return: List of tuples (tile_sw_lat, tile_sw_lon, tile_ne_lat, tile_ne_lon).
    :raises ValueError: If bounds are invalid or total tiles exceeds MAX_TILES.
    """
    lat_steps = _axis_steps(float(sw_lat), float(ne_lat), float(tile_size))
    lon_steps = _axis_steps(float(sw_lon), float(ne_lon), float(tile_size))

    total_tiles = len(lat_steps) * len(lon_steps)
    if total_tiles > MAX_TILES:
        raise ValueError(f"Tile count {total_tiles} exceeds MAX_TILES ({MAX_TILES})")

    grid: list[tuple[float, float, float, float]] = []
    for lat_sw, lat_ne in lat_steps:
        for lon_sw, lon_ne in lon_steps:
            grid.append((lat_sw, lon_sw, lat_ne, lon_ne))

    return grid


def parse_carpet(payload: dict) -> CarpetTile:
    """Parse API response payload dictionary into a CarpetTile instance.

    :param payload: Response JSON payload parsed into a dict.
    :return: CarpetTile instance containing bounds and 2D elevation grid.
    :raises ValueError: If status != 'success', carpet is empty, or row lengths differ.
    """
    if not isinstance(payload, dict):
        raise ValueError("Payload must be a dictionary")

    if payload.get("status") != "success":
        status = payload.get("status")
        raise ValueError(f"API response status is not success: {status!r}")

    data = payload.get("data")
    if not isinstance(data, dict):
        raise ValueError("Payload 'data' field missing or invalid")

    bounds = data.get("bounds")
    if not isinstance(bounds, dict):
        raise ValueError("Payload 'data.bounds' field missing or invalid")

    sw = bounds.get("sw")
    ne = bounds.get("ne")
    if not isinstance(sw, (list, tuple)) or len(sw) < 2:
        raise ValueError("Invalid or missing 'sw' bounds in payload")
    if not isinstance(ne, (list, tuple)) or len(ne) < 2:
        raise ValueError("Invalid or missing 'ne' bounds in payload")

    sw_lat, sw_lon = float(sw[0]), float(sw[1])
    ne_lat, ne_lon = float(ne[0]), float(ne[1])

    carpet = data.get("carpet")
    if not isinstance(carpet, list) or len(carpet) == 0:
        raise ValueError("Carpet array is empty or invalid")

    rows: list[list[float]] = []
    expected_cols: int | None = None

    for i, row in enumerate(carpet):
        if not isinstance(row, list):
            raise ValueError(f"Row {i} in carpet is not a list")
        if expected_cols is None:
            expected_cols = len(row)
            if expected_cols == 0:
                raise ValueError("Carpet row is empty")
        elif len(row) != expected_cols:
            raise ValueError(
                f"Row {i} length ({len(row)}) differs from first row length ({expected_cols})"
            )
        rows.append([float(val) for val in row])

    return CarpetTile(
        sw_lat=sw_lat,
        sw_lon=sw_lon,
        ne_lat=ne_lat,
        ne_lon=ne_lon,
        rows=rows,
    )


def _enable_gdal_exceptions() -> None:
    """Enable GDAL and OSR exceptions per module without importing osgeo.gdal_array.

    In GDAL >= 3.7, the public UseExceptions() method propagates state by importing
    osgeo.gdal_array, a C-extension binary linked to NumPy that breaks in environments
    with NumPy 2.x. Enabling exceptions per module avoids loading osgeo.gdal_array
    while ensuring exceptions are enabled for GDAL and OSR.
    """
    from osgeo import gdal, osr

    for module in (gdal, osr):
        enable = getattr(module, "_UseExceptions", None)
        if enable is None:
            enable = module.UseExceptions
        enable()


def carpet_to_dataset(tile: CarpetTile) -> "gdal.Dataset":
    """Create an in-memory GDAL Dataset (driver MEM) from a CarpetTile.

    Inverts rows (`tile.rows[::-1]`) because row 0 from API represents the south.
    Writes band using WriteRaster and struct (without NumPy — builds with gdal_array compiled against NumPy 1.x fail under NumPy 2.x).

    :param tile: CarpetTile instance containing elevation grid and geographic bounds.
    :return: In-memory GDAL Dataset object.
    """
    from osgeo import gdal, osr

    _enable_gdal_exceptions()
    driver = gdal.GetDriverByName("MEM")
    ds = driver.Create("", tile.n_cols, tile.n_rows, 1, gdal.GDT_Float32)

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)
    ds.SetProjection(srs.ExportToWkt())

    geotransform = (
        tile.sw_lon - tile.lon_step / 2.0,
        tile.lon_step,
        0.0,
        tile.ne_lat + tile.lat_step / 2.0,
        0.0,
        -tile.lat_step,
    )
    ds.SetGeoTransform(geotransform)

    band = ds.GetRasterBand(1)
    band.SetNoDataValue(NODATA_VALUE)

    values = [value for row in tile.rows[::-1] for value in row]
    band.WriteRaster(0, 0, tile.n_cols, tile.n_rows, struct.pack(f"={len(values)}f", *values))

    return ds


def build_dem_geotiff(tiles: Sequence[CarpetTile], out_path: str) -> str:
    """Build a composite GeoTIFF DEM file from a sequence of CarpetTile objects.

    :param tiles: Sequence of CarpetTile instances.
    :param out_path: Target file path for the generated GeoTIFF.
    :return: Output file path string `out_path`.
    :raises ValueError: If `tiles` sequence is empty.
    """
    from osgeo import gdal

    _enable_gdal_exceptions()
    if not tiles:
        raise ValueError("Tiles sequence cannot be empty")

    datasets = [carpet_to_dataset(tile) for tile in tiles]
    warped_ds = gdal.Warp(
        out_path,
        datasets,
        format="GTiff",
        dstNodata=NODATA_VALUE,
        creationOptions=["COMPRESS=DEFLATE", "TILED=YES"],
    )
    del warped_ds
    datasets = None

    return out_path


def fetch_carpet(url: str) -> dict:
    """Fetch elevation carpet JSON payload from the given URL using QgsBlockingNetworkRequest.

    :param url: Full API endpoint URL string.
    :return: Response JSON payload parsed into a dictionary.
    :raises RuntimeError: If network request fails or response body is not valid JSON.
    """
    req = QgsBlockingNetworkRequest()
    net_req = QNetworkRequest(QUrl(url))
    err = req.get(net_req)
    if err != QgsBlockingNetworkRequest.NoError:
        msg = (
            req.errorMessage()
            or req.reply().errorString()
            or f"Network request failed with error code {err}"
        )
        raise RuntimeError(msg)

    content = req.reply().content().data()
    try:
        data = json.loads(content.decode("utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError) as exc:
        raise RuntimeError(f"Invalid JSON response: {exc}") from exc

    if not isinstance(data, dict):
        raise RuntimeError(f"Invalid JSON response: expected dict, got {type(data).__name__}")

    return data


def download_dem(
    sw_lat: float,
    sw_lon: float,
    ne_lat: float,
    ne_lon: float,
    out_path: str,
    *,
    fetch=fetch_carpet,
    feedback=None,
) -> str:
    """Download elevation carpet tiles for a bounding box and generate a composite GeoTIFF DEM.

    :param sw_lat: South-west latitude in degrees.
    :param sw_lon: South-west longitude in degrees.
    :param ne_lat: North-east latitude in degrees.
    :param ne_lon: North-east longitude in degrees.
    :param out_path: Target GeoTIFF file path.
    :param fetch: Function to fetch carpet JSON dictionary given a URL string (default fetch_carpet).
    :param feedback: Optional feedback object supporting setProgress and isCanceled.
    :return: Output file path string `out_path`.
    """
    grid = tile_grid(sw_lat, sw_lon, ne_lat, ne_lon)
    total_tiles = len(grid)
    tiles: list[CarpetTile] = []

    for i, (tile_sw_lat, tile_sw_lon, tile_ne_lat, tile_ne_lon) in enumerate(grid):
        if feedback is not None and feedback.isCanceled():
            break

        url = carpet_url(tile_sw_lat, tile_sw_lon, tile_ne_lat, tile_ne_lon)
        payload = fetch(url)
        tile = parse_carpet(payload)
        tiles.append(tile)

        if feedback is not None:
            feedback.setProgress(int(100.0 * (i + 1) / total_tiles))

    return build_dem_geotiff(tiles, out_path)
