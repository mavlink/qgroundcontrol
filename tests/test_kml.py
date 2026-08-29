"""Unit tests for Litchi KML exporter module."""

import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

from qgc4qgis.core.kml import (
    KML_NS,
    MIN_KML_ALTITUDE,
    route_to_litchi_kml,
    save_litchi_kml,
    validate_litchi_kml_route,
)
from qgc4qgis.core.route import Route, RouteWaypoint


def test_coordinates_count_order_and_components():
    """Test that coordinate count, order, and component count match route waypoints."""
    wp1 = RouteWaypoint(lat=-23.5505, lon=-46.6333, altura=50.0)
    wp2 = RouteWaypoint(lat=-23.5510, lon=-46.6340, altura=60.0)
    wp3 = RouteWaypoint(lat=-23.5515, lon=-46.6350, altura=70.0)
    route = Route(waypoints=[wp1, wp2, wp3])

    kml_str = route_to_litchi_kml(route)
    root = ET.fromstring(kml_str)

    coords_elem = root.find(f".//{{{KML_NS}}}coordinates")
    assert coords_elem is not None and coords_elem.text is not None

    tuples = coords_elem.text.strip().split()
    assert len(tuples) == 3

    for t, wp in zip(tuples, [wp1, wp2, wp3], strict=True):
        parts = t.split(",")
        assert len(parts) == 3
        lon_val, lat_val, alt_val = float(parts[0]), float(parts[1]), float(parts[2])
        assert pytest.approx(lon_val) == wp.lon
        assert pytest.approx(lat_val) == wp.lat
        assert pytest.approx(alt_val) == wp.altura


def test_lon_before_lat():
    """Test that longitude precedes latitude in KML coordinate tuples."""
    wp = RouteWaypoint(lat=-23.5, lon=-46.6, altura=50.0)
    route = Route(waypoints=[wp])

    kml_str = route_to_litchi_kml(route)
    root = ET.fromstring(kml_str)

    coords_elem = root.find(f".//{{{KML_NS}}}coordinates")
    assert coords_elem is not None and coords_elem.text is not None

    first_tuple = coords_elem.text.strip().split()[0]
    parts = first_tuple.split(",")
    assert parts[0].startswith("-46.6")


def test_no_altitudemode_nor_polygon():
    """Test that output contains no altitudeMode element nor Polygon tag."""
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0)
    route = Route(waypoints=[wp])

    kml_str = route_to_litchi_kml(route)
    assert "altitudeMode" not in kml_str
    assert "Polygon" not in kml_str

    root = ET.fromstring(kml_str)
    assert root.find(f".//{{{KML_NS}}}altitudeMode") is None
    assert root.find(f".//{{{KML_NS}}}Polygon") is None


def test_zero_altitude_warning():
    """Test that waypoint altitude 0.0 triggers the 30 m Mission Hub trap warning."""
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=0.0)
    route = Route(waypoints=[wp])

    warnings = validate_litchi_kml_route(route)
    assert any("replaces zero height with 30 m" in w for w in warnings)
    assert (
        "Height of waypoint 1 is zero in the KML: Mission Hub replaces zero height with 30 m without warning."
        in warnings
    )


def test_altitude_out_of_range_warning():
    """Test that waypoint altitude 600.0 triggers out-of-range warning."""
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=600.0)
    route = Route(waypoints=[wp])

    warnings = validate_litchi_kml_route(route)
    assert any("outside the range" in w for w in warnings)
    assert (
        f"Height of 600.00 m at waypoint 1 outside the range [{MIN_KML_ALTITUDE}, 500.0]: Mission Hub truncates it to the limit."
        in warnings
    )


def test_empty_route_raises_value_error():
    """Test that exporting a route with no waypoints raises ValueError."""
    route = Route(waypoints=[])
    with pytest.raises(ValueError, match=r"Route has no waypoints: nothing to export to KML\."):
        route_to_litchi_kml(route)


def test_untransported_fields_warning_always_present():
    """Test that untransported fields warning is always included in validation."""
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0)
    route = Route(waypoints=[wp])

    warnings = validate_litchi_kml_route(route)
    expected = (
        "KML does not carry heading, gimbal, speed, curve, or POI: "
        "Litchi applies its own defaults. Adjust in Mission Hub after importing, or use the .csv, which carries everything."
    )
    assert expected in warnings


def test_valid_xml_parsing():
    """Test that KML output string is well-formed XML parseable by ElementTree."""
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0)
    route = Route(waypoints=[wp])

    kml_str = route_to_litchi_kml(route)
    root = ET.fromstring(kml_str)
    assert root.tag == f"{{{KML_NS}}}kml"


def test_save_litchi_kml():
    """Test saving Litchi KML to file and returning validation warnings."""
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0)
    route = Route(waypoints=[wp])

    with tempfile.TemporaryDirectory() as tmpdir:
        out_path = Path(tmpdir) / "subfolder" / "mission.kml"
        warnings = save_litchi_kml(out_path, route)
        assert out_path.exists()
        content = out_path.read_text(encoding="utf-8")
        assert "<kml" in content
        assert len(warnings) >= 1
