"""Warp Copernicus cells into job tiles (JOB_ZOOM) and write their MAX_ZOOM..JOB_ZOOM pyramid."""

from __future__ import annotations

import math
import time
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures.process import BrokenProcessPool
from typing import TYPE_CHECKING, Any, NamedTuple
from xml.sax.saxutils import escape

import numpy as np
import rasterio
from rasterio.dtypes import dtype_rev, typename_fwd
from rasterio.fill import fillnodata
from rasterio.merge import merge
from rasterio.transform import from_bounds
from rasterio.warp import Resampling, reproject
from rasterio.windows import from_bounds as from_bounds_window

from . import terrarium
from .copernicus import COARSEST_PIXEL_DEG, MIN_VALID_HEIGHT, cells_for_bounds, wrap_lon
from .jobs import IN_FLIGHT_PER_WORKER, OCEAN_STATUS, bounded_completion, done_marker, mark_done
from .tilemath import (
    JOB_ZOOM,
    MAX_ZOOM,
    MERCATOR_HALF_EXTENT,
    TILE_SIZE,
    TileKey,
    children,
    lonlat_bounds,
    mercator_bounds,
)

if TYPE_CHECKING:
    from collections.abc import Callable, Mapping
    from concurrent.futures import Executor
    from pathlib import Path

    from affine import Affine
    from rasterio.io import DatasetReader

JOB_TILES = 1 << (MAX_ZOOM - JOB_ZOOM)
JOB_PIXELS = JOB_TILES * TILE_SIZE
# About 3 km at 1 arc-second: bridges genuine source voids without inventing large areas.
VOID_FILL_PIXELS = 100
# Covers each grid's padding at the coarsest source pixel: the void-fill radius plus four kernel pixels.
CELL_BUFFER_DEG = (VOID_FILL_PIXELS + 4) * COARSEST_PIXEL_DEG
_MERCATOR_RADIUS = MERCATOR_HALF_EXTENT / math.pi

GDAL_ENV: dict[str, Any] = {
    "GDAL_DISABLE_READDIR_ON_OPEN": "EMPTY_DIR",
    "CPL_VSIL_CURL_ALLOWED_EXTENSIONS": ".tif",
    "GDAL_HTTP_MULTIRANGE": "YES",
    "GDAL_HTTP_MERGE_CONSECUTIVE_RANGES": "YES",
    "GDAL_HTTP_MAX_RETRY": "5",
    "GDAL_HTTP_RETRY_DELAY": "2",
    "VSI_CACHE": "TRUE",
    # MB per process; GDAL's default (5% of physical RAM) multiplies across pool workers.
    "GDAL_CACHEMAX": 512,
}


def tile_path(out: Path, key: TileKey) -> Path:
    return out / "terrarium" / str(key.z) / str(key.x) / f"{key.y}.png"


def job_cells(job: TileKey, cells: Mapping[tuple[int, int], str]) -> dict[tuple[int, int], str]:
    return cells_for_bounds(*lonlat_bounds(job), cells, CELL_BUFFER_DEG)


def lattice_bounds(
    west: float, south: float, east: float, north: float, res_x: float, res_y: float
) -> tuple[float, float, float, float]:
    """Expand a box to whole source pixels; their centers sit on whole multiples of the resolution."""

    def lower(value: float, res: float) -> float:
        return (math.floor(value / res + 0.5) - 0.5) * res

    def upper(value: float, res: float) -> float:
        return (math.ceil(value / res + 0.5) - 0.5) * res

    return lower(west, res_x), lower(south, res_y), upper(east, res_x), upper(north, res_y)


class Group(NamedTuple):
    """The cells sharing one source lattice, merged into a native grid around the job."""

    res: tuple[float, float]
    keys: list[tuple[int, int]]
    heights: np.ndarray
    transform: Affine


