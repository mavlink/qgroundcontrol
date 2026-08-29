"""Local AEQD projection utilities and geodesic calculations for QGC4QGIS.

Provides coordinate transformations between WGS84 geodetic coordinates (lat, lon)
and local Azimuthal Equidistant (AEQD) planar coordinates (x=East, y=North),
shoelace planar area calculation, and WGS84 geodesic distance and azimuth calculations.
"""

from collections.abc import Sequence

import pyproj

# Cached WGS84 Geod instance for geodesic calculations
_WGS84_GEOD = pyproj.Geod(ellps="WGS84")


class AEQDProjection:
    """Local Azimuthal Equidistant (AEQD) projection centered at a reference origin.

    Maps (lat, lon) geodetic coordinates on WGS84 to planar (x=East, y=North) in meters,
    with origin (0, 0) at (ref_lat, ref_lon).
    """

    def __init__(self, ref_lat: float, ref_lon: float) -> None:
        """Initialize local AEQD projection with origin at (ref_lat, ref_lon)."""
        self.ref_lat = float(ref_lat)
        self.ref_lon = float(ref_lon)
        proj_str = (
            f"+proj=aeqd +lat_0={self.ref_lat} +lon_0={self.ref_lon} +datum=WGS84 +units=m +no_defs"
        )
        self._to_plane = pyproj.Transformer.from_crs("EPSG:4326", proj_str, always_xy=True)
        self._to_geo = pyproj.Transformer.from_crs(proj_str, "EPSG:4326", always_xy=True)

    def forward(self, lat: float, lon: float) -> tuple[float, float]:
        """Convert geodetic coordinate (lat, lon) to local planar (x=East, y=North) in meters."""
        x, y = self._to_plane.transform(float(lon), float(lat))
        return float(x), float(y)

    def inverse(self, x: float, y: float) -> tuple[float, float]:
        """Convert local planar coordinate (x=East, y=North) in meters to geodetic (lat, lon)."""
        lon, lat = self._to_geo.transform(float(x), float(y))
        return float(lat), float(lon)


def geo_to_plane(lat: float, lon: float, ref_lat: float, ref_lon: float) -> tuple[float, float]:
    """Convert geodetic coordinate (lat, lon) to local AEQD planar (x=East, y=North) in meters.

    Reference origin (ref_lat, ref_lon) maps to (0.0, 0.0).
    """
    proj = AEQDProjection(ref_lat, ref_lon)
    return proj.forward(lat, lon)


def plane_to_geo(x: float, y: float, ref_lat: float, ref_lon: float) -> tuple[float, float]:
    """Convert local AEQD planar coordinate (x=East, y=North) in meters to geodetic (lat, lon).

    Planar coordinate (0.0, 0.0) maps to (ref_lat, ref_lon).
    """
    proj = AEQDProjection(ref_lat, ref_lon)
    return proj.inverse(x, y)


def shoelace_area(points: Sequence[tuple[float, float]]) -> float:
    """Calculate 2D planar polygon area using the Shoelace formula (Gauss's area formula).

    :param points: Sequence of (x, y) coordinate tuples in meters or planar units.
    :return: Polygon area in square units (non-negative float). Returns 0.0 if points < 3.
    """
    n = len(points)
    if n < 3:
        return 0.0

    pts = list(points)
    if pts[0] != pts[-1]:
        pts.append(pts[0])
        n += 1

    area_sum = 0.0
    for i in range(n - 1):
        x1, y1 = pts[i]
        x2, y2 = pts[i + 1]
        area_sum += x1 * y2 - x2 * y1

    return abs(area_sum) / 2.0


def polygon_area_geo(coords: Sequence[tuple[float, float]]) -> float:
    """Calculate planar area of a geodetic polygon [(lat, lon), ...].

    Uses vertex 0 (coords[0]) as the local AEQD reference origin (D5), projects
    all vertices to planar (x=East, y=North) meters, and computes the Shoelace area.

    :param coords: Sequence of (lat, lon) geodetic coordinate tuples.
    :return: Area in square meters (non-negative float).
    """
    if len(coords) < 3:
        return 0.0

    ref_lat, ref_lon = coords[0]
    proj = AEQDProjection(ref_lat, ref_lon)
    planar_points = [proj.forward(lat, lon) for lat, lon in coords]
    return shoelace_area(planar_points)


def geodesic_distance(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    """Calculate geodesic distance in meters between two points on the WGS84 ellipsoid."""
    if lat1 == lat2 and lon1 == lon2:
        return 0.0
    _, _, dist = _WGS84_GEOD.inv(lon1, lat1, lon2, lat2)
    return float(dist)


def geodesic_azimuth(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    """Calculate geodesic forward azimuth in degrees [0.0, 360.0) from point 1 to point 2."""
    if lat1 == lat2 and lon1 == lon2:
        return 0.0
    fwd_az, _, _ = _WGS84_GEOD.inv(lon1, lat1, lon2, lat2)
    return float(fwd_az % 360.0)


def geodesic_destination(
    lat: float, lon: float, azimuth: float, distance: float
) -> tuple[float, float]:
    """Calculate destination geodetic coordinate (lat, lon) from start point, azimuth (deg), and distance (m)."""
    if distance <= 0:
        return float(lat), float(lon)
    dest_lon, dest_lat, _ = _WGS84_GEOD.fwd(lon, lat, azimuth, distance)
    return float(dest_lat), float(dest_lon)
