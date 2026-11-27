"""Accuracy checks of completed jobs against their source and independent references."""

from __future__ import annotations

import math
import statistics
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures.process import BrokenProcessPool
from dataclasses import dataclass, field
from typing import TYPE_CHECKING

import numpy as np
import rasterio
from common.net import fetch_bytes
from rasterio.windows import Window

from . import terrarium
from .check import east_of, edge_findings, own_findings, south_of
from .copernicus import MIN_VALID_HEIGHT, wrap_lon
from .generate import GDAL_ENV, JOB_PIXELS
from .jobs import IN_FLIGHT_PER_WORKER, bounded_completion, tile_path
from .tilemath import (
    MAX_ZOOM,
    MERCATOR_HALF_EXTENT,
    TILE_SIZE,
    TileKey,
    lonlat_bounds,
    lonlat_to_mercator,
    mercator_bounds,
    mercator_to_lonlat,
)

if TYPE_CHECKING:
    from collections.abc import Callable, Iterator, Mapping, Sequence
    from concurrent.futures import Executor
    from pathlib import Path

    from rasterio.io import DatasetReader

    from .ardupilot import HgtCells

TILEZEN_URL = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{key}.png"
# Against its own source only a pipeline bug moves a height this far; GDAL's widened
# downsampling kernel stays within ~15 m even on the steepest terrain.
MAX_SOURCE_ERROR = 50.0
# Copernicus and AW3D30 differ by ~10-20 m horizontally at the source; one 1 arc-second
# pixel more than that is a georeferencing bug.
MAX_REFERENCE_OFFSET_M = 40.0
# Reference land this far above our sea-level pixels counts as land written as sea.
LAND_AS_SEA_MIN_HEIGHT = 10.0
# Our height this far below the reference is the unsafe direction (more apparent clearance).
GROSSLY_LOW_MARGIN = 50.0
# Only judged on gentler slopes (rise/run): there the ~15 m horizontal offset between the
# datasets moves heights by under 10 m, so a 50 m drop means missing or zeroed data.
GROSSLY_LOW_MAX_SLOPE = 0.5
# Coastline and cliff-edge mismatches between the datasets stay well below this share of land.
MAX_LAND_AS_SEA_FRACTION = 0.01
# A check finding is in the source when the tile's pixels behind it are this close to the
# heights the source gives those pixels. Rounding to 0.25 m steps at each averaged zoom, and
# the last source pixel beside a missing cell, stay well inside it; a generator fault is the
# size of the finding itself.
FINDING_TOLERANCE_M = 1.0
# MAX_ZOOM pixels sampled per task: an overview pixel averages thousands of them.
_POINTS_PER_TASK = 200_000
# Raw Copernicus is >50 m below AW3D30 on up to ~3.3% of gentle land above 5000 m (Himalayan
# snow and ice: radar penetration, glacier change, AW3D30 void fill) with none of this pipeline
# involved. Real failures are far above this; smaller ones fail the source check.
MAX_GROSSLY_LOW_FRACTION = 0.05
_ALIGNMENT_MARGIN = 4
MAX_FIT_POINTS = 1_000_000
# Beyond this condition number of the (column-normalized) fit, gradients are too uniform for a
# shift to be told from a vertical bias, as on a plane or flat ground.
MAX_FIT_CONDITION = 100.0
_LATTICE_TOLERANCE = 1e-6


def job_tile_keys(job: TileKey) -> list[TileKey]:
    depth = MAX_ZOOM - job.z
    return [
        TileKey(MAX_ZOOM, (job.x << depth) + tx, (job.y << depth) + ty)
        for ty in range(1 << depth)
        for tx in range(1 << depth)
    ]


def read_job_heights(out: Path, job: TileKey) -> np.ndarray:
    """The job's decoded MAX_ZOOM tiles as one JOB_PIXELS square grid."""
    heights = np.empty((JOB_PIXELS, JOB_PIXELS), dtype=np.float32)
    depth = MAX_ZOOM - job.z
    for key in job_tile_keys(job):
        row = (key.y - (job.y << depth)) * TILE_SIZE
        col = (key.x - (job.x << depth)) * TILE_SIZE
        heights[row : row + TILE_SIZE, col : col + TILE_SIZE] = terrarium.read_heights(
            tile_path(out, key)
        )
    return heights