def _open_cell(lon: int, url: str) -> DatasetReader:
    """Open a source cell, shifted by 360 degrees when *lon* is past the antimeridian.

    A job ending at 180 degrees east interpolates its last pixels from the cell at -180 placed
    at lon 180 (and the reverse at -180). Without it those pixels have no data on one side
    and are written as sea level.
    """
    shift = lon - wrap_lon(lon)
    if not shift:
        return rasterio.open(url)
    with rasterio.open(url) as source:
        width, height = source.width, source.height
        t = source.transform
        dtype, nodata = source.dtypes[0], source.nodata
    filename = f"/vsicurl/{url}" if url.startswith(("http://", "https://")) else url
    nodata_xml = "" if nodata is None else f"<NoDataValue>{nodata!r}</NoDataValue>"
    vrt = (
        f'<VRTDataset rasterXSize="{width}" rasterYSize="{height}"><SRS>EPSG:4326</SRS>'
        f"<GeoTransform>{t.c + shift!r}, {t.a!r}, {t.b!r}, {t.f!r}, {t.d!r}, {t.e!r}</GeoTransform>"
        f'<VRTRasterBand dataType="{typename_fwd[dtype_rev[dtype]]}" band="1">{nodata_xml}'
        f'<SimpleSource><SourceFilename relativeToVRT="0">{escape(filename)}</SourceFilename>'
        "<SourceBand>1</SourceBand></SimpleSource></VRTRasterBand></VRTDataset>"
    )
    return rasterio.open(vrt)


def _merge_same_resolution(
    datasets: list[DatasetReader],
    bounds: tuple[float, float, float, float],
    res: tuple[float, float],
) -> tuple[np.ndarray, Affine]:
    """Exact pixel copy: cells sharing a resolution share one pixel lattice.

    Never merge mixed resolutions directly: rasterio rounds the coarser source windows to
    whole pixels, shifting that data by up to a pixel (~23 m at 60 degrees north).
    """
    merged, transform = merge(
        datasets,
        bounds=lattice_bounds(*bounds, *res),
        res=res,
        nodata=np.nan,
        dtype="float32",
    )
    heights = merged[0]
    heights[heights < MIN_VALID_HEIGHT] = np.nan
    return heights, transform


def _read_groups(job: TileKey, cells: Mapping[tuple[int, int], str]) -> list[Group]:
    """Merge the job's cells into one native-lattice grid per resolution, finest first.

    A grid reaches past the job by the bilinear kernel, the void-fill radius and the kernel
    again, so its own ragged edge stays clear. Its extent depends only on its resolution, so
    every job around a cell sees the same data.
    """
    west, south, east, north = lonlat_bounds(job)
    keys = list(cells)
    datasets: list[DatasetReader] = []
    try:
        for (_, lon), url in cells.items():
            datasets.append(_open_cell(lon, url))
        members: dict[tuple[float, float], list[int]] = defaultdict(list)
        for index, dataset in enumerate(datasets):
            members[dataset.res].append(index)
        groups = []
        for res in sorted(members, key=lambda res: res[0] * res[1]):
            pad_x = (4 + VOID_FILL_PIXELS) * res[0]
            pad_y = (4 + VOID_FILL_PIXELS) * res[1]
            padded = (west - pad_x, south - pad_y, east + pad_x, north + pad_y)
            heights, transform = _merge_same_resolution(
                [datasets[i] for i in members[res]], padded, res
            )
            groups.append(Group(res, [keys[i] for i in members[res]], heights, transform))
        return groups
    finally:
        for dataset in datasets:
            dataset.close()


