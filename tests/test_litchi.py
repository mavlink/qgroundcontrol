"""Unit tests for Litchi CSV exporter module."""

import csv
import io
import tempfile
from pathlib import Path

from qgc4qgis.core.litchi import (
    clamp_gimbal_pitch,
    get_litchi_headers,
    route_to_litchi_csv,
    route_to_litchi_rows,
    save_litchi_csv,
    validate_litchi_route,
)
from qgc4qgis.core.missionitems import DistanceMode
from qgc4qgis.core.route import Route, RouteWaypoint


def test_clamp_gimbal_pitch():
    assert clamp_gimbal_pitch(-100.0) == -90.0
    assert clamp_gimbal_pitch(30.0) == 20.0
    assert clamp_gimbal_pitch(-45.0) == -45.0
    assert clamp_gimbal_pitch(None, default=-90.0) == -90.0
    assert clamp_gimbal_pitch(None, default=0.0) == 0.0


def test_get_litchi_headers():
    headers_0 = get_litchi_headers(0)
    assert headers_0 == [
        "latitude",
        "longitude",
        "altitude(m)",
        "heading(deg)",
        "curvesize(m)",
        "rotationdir",
        "gimbalmode",
        "gimbalpitchangle",
        "altitudemode",
        "speed(m/s)",
        "poi_latitude",
        "poi_longitude",
        "poi_altitude(m)",
        "poi_altitudemode",
        "photo_timeinterval",
        "photo_distinterval",
    ]

    headers_2 = get_litchi_headers(2)
    assert headers_2 == [
        "latitude",
        "longitude",
        "altitude(m)",
        "heading(deg)",
        "curvesize(m)",
        "rotationdir",
        "gimbalmode",
        "gimbalpitchangle",
        "actiontype1",
        "actionparam1",
        "actiontype2",
        "actionparam2",
        "altitudemode",
        "speed(m/s)",
        "poi_latitude",
        "poi_longitude",
        "poi_altitude(m)",
        "poi_altitudemode",
        "photo_timeinterval",
        "photo_distinterval",
    ]


def test_route_to_litchi_rows_basic():
    wp1 = RouteWaypoint(
        lat=-23.5505,
        lon=-46.6333,
        altura=50.0,
        heading=90.0,
        velocidade=5.0,
        gimbal_pitch=-90.0,
        acoes=[{"type": 1, "param": 1000}],
    )
    wp2 = RouteWaypoint(
        lat=-23.5510,
        lon=-46.6340,
        altura=60.0,
        heading=180.0,
        velocidade=6.0,
        gimbal_pitch=15.0,
        acoes=[{"type": 0, "param": 0}, {"type": 2, "param": 500}],
    )

    route = Route(
        waypoints=[wp1, wp2],
        modo_disparo="POR_DISTANCIA",
        distancia_disparo=15.0,
        modo_altitude=DistanceMode.RELATIVE,
    )

    rows = route_to_litchi_rows(route)
    assert len(rows) == 2

    # Check header ordering & max actions = 2
    row1 = rows[0]
    keys1 = list(row1.keys())
    assert keys1 == get_litchi_headers(2)

    # Waypoint 1 values
    assert row1["latitude"] == -23.5505
    assert row1["longitude"] == -46.6333
    assert row1["altitude(m)"] == 50.0
    assert row1["heading(deg)"] == 90.0
    assert row1["curvesize(m)"] == 0
    assert row1["rotationdir"] == 0
    assert row1["gimbalmode"] == 2
    assert row1["gimbalpitchangle"] == -90.0
    assert row1["actiontype1"] == 1
    assert row1["actionparam1"] == 1000.0
    # Short action list padding (-1, 0)
    assert row1["actiontype2"] == -1
    assert row1["actionparam2"] == 0.0
    assert row1["altitudemode"] == 0
    assert row1["speed(m/s)"] == 5.0
    assert row1["poi_latitude"] == 0.0
    assert row1["poi_longitude"] == 0.0
    assert row1["poi_altitude(m)"] == 0.0
    assert row1["poi_altitudemode"] == 0
    assert row1["photo_timeinterval"] == -1
    assert row1["photo_distinterval"] == 15.0

    # Waypoint 2 values
    row2 = rows[1]
    assert row2["actiontype1"] == 0
    assert row2["actionparam1"] == 0.0
    assert row2["actiontype2"] == 2
    assert row2["actionparam2"] == 500.0


def test_pitch_clamping():
    wp_too_low = RouteWaypoint(lat=0.0, lon=0.0, gimbal_pitch=-110.0)
    wp_too_high = RouteWaypoint(lat=0.0, lon=0.0, gimbal_pitch=45.0)
    route = Route(waypoints=[wp_too_low, wp_too_high])

    rows = route_to_litchi_rows(route)
    assert rows[0]["gimbalpitchangle"] == -90.0
    assert rows[1]["gimbalpitchangle"] == 20.0