def pixel_center_lonlats(job: TileKey) -> tuple[np.ndarray, np.ndarray]:
    """Longitude per job column and latitude per job row of the MAX_ZOOM pixel centers."""
    left, _, right, top = mercator_bounds(job)
    resolution = (right - left) / JOB_PIXELS
    centers = (np.arange(JOB_PIXELS) + 0.5) * resolution
    lons = np.array([mercator_to_lonlat(left + offset, 0.0)[0] for offset in centers])
    lats = np.array([mercator_to_lonlat(0.0, top - offset)[1] for offset in centers])
    return lons, lats


class SourceSampler:
    """Point-bilinear Copernicus heights that interpolate across 1-degree cell edges.

    Each cell's pixel centers lie on whole multiples of its resolution, so a neighbor past
    the east or south edge is the first column or row of the adjacent cell. Neighbors in
    missing (ocean) cells or on a different lattice are left out and the weights renormalized,
    matching how the warper treats nodata.
    """

    def __init__(self, cells: Mapping[tuple[int, int], str]) -> None:
        self._cells = cells
        self._open: dict[tuple[int, int], DatasetReader | None] = {}

    def close(self) -> None:
        for dataset in self._open.values():
            if dataset is not None:
                dataset.close()

    def _dataset(self, key: tuple[int, int]) -> DatasetReader | None:
        if key not in self._open:
            url = self._cells.get(key)
            self._open[key] = rasterio.open(url) if url else None
        return self._open[key]

    def _lattice_value(self, key: tuple[int, int], col: int, row: int) -> float | None:
        dataset = self._dataset(key)
        if dataset is None:
            return None
        if col < 0 or row < 0:
            # Pixel centers lie on the cell's west and north edges, so only floating-point
            # rounding at a whole degree gets here; its interpolation weight is negligible.
            return None
        if col >= dataset.width or row >= dataset.height:
            lon = dataset.transform.c + (col + 0.5) * dataset.transform.a
            lat = dataset.transform.f + (row + 0.5) * dataset.transform.e
            neighbor_lon = key[1] + 1 if col >= dataset.width else key[1]
            # Past 180 degrees east the neighbor is the cell at -180, 360 degrees away.
            neighbor_key = (key[0] - 1 if row >= dataset.height else key[0], wrap_lon(neighbor_lon))
            lon -= neighbor_lon - neighbor_key[1]
            neighbor = self._dataset(neighbor_key)
            if neighbor is None:
                return None
            col_f = (lon - neighbor.transform.c) / neighbor.transform.a - 0.5
            row_f = (lat - neighbor.transform.f) / neighbor.transform.e - 0.5
            if max(abs(col_f - round(col_f)), abs(row_f - round(row_f))) > _LATTICE_TOLERANCE:
                return None
            dataset, col, row = neighbor, round(col_f), round(row_f)
        window = Window.from_slices((row, row + 1), (col, col + 1))
        value = float(dataset.read(1, window=window)[0, 0])
        return value if value >= MIN_VALID_HEIGHT else None

    def height(self, lon: float, lat: float) -> float | None:
        key = (math.floor(lat), math.floor(lon))
        dataset = self._dataset(key)
        if dataset is None:
            return terrarium.SEA_LEVEL
        col = (lon - dataset.transform.c) / dataset.transform.a - 0.5
        row = (lat - dataset.transform.f) / dataset.transform.e - 0.5
        col0, row0 = math.floor(col), math.floor(row)
        fx, fy = col - col0, row - row0
        total = weight_sum = 0.0
        for dc, dr, weight in (
            (0, 0, (1 - fx) * (1 - fy)),
            (1, 0, fx * (1 - fy)),
            (0, 1, (1 - fx) * fy),
            (1, 1, fx * fy),
        ):
            value = self._lattice_value(key, col0 + dc, row0 + dr)
            if value is not None and weight > 0:
                total += value * weight
                weight_sum += weight
        return total / weight_sum if weight_sum > 0 else None


