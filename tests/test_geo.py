"""Tests for AEQD projection, shoelace area, and geodesic calculations in qgc4qgis.core.geo."""

import pytest

from qgc4qgis.core.geo import (
    AEQDProjection,
    geo_to_plane,
    geodesic_azimuth,
    geodesic_destination,
    geodesic_distance,
    plane_to_geo,
    polygon_area_geo,
    shoelace_area,
)


def test_aeqd_projection_origin():
    """Verify that reference origin projects to (0.0, 0.0) and roundtrips accurately."""
    ref_lat, ref_lon = -23.5505, -46.6333
    proj = AEQDProjection(ref_lat, ref_lon)

    x, y = proj.forward(ref_lat, ref_lon)
    assert pytest.approx(x, abs=1e-6) == 0.0
    assert pytest.approx(y, abs=1e-6) == 0.0

    lat, lon = proj.inverse(0.0, 0.0)
    assert pytest.approx(lat, abs=1e-7) == ref_lat
    assert pytest.approx(lon, abs=1e-7) == ref_lon


def test_geo_to_plane_and_plane_to_geo_roundtrip():
    """Verify forward and inverse transformations for offset coordinates."""
    ref_lat, ref_lon = -23.5505, -46.6333
    target_lat, target_lon = -23.5405, -46.6233

    x, y = geo_to_plane(target_lat, target_lon, ref_lat, ref_lon)
    assert x > 0.0  # East
    assert y > 0.0  # North

    lat_inv, lon_inv = plane_to_geo(x, y, ref_lat, ref_lon)
    assert pytest.approx(lat_inv, abs=1e-7) == target_lat
    assert pytest.approx(lon_inv, abs=1e-7) == target_lon


def test_east_north_coordinate_directions():
    """Verify X corresponds to East (+lon) and Y corresponds to North (+lat)."""
    ref_lat, ref_lon = 0.0, 0.0

    # Point directly East
    x_east, y_east = geo_to_plane(0.0, 0.01, ref_lat, ref_lon)
    assert x_east > 0.0
    assert pytest.approx(y_east, abs=1.0) == 0.0

    # Point directly North
    x_north, y_north = geo_to_plane(0.01, 0.0, ref_lat, ref_lon)
    assert pytest.approx(x_north, abs=1.0) == 0.0
    assert y_north > 0.0


def test_shoelace_area_known_shapes():
    """Verify shoelace area formula for rectangle, triangle, open vs closed list, and guards."""
    # Rectangle 10m x 20m = 200 m^2
    rect = [(0.0, 0.0), (10.0, 0.0), (10.0, 20.0), (0.0, 20.0)]
    assert pytest.approx(shoelace_area(rect)) == 200.0

    # Explicitly closed rectangle
    rect_closed = [*rect, (0.0, 0.0)]
    assert pytest.approx(shoelace_area(rect_closed)) == 200.0

    # Triangle base 10, height 10 = 50 m^2
    tri = [(0.0, 0.0), (10.0, 0.0), (5.0, 10.0)]
    assert pytest.approx(shoelace_area(tri)) == 50.0

    # Degenerate polygons (< 3 points)
    assert shoelace_area([]) == 0.0
    assert shoelace_area([(0.0, 0.0)]) == 0.0
    assert shoelace_area([(0.0, 0.0), (10.0, 0.0)]) == 0.0


def test_polygon_area_geo_vertex0_d5():
    """Verify polygon_area_geo projects vertices relative to vertex 0 (D5 origin)."""
    ref_lat, ref_lon = -23.5505, -46.6333

    # Generate a square approx 100m x 100m using geodesic destination
    p0 = (ref_lat, ref_lon)
    p1 = geodesic_destination(p0[0], p0[1], 90.0, 100.0)  # 100m East
    p2 = geodesic_destination(p1[0], p1[1], 0.0, 100.0)  # 100m North
    p3 = geodesic_destination(p0[0], p0[1], 0.0, 100.0)  # 100m North from origin

    coords = [p0, p1, p2, p3]
    area = polygon_area_geo(coords)

    # 100m x 100m = 10,000 m^2 (within 0.1% tolerance due to ellipsoid vs plane)
    assert pytest.approx(area, rel=1e-3) == 10000.0

    # Degenerate geodetic polygon (< 3 points)
    assert polygon_area_geo([p0, p1]) == 0.0


def test_geodesic_distance_and_azimuth():
    """Verify geodesic distance and azimuth calculations."""
    lat1, lon1 = -23.5505, -46.6333

    # Same point guard
    assert geodesic_distance(lat1, lon1, lat1, lon1) == 0.0
    assert geodesic_azimuth(lat1, lon1, lat1, lon1) == 0.0

    # Point 1000 meters Due East (azimuth 90 deg)
    lat2, lon2 = geodesic_destination(lat1, lon1, 90.0, 1000.0)

    dist = geodesic_distance(lat1, lon1, lat2, lon2)
    assert pytest.approx(dist, rel=1e-6) == 1000.0

    az = geodesic_azimuth(lat1, lon1, lat2, lon2)
    assert pytest.approx(az, abs=1e-4) == 90.0

    # Point 1000 meters Due North (azimuth 0 deg)
    lat3, lon3 = geodesic_destination(lat1, lon1, 0.0, 1000.0)
    az_north = geodesic_azimuth(lat1, lon1, lat3, lon3)
    assert pytest.approx(az_north, abs=1e-4) == 0.0


def test_geodesic_destination_negative_distance_guard():
    """Verify geodesic_destination returns original point when distance <= 0."""
    lat, lon = -23.5505, -46.6333
    assert geodesic_destination(lat, lon, 90.0, 0.0) == (lat, lon)
    assert geodesic_destination(lat, lon, 90.0, -100.0) == (lat, lon)