def test_photo_interval_mutual_exclusion():
    wp = RouteWaypoint(lat=0.0, lon=0.0)
    route = Route(waypoints=[wp])

    # Case 1: photo_distinterval set explicitly
    rows1 = route_to_litchi_rows(route, photo_distinterval=12.0, photo_timeinterval=3.0)
    assert rows1[0]["photo_distinterval"] == 12.0
    assert rows1[0]["photo_timeinterval"] == -1

    # Case 2: photo_timeinterval set explicitly (when photo_distinterval is None/0)
    rows2 = route_to_litchi_rows(route, photo_timeinterval=4.0)
    assert rows2[0]["photo_timeinterval"] == 4.0
    assert rows2[0]["photo_distinterval"] == -1

    # Case 3: derived from route (POR_TEMPO)
    route_tempo = Route(waypoints=[wp], modo_disparo="POR_TEMPO", distancia_disparo=2.5)
    rows3 = route_to_litchi_rows(route_tempo)
    assert rows3[0]["photo_timeinterval"] == 2.5
    assert rows3[0]["photo_distinterval"] == -1

    # Case 4: POR_FOTO mode -> both intervals unused (-1)
    route_foto = Route(waypoints=[wp], modo_disparo="POR_FOTO", distancia_disparo=10.0)
    rows4 = route_to_litchi_rows(route_foto)
    assert rows4[0]["photo_timeinterval"] == -1
    assert rows4[0]["photo_distinterval"] == -1


def test_route_to_litchi_csv_and_save():
    wp = RouteWaypoint(lat=10.0, lon=20.0, altura=30.0, acoes=[{"actiontype": 1, "actionparam": 0}])
    route = Route(waypoints=[wp], modo_disparo="POR_FOTO")

    csv_str = route_to_litchi_csv(route)
    reader = csv.DictReader(io.StringIO(csv_str))
    fieldnames = reader.fieldnames
    assert fieldnames == get_litchi_headers(1)
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0]["latitude"] == "10.0"
    assert rows[0]["longitude"] == "20.0"

    with tempfile.TemporaryDirectory() as tmpdir:
        out_path = Path(tmpdir) / "test_route.csv"
        warnings = save_litchi_csv(out_path, route)
        assert warnings == []
        assert out_path.exists()
        content = out_path.read_text(encoding="utf-8")
        assert "latitude,longitude,altitude(m)" in content


def test_save_litchi_csv_warnings():
    # 1. Test 100 waypoints limit warning (> 99)
    wps_100 = [RouteWaypoint(lat=10.0, lon=20.0, altura=50.0) for _ in range(100)]
    route_100 = Route(waypoints=wps_100, modo_disparo="POR_FOTO")
    with tempfile.TemporaryDirectory() as tmpdir:
        out_path = Path(tmpdir) / "route_100.csv"
        warnings = save_litchi_csv(out_path, route_100)
        assert any("99" in w and "exceeds" in w for w in warnings)

    # 2. Test pitch out of range warning
    wp_invalid_pitch = RouteWaypoint(lat=10.0, lon=20.0, altura=50.0, gimbal_pitch=30.0)
    route_pitch = Route(waypoints=[wp_invalid_pitch], modo_disparo="POR_FOTO")
    with tempfile.TemporaryDirectory() as tmpdir:
        out_path = Path(tmpdir) / "route_pitch.csv"
        warnings = save_litchi_csv(out_path, route_pitch)
        assert any("pitch" in w.lower() and "outside the range" in w.lower() for w in warnings)

    # 3. Test altitude out of range warning (-250m and 600m)
    wp_low_alt = RouteWaypoint(lat=10.0, lon=20.0, altura=-250.0)
    wp_high_alt = RouteWaypoint(lat=10.0, lon=20.0, altura=600.0)
    route_alt = Route(waypoints=[wp_low_alt, wp_high_alt], modo_disparo="POR_FOTO")
    with tempfile.TemporaryDirectory() as tmpdir:
        out_path = Path(tmpdir) / "route_alt.csv"
        warnings = save_litchi_csv(out_path, route_alt)
        assert len(warnings) == 2
        assert all("altitude" in w.lower() for w in warnings)


def test_altitudemode_mapping():
    wp = RouteWaypoint(lat=10.0, lon=20.0, altura=50.0)

    # AGL -> altitudemode 0
    route_agl = Route(waypoints=[wp], modo_altitude=DistanceMode.RELATIVE)
    rows_agl = route_to_litchi_rows(route_agl)
    assert rows_agl[0]["altitudemode"] == 0

    # MSL -> altitudemode 1
    route_msl = Route(waypoints=[wp], modo_altitude=DistanceMode.ABSOLUTE)
    rows_msl = route_to_litchi_rows(route_msl)
    assert rows_msl[0]["altitudemode"] == 1

    # Terrain -> altitudemode 1
    route_terrain = Route(waypoints=[wp], modo_altitude=DistanceMode.TERRAIN)
    rows_terrain = route_to_litchi_rows(route_terrain)
    assert rows_terrain[0]["altitudemode"] == 1