def source_pixel_height(sampler: SourceSampler, z: int, px: int, py: int) -> float | None:
    """The height the source gives world pixel (*px*, *py*) of zoom *z*.

    At MAX_ZOOM that is the point-bilinear height at the pixel's center, as the generator
    samples it. A coarser pixel is the mean of the MAX_ZOOM pixels it covers, as the pyramid
    and the overviews average them. None where the source has a void.
    """
    scale = 1 << (MAX_ZOOM - z)
    resolution = 2 * MERCATOR_HALF_EXTENT / (TILE_SIZE << MAX_ZOOM)
    total = 0.0
    for sy in range(py * scale, (py + 1) * scale):
        for sx in range(px * scale, (px + 1) * scale):
            lon, lat = mercator_to_lonlat(
                (sx + 0.5) * resolution - MERCATOR_HALF_EXTENT,
                MERCATOR_HALF_EXTENT - (sy + 0.5) * resolution,
            )
            height = sampler.height(lon, lat)
            if height is None:
                return None
            total += height
    return total / (scale * scale)


@dataclass
class FindingCheck:
    """How far the tile's pixels behind one check finding are from the source."""

    key: TileKey
    text: str
    pixels: int
    # None when the source has a void under one of the pixels.
    max_abs: float | None

    @property
    def in_source(self) -> bool:
        return self.max_abs is not None and self.max_abs <= FINDING_TOLERANCE_M


def _pixel_errors(
    cells: Mapping[tuple[int, int], str], z: int, pixels: Sequence[tuple[int, int, float]]
) -> float | None:
    """The largest difference between tile heights and the source over world *pixels*."""
    sampler = SourceSampler(cells)
    worst = 0.0
    with rasterio.Env(**GDAL_ENV):
        try:
            for px, py, ours in pixels:
                expected = source_pixel_height(sampler, z, px, py)
                if expected is None:
                    return None
                worst = max(worst, abs(ours - expected))
        finally:
            sampler.close()
    return worst


def tile_findings(out: Path, key: TileKey) -> list[tuple[str, list[tuple[int, int, float]]]]:
    """Each check finding of a tile with its world pixels and their heights in the build."""
    heights = terrarium.read_heights(tile_path(out, key))
    east_path, south_key = tile_path(out, east_of(key)), south_of(key)
    east = terrarium.read_heights(east_path) if east_path.is_file() else None
    south_path = tile_path(out, south_key) if south_key else None
    south = terrarium.read_heights(south_path) if south_path and south_path.is_file() else None
    width = TILE_SIZE << key.z

    def world(row: int, col: int) -> tuple[int, int, float]:
        if col >= TILE_SIZE and east is not None:
            ours = east[row, col - TILE_SIZE]
        elif row >= TILE_SIZE and south is not None:
            ours = south[row - TILE_SIZE, col]
        else:
            ours = heights[row, col]
        return (key.x * TILE_SIZE + col) % width, key.y * TILE_SIZE + row, float(ours)

    findings = own_findings(key, heights) + edge_findings(key, heights, east, south)
    return [(finding.text, [world(*pixel) for pixel in finding.pixels]) for finding in findings]


def check_findings(
    out: Path,
    keys: Sequence[TileKey],
    cells: Mapping[tuple[int, int], str],
    workers: int,
    executor_factory: Callable[..., Executor] = ProcessPoolExecutor,
) -> list[FindingCheck]:
    """Compare the pixels behind every check finding of the tiles *keys* with the source.

    ``validate`` samples a job's pixels, so a passing job says nothing about a few pixels of
    it, or about a coarser tile. This compares the very pixels of a finding, at its zoom.
    """
    checks: list[FindingCheck] = []
    tasks: list[tuple[int, int, list[tuple[int, int, float]]]] = []
    for key in keys:
        per_task = max(1, _POINTS_PER_TASK >> (2 * (MAX_ZOOM - key.z)))
        for text, pixels in tile_findings(out, key):
            checks.append(FindingCheck(key, text, len(pixels), 0.0))
            tasks += [
                (len(checks) - 1, key.z, pixels[start : start + per_task])
                for start in range(0, len(pixels), per_task)
            ]
    with executor_factory(max_workers=workers) as pool:
        results = bounded_completion(
            lambda task: pool.submit(_pixel_errors, cells, task[1], task[2]),
            tasks,
            limit=workers * IN_FLIGHT_PER_WORKER,
        )
        for (index, _, _), future in results:
            check, worst = checks[index], future.result()
            check.max_abs = (
                None if worst is None or check.max_abs is None else max(check.max_abs, worst)
            )
    return checks


