"""Compare two local builds tile by tile: what a regeneration changed, and by how much.

Use it before publishing a fix to confirm the new build only changes what the fix should; its
list of changed and added tiles is what an upload of only the changes would send.
"""

from __future__ import annotations

import heapq
import math
import os
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass, field
from typing import TYPE_CHECKING

import numpy as np

from . import terrarium
from .jobs import IN_FLIGHT_PER_WORKER, bounded_completion
from .tilemath import MAX_ZOOM, TileKey

if TYPE_CHECKING:
    from collections.abc import Callable
    from concurrent.futures import Executor
    from pathlib import Path
    from typing import TextIO


@dataclass(frozen=True, order=True)
class TileChange:
    # Infinite for a tile that could not be read.
    max_change_m: float
    key: TileKey
    changed_pixels: int


@dataclass
class ZoomDiff:
    zoom: int
    identical: int = 0
    changed: int = 0
    added: int = 0
    removed: int = 0
    unreadable: int = 0
    max_change_m: float = 0.0


@dataclass
class _ColumnDiff:
    zoom: int
    identical: int = 0
    added: list[str] = field(default_factory=list)
    removed: int = 0
    unreadable: int = 0
    changes: list[TileChange] = field(default_factory=list)


@dataclass
class BuildDiff:
    zooms: list[ZoomDiff]
    top_changes: list[TileChange]
    # Changed and added tile paths written to the list file; removed tiles are not listed
    # because an upload does not delete them.
    listed: int


def _names(directory: Path) -> set[str]:
    try:
        return {name for name in os.listdir(directory) if name.endswith(".png")}
    except FileNotFoundError:
        return set()


def _compare_column(new: Path, old: Path, z: int, x: int) -> _ColumnDiff:
    new_dir = new / "terrarium" / str(z) / str(x)
    old_dir = old / "terrarium" / str(z) / str(x)
    new_names, old_names = _names(new_dir), _names(old_dir)
    result = _ColumnDiff(z, removed=len(old_names - new_names))
    result.added = [f"{z}/{x}/{name}" for name in sorted(new_names - old_names)]
    for name in sorted(new_names & old_names):
        key = TileKey(z, x, int(name.removesuffix(".png")))
        try:
            new_bytes = (new_dir / name).read_bytes()
            old_bytes = (old_dir / name).read_bytes()
            if new_bytes == old_bytes:
                result.identical += 1
                continue
            difference = np.abs(
                terrarium.heights_from_bytes(new_bytes) - terrarium.heights_from_bytes(old_bytes)
            )
        except (OSError, ValueError):
            # Unreadable or the wrong size; `check` reports why.
            result.unreadable += 1
            result.changes.append(TileChange(math.inf, key, 0))
            continue
        result.changes.append(
            TileChange(float(difference.max()), key, int(np.count_nonzero(difference)))
        )
    return result


def _columns(new: Path, old: Path) -> list[tuple[int, int]]:
    columns = set()
    for root in (new, old):
        for z in range(MAX_ZOOM + 1):
            zoom_dir = root / "terrarium" / str(z)
            if zoom_dir.is_dir():
                columns.update((z, int(name)) for name in os.listdir(zoom_dir) if name.isdigit())
    return sorted(columns)


def compare_builds(
    new: Path,
    old: Path,
    workers: int,
    top: int,
    list_file: TextIO | None = None,
    executor_factory: Callable[..., Executor] = ProcessPoolExecutor,
) -> BuildDiff:
    """Compare *new* with *old*; changed and added tile paths are written to *list_file* as
    each column finishes, so a full-build diff never holds them all in memory."""
    zooms: dict[int, ZoomDiff] = {}
    top_changes: list[TileChange] = []
    listed = 0
    with executor_factory(max_workers=workers) as pool:
        results = bounded_completion(
            lambda column: pool.submit(_compare_column, new, old, *column),
            _columns(new, old),
            limit=workers * IN_FLIGHT_PER_WORKER,
        )
        for _, future in results:
            column = future.result()
            zoom = zooms.setdefault(column.zoom, ZoomDiff(column.zoom))
            zoom.identical += column.identical
            zoom.changed += len(column.changes)
            zoom.added += len(column.added)
            zoom.removed += column.removed
            zoom.unreadable += column.unreadable
            for change in column.changes:
                if math.isfinite(change.max_change_m):
                    zoom.max_change_m = max(zoom.max_change_m, change.max_change_m)
            if list_file is not None:
                paths = [f"{c.key.z}/{c.key.x}/{c.key.y}.png" for c in column.changes]
                paths += column.added
                list_file.writelines(f"{path}\n" for path in paths)
                listed += len(paths)
            top_changes = heapq.nlargest(top, [*top_changes, *column.changes])
    return BuildDiff([zooms[z] for z in sorted(zooms)], top_changes, listed)
