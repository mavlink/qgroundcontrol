"""Whole-build check run before upload: every tile's structure, the seams between tiles, and
gross height jumps.

``validate`` measures accuracy on a sample of jobs; this reads every tile, so a fault in a job
the sample missed (a misplaced or stale tile, a seam between independently generated jobs, an
unreadable file) cannot reach an upload. Work is one job (or overview tile) per task; a job
also reads the edge tiles of its east and south neighbors so seams between jobs are covered
without holding the whole world in memory.
"""

from __future__ import annotations

import io
import math
import time
from concurrent.futures import ProcessPoolExecutor
from typing import TYPE_CHECKING

import numpy as np
from PIL import Image

from . import terrarium
from .generate import tile_path
from .heights import HEIGHT_STEP, SEA_LEVEL
from .jobs import (
    IN_FLIGHT_PER_WORKER,
    bounded_completion,
    check_passed_marker,
    done_markers,
)
from .tilemath import (
    JOB_ZOOM,
    MAX_ZOOM,
    MERCATOR_HALF_EXTENT,
    TILE_SIZE,
    TileKey,
    children,
    lonlat_bounds,
)

if TYPE_CHECKING:
    from collections.abc import Callable
    from concurrent.futures import Executor
    from pathlib import Path

# Dead Sea shore is about -430 m, Everest 8849 m.
MIN_HEIGHT_M = -500.0
MAX_HEIGHT_M = 9000.0
# Quantized heights leave only multiples of this in the blue channel (64 for 0.25 m steps).
_STEP_CODE = round(HEIGHT_STEP * 256)
# A boundary is a seam when its mean jump exceeds this many times the mean step beside it,
# plus a tolerance for quantization. Genuine terrain is as steep across a boundary as beside
# it; a misplaced, stale or zeroed tile is not.
SEAM_RATIO = 3.0
SEAM_TOLERANCE_M = 0.5
# Neighboring pixels further apart than this are an artifact, as in ArduPilot's find_steep.py.
# Coarse zooms average large areas, so the limit grows with the pixel's ground size.
STEEP_JUMP_M = 1000.0
STEEP_MAX_SLOPE = 2.0


def decode_tile(data: bytes) -> tuple[np.ndarray | None, list[str]]:
    """Heights of one tile and its structural problems; heights is None if it can't be read."""
    try:
        with Image.open(io.BytesIO(data)) as image:
            if image.format != "PNG":
                return None, [f"not a readable PNG ({image.format})"]
            if image.size != (TILE_SIZE, TILE_SIZE) or image.mode != "RGB":
                return None, [
                    f"is {image.size[0]}x{image.size[1]} {image.mode}, "
                    f"not {TILE_SIZE}x{TILE_SIZE} RGB"
                ]
            rgb = np.asarray(image)
    except OSError as error:
        return None, [f"not a readable PNG: {error}"]
    problems = []
    if np.any(rgb[..., 2] % _STEP_CODE):
        problems.append(f"heights not on {HEIGHT_STEP} m steps")
    heights = terrarium.decode(rgb)
    low, high = float(heights.min()), float(heights.max())
    if low < MIN_HEIGHT_M or high > MAX_HEIGHT_M:
        problems.append(f"heights {low:.1f}..{high:.1f} m outside {MIN_HEIGHT_M}..{MAX_HEIGHT_M}")
    if low == high == SEA_LEVEL and data != terrarium.sea_png_bytes():
        problems.append("all-sea tile is not the shared sea PNG")
    return heights, problems


def seam_is_broken(
    second_last: np.ndarray, last: np.ndarray, first: np.ndarray, second: np.ndarray
) -> bool:
    """Whether the boundary between pixel lines *last* and *first* is a seam.

    *second_last*/*last* are the two lines before the boundary, *first*/*second* the two
    after it.
    """
    jump = float(np.abs(first - last).mean())
    beside = float((np.abs(last - second_last).mean() + np.abs(second - first).mean()) / 2)
    return jump > SEAM_RATIO * beside + SEAM_TOLERANCE_M


def steep_limit_m(key: TileKey) -> float:
    _, south, _, north = lonlat_bounds(key)
    pixel_m = (
        2
        * MERCATOR_HALF_EXTENT
        / (1 << key.z)
        / TILE_SIZE
        * math.cos(math.radians((south + north) / 2))
    )
    return max(STEEP_JUMP_M, STEEP_MAX_SLOPE * pixel_m)


def _max_jump(heights: np.ndarray) -> float:
    return float(
        max(np.abs(np.diff(heights, axis=0)).max(), np.abs(np.diff(heights, axis=1)).max())
    )


def _read_checked(
    out: Path, key: TileKey, problems: list[str], *, check_jumps: bool = True
) -> np.ndarray | None:
    path = tile_path(out, key)
    if not path.is_file():
        problems.append(f"{key}: missing")
        return None
    heights, tile_problems = decode_tile(path.read_bytes())
    problems.extend(f"{key}: {problem}" for problem in tile_problems)
    if heights is not None and check_jumps:
        jump = _max_jump(heights)
        if jump > steep_limit_m(key):
            problems.append(f"{key}: {jump:.0f} m jump between neighboring pixels")
    return heights


def _read_neighbor(out: Path, key: TileKey) -> np.ndarray | None:
    """A neighbor's heights for seam checks; its own problems are reported by its own task."""
    path = tile_path(out, key)
    if not path.is_file():
        return None
    heights, _ = decode_tile(path.read_bytes())
    return heights


def _boundary_jump_problem(
    key: TileKey, last: np.ndarray, first: np.ndarray, side: str
) -> list[str]:
    jump = float(np.abs(first - last).max())
    if jump > steep_limit_m(key):
        return [f"{key}: {jump:.0f} m jump to its {side} neighbor"]
    return []


