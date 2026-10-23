"""Tile counts and sizes per zoom of a local build (no rasterio needed)."""

from __future__ import annotations

import statistics
from collections import defaultdict
from dataclasses import dataclass
from typing import TYPE_CHECKING

import numpy as np

from . import terrarium
from .jobs import TILES_DIR

if TYPE_CHECKING:
    from pathlib import Path


@dataclass
class SizeStats:
    zoom: int
    count: int
    total_bytes: int
    mean_bytes: float
    p95_bytes: float
    sea_count: int


def size_report(out: Path) -> list[SizeStats]:
    sizes: dict[int, list[int]] = defaultdict(list)
    sea_counts: dict[int, int] = defaultdict(int)
    sea_bytes = terrarium.sea_png_bytes()
    for path in (out / TILES_DIR).glob("*/*/*.png"):
        zoom = int(path.parent.parent.name)
        size = path.stat().st_size
        sizes[zoom].append(size)
        # A flat non-sea tile (e.g. a flattened lake) can compress to the same size, so
        # only same-size files are read and compared.
        if size == len(sea_bytes) and path.read_bytes() == sea_bytes:
            sea_counts[zoom] += 1
    return [
        SizeStats(
            zoom=zoom,
            count=len(values),
            total_bytes=sum(values),
            mean_bytes=statistics.fmean(values),
            p95_bytes=float(np.percentile(values, 95)),
            sea_count=sea_counts[zoom],
        )
        for zoom, values in sorted(sizes.items())
    ]