def _complete_group(job: TileKey, group: Group, others: list[Group]) -> tuple[np.ndarray, int]:
    """The group's grid with nodes it lacks taken from the other groups, then its voids filled.

    Nodes beyond the group's own cells (a neighboring band or GLO-90 cell, whose lattice
    differs) are resampled from the other groups, so a cell interpolates on its own lattice
    and blends with its neighbors only within a pixel of their shared edge. Returns the grid
    and the number of void nodes filled.
    """
    heights = group.heights
    shape = heights.shape
    for other in others:
        if not np.isfinite(other.heights).any():
            continue
        warped = np.full(shape, np.nan, dtype=np.float32)
        reproject(
            source=other.heights,
            destination=warped,
            src_transform=other.transform,
            src_crs="EPSG:4326",
            src_nodata=np.nan,
            dst_transform=group.transform,
            dst_crs="EPSG:4326",
            dst_nodata=np.nan,
            resampling=Resampling.bilinear,
            num_threads=1,
            # Plain bilinear even onto a coarser lattice: GDAL otherwise widens the kernel.
            XSCALE=1,
            YSCALE=1,
        )
        heights = np.where(np.isnan(heights), warped, heights)

    # Only voids inside the job footprint plus the kernel margin can reach the output.
    margin_x, margin_y = 2 * group.res[0], 2 * group.res[1]
    west, south, east, north = lonlat_bounds(job)
    footprint = from_bounds_window(
        west - margin_x,
        south - margin_y,
        east + margin_x,
        north + margin_y,
        transform=group.transform,
    )
    (row0, row1), (col0, col1) = footprint.round_offsets().round_lengths().toranges()
    # The padding guarantees this; a footprint outside the grid means the padding broke.
    if not (0 <= row0 <= row1 <= shape[0] and 0 <= col0 <= col1 <= shape[1]):
        raise RuntimeError(
            f"Job footprint rows {row0}:{row1}, cols {col0}:{col1} "
            f"falls outside the {shape[0]}x{shape[1]} grid"
        )
    # Coverage is whole 1-degree cells, not file extents. A pixel centered on a whole degree
    # is the top row (or first column) of the cell south (or east) of it, so the cell of a
    # pixel is that of its bottom (or right) edge.
    lat_cells = np.floor(group.transform.f + (np.arange(shape[0]) + 1) * group.transform.e).astype(
        int
    )
    lon_cells = np.floor(group.transform.c + (np.arange(shape[1]) + 1) * group.transform.a).astype(
        int
    )
    covered = np.zeros(shape, dtype=bool)
    for lat, lon in group.keys:
        covered[np.ix_(lat_cells == lat, lon_cells == lon)] = True
    in_footprint = np.zeros(shape, dtype=bool)
    in_footprint[row0:row1, col0:col1] = True
    covered &= in_footprint

    void_mask = np.isnan(heights) & covered
    voids = int(void_mask.sum())
    if voids:
        # fillnodata writes into its input and fills ocean too; only voids may take its values.
        filled = fillnodata(
            heights.copy(), mask=~np.isnan(heights), max_search_distance=VOID_FILL_PIXELS
        )
        heights = np.where(void_mask, filled, heights).astype(np.float32)
        unfilled = int((np.isnan(heights) & covered).sum())
        if unfilled:
            # Writing these as sea level would put land at 0 m, the failure terraingen shipped.
            raise RuntimeError(f"{unfilled} void pixels inside source cells could not be filled")
    return heights, voids


def _pixel_cells(job: TileKey) -> tuple[np.ndarray, np.ndarray]:
    """The 1-degree cell latitude of each output row and longitude of each column center."""
    left, _, right, top = mercator_bounds(job)
    west, _, east, _ = lonlat_bounds(job)
    centers = np.arange(JOB_PIXELS) + 0.5
    lats = np.degrees(
        np.arctan(np.sinh((top - centers * (right - left) / JOB_PIXELS) / _MERCATOR_RADIUS))
    )
    lons = west + centers * (east - west) / JOB_PIXELS
    return np.floor(lats).astype(int), np.floor(lons).astype(int)


def warp_job(job: TileKey, cells: Mapping[tuple[int, int], str]) -> tuple[np.ndarray, int]:
    """Warp the job's cells into its MAX_ZOOM-resolution mercator grid; returns voids filled.

    Each output pixel is sampled once, bilinearly, from the native lattice of the cell that
    contains it. Resampling everything onto one lattice would interpolate a cell's data twice
    in a job that also reads a finer neighbor, and not in the job next to it.
    """
    groups = _read_groups(job, cells)
    rows, cols = _pixel_cells(job)
    heights = np.full((JOB_PIXELS, JOB_PIXELS), np.nan, dtype=np.float32)
    destination = from_bounds(*mercator_bounds(job), JOB_PIXELS, JOB_PIXELS)
    voids = 0
    for group in groups:
        owned = np.zeros(heights.shape, dtype=bool)
        for lat, lon in group.keys:
            owned[np.ix_(rows == lat, cols == lon)] = True
        if not owned.any():
            continue
        grid, filled = _complete_group(job, group, [g for g in groups if g is not group])
        voids += filled
        warped = np.full(heights.shape, np.nan, dtype=np.float32)
        reproject(
            source=grid,
            destination=warped,
            src_transform=group.transform,
            src_crs="EPSG:4326",
            src_nodata=np.nan,
            dst_transform=destination,
            dst_crs="EPSG:3857",
            dst_nodata=np.nan,
            resampling=Resampling.bilinear,
            num_threads=1,
        )
        heights[owned] = warped[owned]
    return np.nan_to_num(heights, nan=terrarium.SEA_LEVEL), voids