def _check_seams(
    key: TileKey, heights: np.ndarray, east: np.ndarray | None, south: np.ndarray | None
) -> list[str]:
    problems = []
    if east is not None:
        if seam_is_broken(heights[:, -2], heights[:, -1], east[:, 0], east[:, 1]):
            problems.append(f"{key}: seam with its east neighbor")
        problems += _boundary_jump_problem(key, heights[:, -1], east[:, 0], "east")
    if south is not None:
        if seam_is_broken(heights[-2, :], heights[-1, :], south[0, :], south[1, :]):
            problems.append(f"{key}: seam with its south neighbor")
        problems += _boundary_jump_problem(key, heights[-1, :], south[0, :], "south")
    return problems


def _east(key: TileKey) -> TileKey:
    # The antimeridian is continuous terrain (Fiji, Chukotka), so the world wraps east-west.
    return TileKey(key.z, (key.x + 1) % (1 << key.z), key.y)


def _south(key: TileKey) -> TileKey | None:
    return TileKey(key.z, key.x, key.y + 1) if key.y + 1 < 1 << key.z else None


def check_job(out: Path, job: TileKey) -> list[str]:
    """Every tile of a completed job, the seams inside it, and its seams with the jobs east and
    south of it."""
    problems: list[str] = []
    for z in range(JOB_ZOOM, MAX_ZOOM + 1):
        depth = z - JOB_ZOOM
        n = 1 << depth
        tiles = {
            (tx, ty): _read_checked(
                out, TileKey(z, (job.x << depth) + tx, (job.y << depth) + ty), problems
            )
            for ty in range(n)
            for tx in range(n)
        }
        for (tx, ty), heights in tiles.items():
            if heights is None:
                continue
            key = TileKey(z, (job.x << depth) + tx, (job.y << depth) + ty)
            east = tiles.get((tx + 1, ty)) if tx + 1 < n else _read_neighbor(out, _east(key))
            south_key = _south(key)
            south = (
                tiles.get((tx, ty + 1))
                if ty + 1 < n
                else (_read_neighbor(out, south_key) if south_key else None)
            )
            problems.extend(_check_seams(key, heights, east, south))
    return problems


def _is_fully_built(out: Path, key: TileKey) -> bool:
    """Whether every tile below *key* down to JOB_ZOOM exists, so none of it is sea fill."""
    if key.z >= JOB_ZOOM:
        return True
    return all(
        tile_path(out, child).is_file() and _is_fully_built(out, child) for child in children(key)
    )


def check_overview_tile(out: Path, key: TileKey) -> list[str]:
    """Structure always; jumps and seams only where every job below it exists.

    Overviews fill missing descendants with sea level, which is intended in a partial build. In
    a full build every descendant exists, and a missing one is reported by its job or as a
    missing parent.
    """
    problems: list[str] = []
    complete = _is_fully_built(out, key)
    heights = _read_checked(out, key, problems, check_jumps=complete)
    if heights is not None and complete:
        east_key, south_key = _east(key), _south(key)
        east = _read_neighbor(out, east_key) if _is_fully_built(out, east_key) else None
        south = (
            _read_neighbor(out, south_key)
            if south_key is not None and _is_fully_built(out, south_key)
            else None
        )
        problems.extend(_check_seams(key, heights, east, south))
    return problems


def _tile_keys(out: Path, z: int) -> set[TileKey]:
    return {
        TileKey(z, int(path.parent.name), int(path.stem))
        for path in (out / "terrarium" / str(z)).glob("*/*.png")
    }


def missing_parents(out: Path) -> list[str]:
    """Overview tiles that should exist above existing tiles but don't."""
    problems = []
    below = _tile_keys(out, JOB_ZOOM)
    for z in range(JOB_ZOOM - 1, -1, -1):
        existing = _tile_keys(out, z)
        for key in sorted({TileKey(z, child.x >> 1, child.y >> 1) for child in below} - existing):
            problems.append(f"{key}: missing parent of existing tiles; rerun overviews")
        below = existing
    return problems


def _check_task(out: Path, kind: str, key: TileKey) -> list[str]:
    return check_job(out, key) if kind == "job" else check_overview_tile(out, key)


def check_build(
    out: Path,
    workers: int,
    executor_factory: Callable[..., Executor] = ProcessPoolExecutor,
) -> list[str]:
    """Check every completed job and every overview tile; returns the problems found.

    Marks the build as checked only when there are none. Tiles of unfinished jobs are skipped.
    """
    marker = check_passed_marker(out)
    marker.unlink(missing_ok=True)
    problems = missing_parents(out)
    tasks = [
        ("job", TileKey(*(int(part) for part in path.name.split("_"))))
        for path in sorted(done_markers(out))
    ]
    tasks += [("overview", key) for z in range(JOB_ZOOM) for key in sorted(_tile_keys(out, z))]
    start = time.monotonic()
    with executor_factory(max_workers=workers) as pool:
        results = bounded_completion(
            lambda task: pool.submit(_check_task, out, *task),
            tasks,
            limit=workers * IN_FLIGHT_PER_WORKER,
        )
        for done, ((kind, key), future) in enumerate(results, start=1):
            try:
                problems.extend(future.result())
            except Exception as error:
                problems.append(f"{kind} {key}: check failed: {error}")
            if done % 1000 == 0 or done == len(tasks):
                elapsed = time.monotonic() - start
                print(
                    f"[{done}/{len(tasks)} {elapsed:7.1f}s] {len(problems)} problem(s)", flush=True
                )
    if not problems:
        marker.parent.mkdir(parents=True, exist_ok=True)
        marker.write_text(f"{len(tasks)} tasks checked\n", encoding="utf-8")
    return problems