def test_csv_literal_header_comparison():
    wp = RouteWaypoint(lat=-23.5505, lon=-46.6333, altura=50.0)
    route = Route(waypoints=[wp])
    csv_str = route_to_litchi_csv(route)
    header_line = csv_str.splitlines()[0]
    expected_header = (
        "latitude,longitude,altitude(m),heading(deg),curvesize(m),rotationdir,"
        "gimbalmode,gimbalpitchangle,altitudemode,speed(m/s),poi_latitude,"
        "poi_longitude,poi_altitude(m),poi_altitudemode,photo_timeinterval,photo_distinterval"
    )
    assert header_line == expected_header


def test_csv_column_count_uniformity():
    wp1 = RouteWaypoint(lat=-23.5505, lon=-46.6333, altura=50.0, acoes=[{"type": 1, "param": 100}])
    wp2 = RouteWaypoint(lat=-23.5510, lon=-46.6340, altura=60.0)
    route = Route(waypoints=[wp1, wp2])
    csv_str = route_to_litchi_csv(route)
    lines = [line for line in csv_str.splitlines() if line.strip()]
    expected_col_count = len(lines[0].split(","))
    assert len(lines) == 3  # Header + 2 rows
    for line in lines:
        assert len(line.split(",")) == expected_col_count


def test_csv_round_trip_coordinates():
    wp1 = RouteWaypoint(lat=-23.550521, lon=-46.633308, altura=123.45)
    wp2 = RouteWaypoint(lat=-23.551234, lon=-46.634567, altura=67.89)
    route = Route(waypoints=[wp1, wp2])
    csv_str = route_to_litchi_csv(route)

    reader = csv.DictReader(io.StringIO(csv_str))
    rows = list(reader)
    assert len(rows) == 2
    assert float(rows[0]["latitude"]) == wp1.lat
    assert float(rows[0]["longitude"]) == wp1.lon
    assert float(rows[0]["altitude(m)"]) == wp1.altura
    assert float(rows[1]["latitude"]) == wp2.lat
    assert float(rows[1]["longitude"]) == wp2.lon
    assert float(rows[1]["altitude(m)"]) == wp2.altura


def test_modo_por_distancia_photo_intervals():
    wp = RouteWaypoint(lat=-23.5505, lon=-46.6333, altura=50.0)
    route = Route(waypoints=[wp], modo_disparo="POR_DISTANCIA", distancia_disparo=15.5)
    rows = route_to_litchi_rows(route)
    assert rows[0]["photo_distinterval"] == 15.5
    assert rows[0]["photo_timeinterval"] == -1


def test_validate_litchi_route_photo_action_warning():
    # 1. Route with modo_disparo="POR_DISTANCIA" and acoes=[] produces the warning, and actiontype* col count is 0
    wp1 = RouteWaypoint(lat=-23.5505, lon=-46.6333, altura=50.0, acoes=[])
    route_dist = Route(waypoints=[wp1], modo_disparo="POR_DISTANCIA")

    warnings_dist = validate_litchi_route(route_dist)
    expected_warning = (
        'Trigger mode "POR_DISTANCIA": the CSV does not carry a "Take Photo" action per waypoint '
        "— waypoints only sit at the ends of transects and the capture depends on the interval "
        "(photo_distinterval/photo_timeinterval). For a photo action at every photo center, "
        'use trigger mode "By photo".'
    )
    assert expected_warning in warnings_dist

    csv_dist = route_to_litchi_csv(route_dist)
    header_dist = csv_dist.splitlines()[0].split(",")
    action_cols_dist = [c for c in header_dist if c.startswith("actiontype")]
    assert len(action_cols_dist) == 0

    # 2. Route with modo_disparo="POR_FOTO" and acoes=[{"actiontype": 1, "actionparam": 0}] does NOT produce warning, and actiontype* col count is 1
    wp2 = RouteWaypoint(
        lat=-23.5505, lon=-46.6333, altura=50.0, acoes=[{"actiontype": 1, "actionparam": 0}]
    )
    route_foto = Route(waypoints=[wp2], modo_disparo="POR_FOTO")

    warnings_foto = validate_litchi_route(route_foto)
    assert expected_warning not in warnings_foto

    csv_foto = route_to_litchi_csv(route_foto)
    header_foto = csv_foto.splitlines()[0].split(",")
    action_cols_foto = [c for c in header_foto if c.startswith("actiontype")]
    assert len(action_cols_foto) == 1