def write_tile(out: Path, key: TileKey, heights: np.ndarray) -> None:
    path = tile_path(out, key)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(terrarium.tile_png_bytes(heights))


def write_job_pyramid(out: Path, job: TileKey, heights: np.ndarray) -> None:
    level = heights
    for z in range(MAX_ZOOM, JOB_ZOOM - 1, -1):
        depth = z - JOB_ZOOM
        for ty in range(1 << depth):
            for tx in range(1 << depth):
                block = level[
                    ty * TILE_SIZE : (ty + 1) * TILE_SIZE, tx * TILE_SIZE : (tx + 1) * TILE_SIZE
                ]
                write_tile(out, TileKey(z, (job.x << depth) + tx, (job.y << depth) + ty), block)
        if z > JOB_ZOOM:
            level = terrarium.downsample(level)


def run_job(out: Path, job: TileKey, cells: Mapping[tuple[int, int], str], force: bool) -> str:
    if done_marker(out, job).exists() and not force:
        return "skipped"
    if cells:
        with rasterio.Env(**GDAL_ENV):
            heights, voids_filled = warp_job(job, cells)
        status = f"{len(cells)} cells, {voids_filled} void px filled"
    else:
        heights = np.zeros((JOB_PIXELS, JOB_PIXELS), dtype=np.float32)
        status = OCEAN_STATUS
    done_marker(out, job).unlink(missing_ok=True)
    write_job_pyramid(out, job, heights)
    mark_done(out, job, status)
    return status


def generate(
    out: Path,
    jobs: list[TileKey],
    cells: Mapping[tuple[int, int], str],
    workers: int,
    force: bool,
    executor_factory: Callable[..., Executor] = ProcessPoolExecutor,
) -> int:
    """Run all jobs in a process pool; returns the number of failed jobs.

    If a worker dies (e.g. out of memory) the pool breaks: every job not yet finished counts
    as failed, and a rerun resumes from the done markers.
    """
    failures = 0
    finished = 0
    start = time.monotonic()
    with executor_factory(max_workers=workers) as pool:
        completed = bounded_completion(
            lambda job: pool.submit(run_job, out, job, job_cells(job, cells), force),
            jobs,
            limit=workers * IN_FLIGHT_PER_WORKER,
        )
        try:
            for job, future in completed:
                finished += 1
                try:
                    status = future.result()
                except Exception as error:
                    failures += 1
                    status = f"FAILED: {error}"
                elapsed = time.monotonic() - start
                print(f"[{finished}/{len(jobs)} {elapsed:7.1f}s] job {job}: {status}", flush=True)
        except BrokenProcessPool as error:
            unfinished = len(jobs) - finished
            failures += unfinished
            print(f"Worker pool broke ({error}); {unfinished} job(s) not run", flush=True)
    return failures


def build_overviews(out: Path) -> None:
    """Build zooms below JOB_ZOOM from existing tiles; missing children are sea level."""
    # Written last on success, so its absence fails the production gate after an interruption.
    tile_path(out, TileKey(0, 0, 0)).unlink(missing_ok=True)
    for z in range(JOB_ZOOM - 1, -1, -1):
        child_dir = out / "terrarium" / str(z + 1)
        parents = {
            TileKey(z, int(path.parent.name) >> 1, int(path.stem) >> 1)
            for path in child_dir.glob("*/*.png")
        }
        missing = 0
        for key in sorted(parents):
            mosaic = np.zeros((2 * TILE_SIZE, 2 * TILE_SIZE), dtype=np.float32)
            for index, child in enumerate(children(key)):
                path = tile_path(out, child)
                if not path.is_file():
                    missing += 1
                    continue
                row, col = divmod(index, 2)
                mosaic[
                    row * TILE_SIZE : (row + 1) * TILE_SIZE, col * TILE_SIZE : (col + 1) * TILE_SIZE
                ] = terrarium.read_heights(path)
            write_tile(out, key, terrarium.downsample(mosaic))
        print(f"z{z}: {len(parents)} tiles ({missing} missing children filled with sea level)")