@dataclass
class SourceCheck:
    job: TileKey
    samples: int
    mean_abs: float
    p95_abs: float
    max_abs: float
    border_samples: int
    border_mean_abs: float


def _sample_pixels(
    job: TileKey, samples: int, rng: np.random.Generator
) -> list[tuple[int, int, bool]]:
    """Exactly *samples* job pixels: up to half within two pixels of the 1-degree cell borders
    the job crosses, the rest uniform."""
    if samples < 1:
        raise ValueError("Need at least one source sample per job")
    left, _, right, top = mercator_bounds(job)
    resolution = (right - left) / JOB_PIXELS
    west, south, east, north = lonlat_bounds(job)

    border_lines: list[tuple[str, int]] = []
    for lon in range(math.ceil(west), math.floor(east) + 1):
        border_lines.append(("col", int((lonlat_to_mercator(lon, 0)[0] - left) / resolution)))
    for lat in range(math.ceil(south), math.floor(north) + 1):
        border_lines.append(("row", int((top - lonlat_to_mercator(0, lat)[1]) / resolution)))
    border_budget = samples // 2 if border_lines else 0

    pixels = [
        (int(r), int(c), False)
        for r, c in rng.integers(0, JOB_PIXELS, size=(samples - border_budget, 2))
    ]
    for index, (axis, line) in enumerate(border_lines):
        count = border_budget // len(border_lines) + int(index < border_budget % len(border_lines))
        along = rng.integers(0, JOB_PIXELS, size=count)
        across = np.clip(line + rng.integers(-2, 2, size=count), 0, JOB_PIXELS - 1)
        for a, b in zip(along, across, strict=True):
            pixels.append((int(b), int(a), True) if axis == "row" else (int(a), int(b), True))
    return pixels


def check_source(
    job: TileKey,
    ours: np.ndarray,
    cells: Mapping[tuple[int, int], str],
    samples: int,
    rng: np.random.Generator,
) -> SourceCheck:
    left, _, right, top = mercator_bounds(job)
    resolution = (right - left) / JOB_PIXELS
    errors: list[float] = []
    border_errors: list[float] = []
    sampler = SourceSampler(cells)
    with rasterio.Env(**GDAL_ENV):
        try:
            for row, col, near_border in _sample_pixels(job, samples, rng):
                lon, lat = mercator_to_lonlat(
                    left + (col + 0.5) * resolution, top - (row + 0.5) * resolution
                )
                expected = sampler.height(lon, lat)
                if expected is None:
                    continue
                error = abs(float(ours[row, col]) - expected)
                errors.append(error)
                if near_border:
                    border_errors.append(error)
        finally:
            sampler.close()
    if not errors:
        # Every sample fell in an unfillable source void; the caller reports it as a problem.
        return SourceCheck(job, 0, math.nan, math.nan, math.nan, 0, math.nan)
    errors_array = np.asarray(errors)
    return SourceCheck(
        job=job,
        samples=len(errors),
        mean_abs=float(errors_array.mean()),
        p95_abs=float(np.percentile(errors_array, 95)),
        max_abs=float(errors_array.max()),
        border_samples=len(border_errors),
        border_mean_abs=statistics.fmean(border_errors) if border_errors else math.nan,
    )


