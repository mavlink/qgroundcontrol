"""Whole-build check run before upload: every tile's structure, the seams between tiles,
gross height jumps, and land written as sea level.

``validate`` measures accuracy on a sample of jobs; this reads every tile, so a fault in a job
the sample missed (a misplaced or stale tile, a seam between independently generated jobs, an
unreadable file) cannot reach an upload. Work is one job (or overview tile) per task; a job
also reads the edge tiles of its east and south neighbors so seams between jobs are covered
without holding the whole world in memory.
"""

from __future__ import annotations

import io
import math
import re
import time
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures.process import BrokenProcessPool
from typing import TYPE_CHECKING, NamedTuple

import numpy as np
from PIL import Image

from . import terrarium
from .heights import HEIGHT_STEP, SEA_LEVEL
from .jobs import (
    IN_FLIGHT_PER_WORKER,
    TILES_DIR,
    bounded_completion,
    check_passed_marker,
    done_markers,
    job_from_marker,
    tile_path,
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
    from collections.abc import Callable, Iterable, Mapping
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
# A few sea-level pixels enclosed by land this high are land the generator gave no height, as
# at the corners where one GLO-90 cell meets three GLO-30 cells, not sea. The jump limit only
# sees them above STEEP_JUMP_M.
HOLE_MAX_PIXELS = 4
HOLE_MIN_LAND_M = 25.0
ALL_JOBS = 1 << (2 * JOB_ZOOM)

_ACCEPTED_LINE = re.compile(
    r"(\d+/\d+/\d+) (?:(jump(?: (?:east|south))?) (\d+) (\d+)"
    r"|(hole) (\d+)|(seam (?:east|south)) (\d+(?:\.\d+)?))"
)
_FINDING = re.compile(
    r"(?P<tile>\d+/\d+/\d+): (?:"
    r"(?P<jump>\d+) m jump (?:between neighboring pixels|to its (?P<across>east|south) neighbor)"
    r", (?P<pairs>\d+) pair\(s\) over the limit"
    r"|(?P<seam>\d+\.\d) m seam with its (?P<side>east|south) neighbor"
    r"|(?P<hole>\d+) sea-level pixel\(s\) enclosed by land)"
)


class Limit(NamedTuple):
    """The largest size a finding is accepted at and, for jumps, the most pixel pairs."""

    size: float
    pairs: int = 1


def load_accepted(path: Path) -> dict[tuple[str, str], Limit]:
    """Accepted findings by ``(tile, finding)``, from lines such as ``13/6033/3419 jump 1016 2``.

    The finding is ``jump`` (inside the tile), ``jump east`` or ``jump south`` (to that
    neighbor), ``hole``, ``seam east`` or ``seam south``, each with the largest size it is
    accepted at: meters between pixels, pixels, or the mean step in meters across the tile's
    edge. A jump also gives the most pairs of pixels that may be over the limit, so that a new
    one beside a reviewed one is not accepted with it. Blank lines and ``#`` comments are
    skipped.
    """
    accepted: dict[tuple[str, str], Limit] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        entry = " ".join(line.split("#", 1)[0].split())
        if not entry:
            continue
        match = _ACCEPTED_LINE.fullmatch(entry)
        if match is None:
            raise ValueError(f"{path}:{number}: not an accepted finding: {line.strip()!r}")
        tile, jump, meters, pairs, hole, pixels, seam, step = match.groups()
        accepted[tile, jump or hole or seam] = (
            Limit(float(meters), int(pairs)) if jump else Limit(float(pixels or step))
        )
    return accepted


def accepted_entry(
    problem: str, accepted: Mapping[tuple[str, str], Limit]
) -> tuple[str, str] | None:
    """The accepted finding *problem* is, no larger than it was accepted at, or None.

    Anything else about an accepted tile is still a problem.
    """
    match = _FINDING.fullmatch(problem)
    if match is None:
        return None
    if match["jump"]:
        finding = f"jump {match['across']}" if match["across"] else "jump"
    else:
        finding = "hole" if match["hole"] else f"seam {match['side']}"
    entry = match["tile"], finding
    limit = accepted.get(entry)
    if limit is None:
        return None
    size = float(match["jump"] or match["hole"] or match["seam"])
    return entry if size <= limit.size and int(match["pairs"] or 1) <= limit.pairs else None


def is_accepted(problem: str, accepted: Mapping[tuple[str, str], Limit]) -> bool:
    return accepted_entry(problem, accepted) is not None


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


def seam_step_m(
    second_last: np.ndarray, last: np.ndarray, first: np.ndarray, second: np.ndarray
) -> float | None:
    """The mean step across the boundary between pixel lines *last* and *first*, if a seam.

    *second_last*/*last* are the two lines before the boundary, *first*/*second* the two
    after it.
    """
    jump = float(np.abs(first - last).mean())
    beside = float((np.abs(last - second_last).mean() + np.abs(second - first).mean()) / 2)
    return jump if jump > SEAM_RATIO * beside + SEAM_TOLERANCE_M else None


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


def hole_pixels(heights: np.ndarray) -> list[tuple[int, int]]:
    """The (row, col) of sea-level pixels in a group of at most HOLE_MAX_PIXELS enclosed by land.

    A pixel counts when its 5x5 neighborhood holds no more sea-level pixels than that and every
    other pixel touching it is at least HOLE_MIN_LAND_M high. Coasts, channels and tidal flats
    fail one or the other; the head of an inlet one pixel wide between banks that high would
    count, and the first global build had none. Pixels within two of the tile's edge are not
    judged: the neighborhood would need the next tile.
    """
    sea = heights == SEA_LEVEL
    if not sea.any() or sea.all():
        return []
    # Each window as a few shifted slices, a row pass then a column pass: several times faster
    # than reducing a sliding-window view, and coastal tiles are checked by the million.
    count = sea.astype(np.uint8)
    count = count[:, :-4] + count[:, 1:-3] + count[:, 2:-2] + count[:, 3:-1] + count[:, 4:]
    nearby_sea = count[:-4] + count[1:-3] + count[2:-2] + count[3:-1] + count[4:]
    land = np.where(sea, np.inf, heights)
    land = np.minimum(np.minimum(land[:, :-2], land[:, 1:-1]), land[:, 2:])
    lowest_land = np.minimum(np.minimum(land[:-2], land[1:-1]), land[2:])[1:-1, 1:-1]
    holes = sea[2:-2, 2:-2] & (nearby_sea <= HOLE_MAX_PIXELS) & (lowest_land >= HOLE_MIN_LAND_M)
    return [(int(row) + 2, int(col) + 2) for row, col in np.argwhere(holes)]


def sea_holes(heights: np.ndarray) -> int:
    return len(hole_pixels(heights))


class Finding(NamedTuple):
    """A jump, seam or hole of a tile, as its problem reads after the tile, and its pixels.

    The pixels are every one the finding was decided from: both of each pair of a jump, the
    two lines on each side of a seam, the 5x5 neighborhood of each pixel of a hole. They are
    (row, col) in the tile; indices from TILE_SIZE on are rows or columns of the tile south or
    east of it.
    """

    text: str
    pixels: list[tuple[int, int]]


def _jump_text(largest: float, where: str, pairs: int) -> str:
    return f"{largest:.0f} m jump {where}, {pairs} pair(s) over the limit"


def _jumps(key: TileKey, heights: np.ndarray) -> Finding | None:
    """Every pair of neighboring pixels of a tile further apart than the limit."""
    limit = steep_limit_m(key)
    down, across = np.abs(np.diff(heights, axis=0)), np.abs(np.diff(heights, axis=1))
    largest = float(max(down.max(), across.max()))
    if largest <= limit:
        return None
    pairs = [
        ((int(row), int(col)), (int(row) + below, int(col) + 1 - below))
        for jumps, below in ((down, 1), (across, 0))
        for row, col in np.argwhere(jumps > limit)
    ]
    pixels = sorted({pixel for pair in pairs for pixel in pair})
    return Finding(_jump_text(largest, "between neighboring pixels", len(pairs)), pixels)


def own_findings(key: TileKey, heights: np.ndarray, *, jumps: bool = True) -> list[Finding]:
    """The jumps and the holes inside one tile."""
    findings = []
    if jumps:
        jump = _jumps(key, heights)
        if jump is not None:
            findings.append(jump)
    # Only where the source is resampled: coarser zooms average these pixels away.
    if key.z == MAX_ZOOM:
        holes = hole_pixels(heights)
        if holes:
            reach = range(-2, 3)
            nearby = {
                (row + down, col + over) for row, col in holes for down in reach for over in reach
            }
            findings.append(
                Finding(f"{len(holes)} sea-level pixel(s) enclosed by land", sorted(nearby))
            )
    return findings


def edge_findings(
    key: TileKey, heights: np.ndarray, east: np.ndarray | None, south: np.ndarray | None
) -> list[Finding]:
    """The seams and jumps between a tile and the tiles east and south of it."""
    findings = []
    limit = steep_limit_m(key)
    last = TILE_SIZE - 1

    def both_sides(side: str, along: Iterable[int], lines: int) -> list[tuple[int, int]]:
        """The *lines* nearest pixels on each side of the edge, at the indices *along* it."""
        return [
            (index, last + beyond) if side == "east" else (last + beyond, index)
            for index in along
            for beyond in range(1 - lines, 1 + lines)
        ]

    for side, neighbor in (("east", east), ("south", south)):
        if neighbor is None:
            continue
        if side == "east":
            lines = heights[:, -2], heights[:, -1], neighbor[:, 0], neighbor[:, 1]
        else:
            lines = heights[-2, :], heights[-1, :], neighbor[0, :], neighbor[1, :]
        step = seam_step_m(*lines)
        if step is not None:
            findings.append(
                Finding(
                    f"{step:.1f} m seam with its {side} neighbor",
                    both_sides(side, range(TILE_SIZE), lines=2),
                )
            )
        jumps = np.abs(lines[2] - lines[1])
        if jumps.max() > limit:
            over = [int(index) for index in np.flatnonzero(jumps > limit)]
            findings.append(
                Finding(
                    _jump_text(float(jumps.max()), f"to its {side} neighbor", len(over)),
                    both_sides(side, over, lines=1),
                )
            )
    return findings


def _read_checked(
    out: Path, key: TileKey, problems: list[str], *, check_jumps: bool = True
) -> np.ndarray | None:
    path = tile_path(out, key)
    if not path.is_file():
        problems.append(f"{key}: missing")
        return None
    heights, tile_problems = decode_tile(path.read_bytes())
    problems.extend(f"{key}: {problem}" for problem in tile_problems)
    if heights is not None:
        findings = own_findings(key, heights, jumps=check_jumps)
        problems.extend(f"{key}: {finding.text}" for finding in findings)
    return heights


def _read_neighbor(out: Path, key: TileKey) -> np.ndarray | None:
    """A neighbor's heights for seam checks; its own problems are reported by its own task."""
    path = tile_path(out, key)
    if not path.is_file():
        return None
    heights, _ = decode_tile(path.read_bytes())
    return heights


def _check_seams(
    key: TileKey, heights: np.ndarray, east: np.ndarray | None, south: np.ndarray | None
) -> list[str]:
    return [f"{key}: {finding.text}" for finding in edge_findings(key, heights, east, south)]


def east_of(key: TileKey) -> TileKey:
    # The antimeridian is continuous terrain (Fiji, Chukotka), so the world wraps east-west.
    return TileKey(key.z, (key.x + 1) % (1 << key.z), key.y)


def south_of(key: TileKey) -> TileKey | None:
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
            east = tiles.get((tx + 1, ty)) if tx + 1 < n else _read_neighbor(out, east_of(key))
            south_key = south_of(key)
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
        east_key, south_key = east_of(key), south_of(key)
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
        for path in (out / TILES_DIR / str(z)).glob("*/*.png")
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
    accepted: Mapping[tuple[str, str], Limit] | None = None,
) -> list[str]:
    """Check every completed job and every overview tile; returns the problems found.

    Marks the build as checked only when there are none. Tiles of unfinished jobs are skipped.
    A task that fails, or a worker pool that breaks, is reported as a problem. Findings in
    *accepted* (see load_accepted) are printed, not returned; so are, for a whole build that
    passes, the accepted findings that no longer occur and can leave the list.
    """
    marker = check_passed_marker(out)
    marker.unlink(missing_ok=True)
    problems = missing_parents(out)
    tasks = [("job", job_from_marker(path)) for path in sorted(done_markers(out))]
    tasks += [("overview", key) for z in range(JOB_ZOOM) for key in sorted(_tile_keys(out, z))]
    start = time.monotonic()
    done = 0
    with executor_factory(max_workers=workers) as pool:
        results = bounded_completion(
            lambda task: pool.submit(_check_task, out, *task),
            tasks,
            limit=workers * IN_FLIGHT_PER_WORKER,
        )
        try:
            for (kind, key), future in results:
                done += 1
                try:
                    problems.extend(future.result())
                except Exception as error:
                    problems.append(f"{kind} {key}: check failed: {error}")
                if done % 1000 == 0 or done == len(tasks):
                    elapsed = time.monotonic() - start
                    print(
                        f"[{done}/{len(tasks)} {elapsed:7.1f}s] {len(problems)} finding(s)",
                        flush=True,
                    )
        except BrokenProcessPool as error:
            problems.append(f"Worker pool broke ({error}); {len(tasks) - done} task(s) not checked")
    accepted = accepted or {}
    known: list[str] = []
    unaccepted: list[str] = []
    occurred: set[tuple[str, str]] = set()
    for problem in problems:
        entry = accepted_entry(problem, accepted)
        if entry is None:
            unaccepted.append(problem)
        else:
            known.append(problem)
            occurred.add(entry)
    for problem in known:
        print(f"accepted: {problem}", flush=True)
    # Only a whole build that passed can tell. In a partial one most accepted tiles were never
    # checked; in a failed one a finding may be over its size, or its task may not have run.
    if not unaccepted and sum(kind == "job" for kind, _ in tasks) == ALL_JOBS:
        for tile, finding in sorted(accepted.keys() - occurred):
            print(f"accepted finding no longer occurs: {tile} {finding}", flush=True)
    if not unaccepted:
        marker.parent.mkdir(parents=True, exist_ok=True)
        marker.write_text(
            f"{len(tasks)} tasks checked, {len(known)} accepted finding(s)\n", encoding="utf-8"
        )
    return unaccepted
