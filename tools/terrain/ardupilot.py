"""ArduPilot terrain server SRTM1 cells (JAXA AW3D30 repackaged as .hgt), an independent reference.

The reader is plain numpy with the SRTM pixel-is-point convention, deliberately sharing no
georeferencing code with the generator. Neighboring cells duplicate their shared edge rows and
columns, so bilinear sampling never needs to cross cells.
"""

from __future__ import annotations

import io
import math
import urllib.error
import zipfile
from collections import OrderedDict
from typing import TYPE_CHECKING

import numpy as np

from .fetch import fetch_bytes

if TYPE_CHECKING:
    from pathlib import Path

SRTM1_URL = "https://terrain.ardupilot.org/SRTM1/{name}.hgt.zip"
HGT_VOID = -32768
# ~52 MB per decoded 1 arc-second cell; a job needs at most a few at once.
MAX_CACHED_CELLS = 8
_CHUNK_ROWS = 512


def hgt_name(lat: int, lon: int) -> str:
    ns = "N" if lat >= 0 else "S"
    ew = "E" if lon >= 0 else "W"
    return f"{ns}{abs(lat):02d}{ew}{abs(lon):03d}"


def parse_hgt(data: bytes) -> np.ndarray:
    samples = len(data) // 2
    side = math.isqrt(samples)
    if side * side != samples or side < 2:
        raise ValueError(f"Not a square .hgt grid: {len(data)} bytes")
    heights = np.frombuffer(data, dtype=">i2").reshape(side, side).astype(np.float32)
    heights[heights == HGT_VOID] = np.nan
    return heights


class HgtCells:
    """Downloads, caches and samples ArduPilot SRTM1 cells; cells it doesn't serve are None.

    Archives are cached on disk; decoded grids in a small LRU.
    """

    def __init__(self, cache_dir: Path) -> None:
        self._cache_dir = cache_dir
        self._cells: OrderedDict[tuple[int, int], np.ndarray | None] = OrderedDict()

    def _download(self, name: str) -> bytes | None:
        path = self._cache_dir / f"{name}.hgt.zip"
        missing = self._cache_dir / f"{name}.missing"
        if missing.exists():
            return None
        if not path.is_file():
            self._cache_dir.mkdir(parents=True, exist_ok=True)
            try:
                data = fetch_bytes(SRTM1_URL.format(name=name), timeout=120)
            except urllib.error.HTTPError as error:
                if error.code != 404:
                    raise
                missing.touch()
                return None
            staging = path.with_suffix(".tmp")
            staging.write_bytes(data)
            staging.replace(path)
        return path.read_bytes()

    def cell(self, lat: int, lon: int) -> np.ndarray | None:
        key = (lat, lon)
        if key in self._cells:
            self._cells.move_to_end(key)
        else:
            name = hgt_name(lat, lon)
            archive = self._download(name)
            if archive is None:
                self._cells[key] = None
            else:
                with zipfile.ZipFile(io.BytesIO(archive)) as bundle:
                    self._cells[key] = parse_hgt(bundle.read(f"{name}.hgt"))
            while len(self._cells) > MAX_CACHED_CELLS:
                self._cells.popitem(last=False)
        return self._cells[key]

    def sample_grid(self, lons: np.ndarray, lats: np.ndarray) -> np.ndarray:
        """Bilinear heights at every (lats[row], lons[col]); NaN where no cell or a void."""
        result = np.full((lats.size, lons.size), np.nan, dtype=np.float32)
        lat_cells = np.floor(lats).astype(int)
        lon_cells = np.floor(lons).astype(int)
        for lat in np.unique(lat_cells):
            rows = np.flatnonzero(lat_cells == lat)
            for lon in np.unique(lon_cells):
                grid = self.cell(int(lat), int(lon))
                if grid is None:
                    continue
                cols = np.flatnonzero(lon_cells == lon)
                spacing = grid.shape[0] - 1
                col_f = (lons[cols] - lon) * spacing
                c0 = np.clip(np.floor(col_f).astype(int), 0, spacing - 1)
                fx = (col_f - c0).astype(np.float32)[None, :]
                # Row chunks keep the float32 temporaries to a few MB each.
                for start in range(0, rows.size, _CHUNK_ROWS):
                    chunk = rows[start : start + _CHUNK_ROWS]
                    row_f = (lat + 1 - lats[chunk]) * spacing
                    r0 = np.clip(np.floor(row_f).astype(int), 0, spacing - 1)
                    fy = (row_f - r0).astype(np.float32)[:, None]
                    top_left = grid[np.ix_(r0, c0)]
                    bottom_left = grid[np.ix_(r0 + 1, c0)]
                    top = top_left + (grid[np.ix_(r0, c0 + 1)] - top_left) * fx
                    bottom = bottom_left + (grid[np.ix_(r0 + 1, c0 + 1)] - bottom_left) * fx
                    result[np.ix_(chunk, cols)] = top + (bottom - top) * fy
        return result
