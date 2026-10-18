"""Web Mercator (EPSG:3857) slippy-tile math for the terrarium pyramid."""

from __future__ import annotations

import math
from typing import NamedTuple

TILE_SIZE = 256
MAX_ZOOM = 13
JOB_ZOOM = 9
MERCATOR_HALF_EXTENT = 20037508.342789244
MAX_LATITUDE = 85.0511287798066


class TileKey(NamedTuple):
    z: int
    x: int
    y: int

    def __str__(self) -> str:
        return f"{self.z}/{self.x}/{self.y}"


def mercator_bounds(key: TileKey) -> tuple[float, float, float, float]:
    """Return (left, bottom, right, top) in EPSG:3857 meters."""
    span = 2 * MERCATOR_HALF_EXTENT / (1 << key.z)
    left = -MERCATOR_HALF_EXTENT + key.x * span
    top = MERCATOR_HALF_EXTENT - key.y * span
    return left, top - span, left + span, top


def mercator_to_lonlat(mx: float, my: float) -> tuple[float, float]:
    lon = mx / MERCATOR_HALF_EXTENT * 180.0
    lat = math.degrees(math.atan(math.sinh(my / MERCATOR_HALF_EXTENT * math.pi)))
    return lon, lat


def lonlat_to_mercator(lon: float, lat: float) -> tuple[float, float]:
    lat = max(-MAX_LATITUDE, min(MAX_LATITUDE, lat))
    mx = lon / 180.0 * MERCATOR_HALF_EXTENT
    my = math.asinh(math.tan(math.radians(lat))) / math.pi * MERCATOR_HALF_EXTENT
    return mx, my


def lonlat_bounds(key: TileKey) -> tuple[float, float, float, float]:
    """Return (west, south, east, north) in degrees."""
    left, bottom, right, top = mercator_bounds(key)
    west, south = mercator_to_lonlat(left, bottom)
    east, north = mercator_to_lonlat(right, top)
    return west, south, east, north


def tile_for_lonlat(lon: float, lat: float, z: int) -> TileKey:
    n = 1 << z
    mx, my = lonlat_to_mercator(lon, lat)
    span = 2 * MERCATOR_HALF_EXTENT / n
    x = math.floor((mx + MERCATOR_HALF_EXTENT) / span)
    y = math.floor((MERCATOR_HALF_EXTENT - my) / span)
    return TileKey(z, min(max(x, 0), n - 1), min(max(y, 0), n - 1))


def tiles_in_bbox(west: float, south: float, east: float, north: float, z: int) -> list[TileKey]:
    """All tiles at zoom *z* intersecting the lon/lat box (no antimeridian wrap)."""
    if west > east or south > north:
        raise ValueError(f"Invalid bbox: {(west, south, east, north)}")
    nw = tile_for_lonlat(west, north, z)
    se = tile_for_lonlat(east, south, z)
    return [TileKey(z, x, y) for y in range(nw.y, se.y + 1) for x in range(nw.x, se.x + 1)]


def all_tiles(z: int) -> list[TileKey]:
    n = 1 << z
    return [TileKey(z, x, y) for y in range(n) for x in range(n)]


def parent(key: TileKey) -> TileKey:
    if key.z == 0:
        raise ValueError("Zoom 0 tile has no parent")
    return TileKey(key.z - 1, key.x >> 1, key.y >> 1)


def children(key: TileKey) -> list[TileKey]:
    """The four children in row-major order: NW, NE, SW, SE."""
    x, y, z = key.x * 2, key.y * 2, key.z + 1
    return [TileKey(z, x, y), TileKey(z, x + 1, y), TileKey(z, x, y + 1), TileKey(z, x + 1, y + 1)]