def fit_offset(
    ours: np.ndarray,
    reference: np.ndarray,
    gradients: tuple[np.ndarray, np.ndarray] | None = None,
) -> tuple[float, float, float]:
    """Least-squares (dx, dy, bias) with reference ~= ours shifted by (dx, dy) pixels plus bias.

    *gradients* is ``np.gradient(ours)`` when the caller already has it. Large grids are fitted
    on an even stride of about MAX_FIT_POINTS pixels; three parameters need no more. Returns NaNs
    when the terrain is too planar to tell a shift from a bias.
    """
    grad_y, grad_x = gradients if gradients is not None else np.gradient(ours)
    stride = max(1, math.isqrt(ours.size // MAX_FIT_POINTS))
    window = (slice(_ALIGNMENT_MARGIN, -_ALIGNMENT_MARGIN, stride),) * 2
    difference = reference[window].astype(np.float64) - ours[window]
    valid = np.isfinite(difference)
    design = np.column_stack(
        [
            grad_x[window][valid].astype(np.float64),
            grad_y[window][valid].astype(np.float64),
            np.ones(int(valid.sum())),
        ]
    )
    scale = np.linalg.norm(design, axis=0)
    unmeasurable = (math.nan, math.nan, math.nan)
    if not scale.all():
        return unmeasurable
    singular = np.linalg.svd(design / scale, compute_uv=False)
    if singular[0] > MAX_FIT_CONDITION * singular[-1]:
        return unmeasurable
    (offset_x, offset_y, bias), *_ = np.linalg.lstsq(design, difference[valid], rcond=None)
    return float(offset_x), float(offset_y), float(bias)


@dataclass
class AlignmentCheck:
    key: TileKey
    offset_x: float
    offset_y: float
    bias: float


def _fetch_tilezen(key: TileKey) -> np.ndarray:
    return terrarium.heights_from_bytes(fetch_bytes(TILEZEN_URL.format(key=key), timeout=60))


def check_tilezen(job: TileKey, ours: np.ndarray, tile_count: int) -> list[AlignmentCheck]:
    """Sub-pixel offset of Tilezen relative to the most rugged MAX_ZOOM tiles of a job.

    *ours* is the job's decoded grid from read_job_heights. Informational only: the client
    was built against Tilezen, which sits ~10-15 m off Copernicus like AW3D30 does, and is
    too coarse at high latitudes for a reliable fit.
    """
    depth = MAX_ZOOM - job.z

    def tile(key: TileKey) -> np.ndarray:
        row = (key.y - (job.y << depth)) * TILE_SIZE
        col = (key.x - (job.x << depth)) * TILE_SIZE
        return ours[row : row + TILE_SIZE, col : col + TILE_SIZE]

    keys = job_tile_keys(job)
    rugged = sorted(keys, key=lambda key: float(tile(key).std()), reverse=True)[:tile_count]
    return [AlignmentCheck(key, *fit_offset(tile(key), _fetch_tilezen(key))) for key in rugged]


@dataclass
class ReferenceCheck:
    job: TileKey
    land_pixels: int
    median_diff: float
    median_abs: float
    p99_abs: float
    land_as_sea: int
    gentle_pixels: int
    grossly_low: int
    offset_east_m: float
    offset_north_m: float


def check_ardupilot(job: TileKey, ours: np.ndarray, hgt: HgtCells) -> ReferenceCheck | None:
    """Whole-job comparison against ArduPilot's AW3D30 cells (an independent DSM).

    Differences of a few meters are expected between the datasets; the checks target the
    failure classes ArduPilot's terraingen shipped: land written as sea level, heights far
    too low, and horizontal misregistration.
    """
    reference = hgt.sample_grid(*pixel_center_lonlats(job))
    land = np.isfinite(reference) & (reference > LAND_AS_SEA_MIN_HEIGHT)
    if not land.any():
        return None
    reference[~land] = np.nan
    difference = ours - reference
    land_difference = np.abs(difference[land])
    grad_y, grad_x = np.gradient(ours)
    offset_x, offset_y, _ = fit_offset(ours, reference, (grad_y, grad_x))
    left, _, right, _ = mercator_bounds(job)
    center_lat = (lonlat_bounds(job)[1] + lonlat_bounds(job)[3]) / 2
    pixel_m = (right - left) / JOB_PIXELS * math.cos(math.radians(center_lat))
    gentle = land & (np.hypot(grad_x, grad_y) < GROSSLY_LOW_MAX_SLOPE * pixel_m)
    return ReferenceCheck(
        job=job,
        land_pixels=int(land.sum()),
        median_diff=float(np.median(difference[land])),
        median_abs=float(np.median(land_difference)),
        p99_abs=float(np.percentile(land_difference, 99)),
        land_as_sea=int((land & (ours == terrarium.SEA_LEVEL)).sum()),
        gentle_pixels=int(gentle.sum()),
        grossly_low=int((gentle & (difference < -GROSSLY_LOW_MARGIN)).sum()),
        offset_east_m=offset_x * pixel_m,
        offset_north_m=-offset_y * pixel_m,
    )


def source_problems(result: SourceCheck) -> list[str]:
    if result.samples == 0:
        return [f"{result.job}: no source sample could be compared"]
    if result.max_abs > MAX_SOURCE_ERROR:
        return [f"{result.job}: {result.max_abs:.1f} m from the source"]
    return []


def reference_problems(ref: ReferenceCheck) -> list[str]:
    problems = []
    # An unmeasurable offset is NaN, which never exceeds the limit.
    if math.hypot(ref.offset_east_m, ref.offset_north_m) > MAX_REFERENCE_OFFSET_M:
        problems.append(
            f"{ref.job}: ArduPilot offset ({ref.offset_east_m:.1f}, {ref.offset_north_m:.1f}) m"
        )
    if ref.land_as_sea > MAX_LAND_AS_SEA_FRACTION * ref.land_pixels:
        problems.append(f"{ref.job}: {ref.land_as_sea} land as sea pixels of {ref.land_pixels}")
    if ref.grossly_low > MAX_GROSSLY_LOW_FRACTION * ref.gentle_pixels:
        problems.append(
            f"{ref.job}: {ref.grossly_low} grossly low pixels of {ref.gentle_pixels} on gentle slopes"
        )
    return problems


@dataclass
class JobValidation:
    """One job's check results and problems; a check that failed outright leaves its result unset."""

    job: TileKey
    problems: list[str] = field(default_factory=list)
    source: SourceCheck | None = None
    reference_checked: bool = False
    # None after a successful check means the job has no ArduPilot land.
    reference: ReferenceCheck | None = None
    alignments: list[AlignmentCheck] = field(default_factory=list)
    tilezen_error: str | None = None


def validate_job(
    out: Path,
    job: TileKey,
    cells: Mapping[tuple[int, int], str],
    hgt: HgtCells,
    samples: int,
    alignment_tiles: int,
    seed: int,
) -> JobValidation:
    """Every check of one completed job. A check that fails (e.g. network) is recorded as a
    problem instead of ending the run; a failed Tilezen check is only reported."""
    result = JobValidation(job)
    try:
        ours = read_job_heights(out, job)
    except Exception as error:
        result.problems.append(f"{job}: tiles unreadable: {error}")
        return result
    # Seeded per job, so a job's samples do not depend on the worker count or job order.
    rng = np.random.default_rng((seed, job.z, job.x, job.y))
    try:
        result.source = check_source(job, ours, cells, samples, rng)
        result.problems += source_problems(result.source)
    except Exception as error:
        result.problems.append(f"{job}: source check failed: {error}")
    try:
        result.reference = check_ardupilot(job, ours, hgt)
        result.reference_checked = True
        if result.reference is not None:
            result.problems += reference_problems(result.reference)
    except Exception as error:
        result.problems.append(f"{job}: ArduPilot check failed: {error}")
    try:
        result.alignments = check_tilezen(job, ours, alignment_tiles)
    except Exception as error:
        result.tilezen_error = str(error)
    return result


def validate_jobs(
    out: Path,
    jobs: Sequence[TileKey],
    cells: Mapping[tuple[int, int], str],
    hgt: HgtCells,
    *,
    samples: int,
    alignment_tiles: int,
    seed: int,
    workers: int,
    executor_factory: Callable[..., Executor] = ProcessPoolExecutor,
) -> Iterator[JobValidation]:
    """Validate *jobs* in a pool, yielding each result as its job finishes.

    Only result rows are kept, never a decoded job. A job whose worker fails, and every job
    left when the pool breaks, yields a result holding only that problem.
    """
    finished: set[TileKey] = set()
    with executor_factory(max_workers=workers) as pool:
        results = bounded_completion(
            lambda job: pool.submit(
                validate_job, out, job, cells, hgt, samples, alignment_tiles, seed
            ),
            jobs,
            limit=workers * IN_FLIGHT_PER_WORKER,
        )
        try:
            for job, future in results:
                finished.add(job)
                try:
                    result = future.result()
                except Exception as error:
                    result = JobValidation(job, [f"{job}: validation failed: {error}"])
                yield result
        except BrokenProcessPool as error:
            for job in jobs:
                if job not in finished:
                    yield JobValidation(job, [f"{job}: not validated, worker pool broke ({error})"])
