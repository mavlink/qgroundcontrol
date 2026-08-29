"""Unit tests for qgc4qgis core route module."""

import math

from qgc4qgis.core.geo import AEQDProjection, geodesic_azimuth
from qgc4qgis.core.missionitems import DistanceMode
from qgc4qgis.core.route import (
    PhotoCenter,
    Route,
    RouteWaypoint,
    rebase_route_to_takeoff,
    route_from_transects,
    sample_photo_centers,
)
from qgc4qgis.core.survey import generate_survey_transects


def test_route_waypoint_dataclass() -> None:
    wp = RouteWaypoint(
        lat=-23.55,
        lon=-46.63,
        altura=100.0,
        velocidade=12.5,
        heading=90.0,
        gimbal_pitch=-90.0,
        acoes=[{"command": 206}],
    )
    assert wp.lat == -23.55
    assert wp.lon == -46.63
    assert wp.altura == 100.0
    assert wp.alt == 100.0
    assert wp.velocidade == 12.5
    assert wp.heading == 90.0
    assert wp.gimbal_pitch == -90.0
    assert wp.acoes == [{"command": 206}]
    assert wp.actions == [{"command": 206}]


def test_route_dataclass_defaults() -> None:
    route = Route()
    assert route.waypoints == []
    assert route.modo_disparo == "POR_DISTANCIA"
    assert route.trigger_mode == "POR_DISTANCIA"
    assert route.distancia_disparo == 0.0
    assert route.trigger_distance == 0.0
    assert route.modo_altitude == DistanceMode.RELATIVE
    assert route.distance_mode == DistanceMode.RELATIVE
    assert route.avisos == []
    assert route.warnings == []


def test_route_from_transects_empty() -> None:
    route = route_from_transects([])
    assert route.waypoints == []
    assert route.modo_disparo == "POR_DISTANCIA"


def test_route_from_transects_single_point() -> None:
    transects = [[(-23.55, -46.63)]]
    route = route_from_transects(transects, altitude=60.0)
    assert len(route.waypoints) == 1
    wp = route.waypoints[0]
    assert wp.lat == -23.55
    assert wp.lon == -46.63
    assert wp.altura == 60.0
    assert wp.heading == 0.0


def test_route_from_transects_por_distancia() -> None:
    # 2 transects, each with start and end points (lat, lon)
    # Transect 0: (-23.0, -46.0) to (-23.0, -45.99) (Eastward, ~90 deg heading)
    # Transect 1: (-22.99, -45.99) to (-22.99, -46.0) (Westward, ~270 deg heading)
    p0 = (-23.0, -46.0)
    p1 = (-23.0, -45.99)
    p2 = (-22.99, -45.99)
    p3 = (-22.99, -46.0)

    transects = [[p0, p1], [p2, p3]]

    route = route_from_transects(
        transects,
        altitude=50.0,
        trigger_distance=25.0,
        flight_speed=15.0,
        gimbal_pitch=-90.0,
    )

    assert route.modo_disparo == "POR_DISTANCIA"
    assert route.distancia_disparo == 25.0
    assert len(route.waypoints) == 4

    wp0, wp1, wp2, wp3 = route.waypoints

    # Heading wp0: segment p0 -> p1 (~90 deg)
    az_01 = geodesic_azimuth(p0[0], p0[1], p1[0], p1[1])
    assert math.isclose(wp0.heading, az_01, abs_tol=1e-5)

    # Heading wp1: end of transect 0 inherits its own transect azimuth
    assert wp1.heading == wp0.heading
    assert math.isclose(wp1.heading, az_01, abs_tol=1e-5)

    # Heading wp2: segment p2 -> p3 (~270 deg)
    az_23 = geodesic_azimuth(p2[0], p2[1], p3[0], p3[1])
    assert math.isclose(wp2.heading, az_23, abs_tol=1e-5)

    # Heading wp3: last waypoint inherits preceding heading (wp2)
    assert wp3.heading == wp2.heading
    assert math.isclose(wp3.heading, az_23, abs_tol=1e-5)

    # Attributes check
    for wp in (wp0, wp1, wp2, wp3):
        assert wp.altura == 50.0
        assert wp.velocidade == 15.0
        assert wp.gimbal_pitch == -90.0


def test_route_from_transects_3d_points_and_aliases() -> None:
    transects = [[(-23.0, -46.0, 75.0), (-23.0, -45.99, 80.0)]]
    route = route_from_transects(
        transects,
        altura=60.0,  # Alias
        distancia_disparo=30.0,  # Alias
        modo_disparo="POR_DISTANCIA",
        velocidade=10.0,
    )

    assert route.distancia_disparo == 30.0
    assert len(route.waypoints) == 2
    assert route.waypoints[0].altura == 75.0
    assert route.waypoints[1].altura == 80.0
    assert route.waypoints[1].heading == route.waypoints[0].heading


def test_sample_photo_centers_empty_or_short() -> None:
    assert sample_photo_centers([], 25.0) == []
    assert sample_photo_centers([(-23.0, -46.0)], 25.0) == []


def test_sample_photo_centers_basic() -> None:
    # Transect of ~100m length eastward
    transect = [(-23.0, -46.0), (-23.0, -45.999)]
    centers = sample_photo_centers(transect, trigger_distance=30.0)

    assert len(centers) > 0
    first = centers[0]
    assert isinstance(first, PhotoCenter)
    assert first.photo_in_transect_id == 1
    assert math.isclose(first.lat, -23.0, abs_tol=1e-5)
    assert math.isclose(first.lon, -46.0, abs_tol=1e-5)

    corners = first.footprint_corners(fp_frontal=50.0, fp_side=80.0)
    assert len(corners) == 4


def test_route_200m_square_por_distancia_and_por_foto() -> None:
    poly_planar = [(0.0, 0.0), (200.0, 0.0), (200.0, 200.0), (0.0, 200.0)]
    planar_transects = generate_survey_transects(poly_planar, grid_angle=0.0, grid_spacing=50.0)
    assert len(planar_transects) == 4

    proj = AEQDProjection(-23.55, -46.63)
    geo_transects = [[proj.inverse(x, y) for x, y in t] for t in planar_transects]

    route_dist = route_from_transects(geo_transects, trigger_mode="POR_DISTANCIA")
    assert len(route_dist.waypoints) == 2 * len(geo_transects)

    az_t0 = geodesic_azimuth(
        geo_transects[0][0][0],
        geo_transects[0][0][1],
        geo_transects[0][1][0],
        geo_transects[0][1][1],
    )
    assert math.isclose(route_dist.waypoints[0].heading, az_t0, abs_tol=1e-3)

    az_t1 = geodesic_azimuth(
        geo_transects[1][0][0],
        geo_transects[1][0][1],
        geo_transects[1][1][0],
        geo_transects[1][1][1],
    )
    assert math.isclose(route_dist.waypoints[2].heading, az_t1, abs_tol=1e-3)
    assert abs(az_t0 - az_t1) > 170.0

    trigger = 50.0
    comprimento = 200.0
    expected_per_transect = math.ceil(comprimento / trigger) + 1
    route_foto = route_from_transects(
        geo_transects, trigger_mode="POR_FOTO", trigger_distance=trigger
    )
    assert len(route_foto.waypoints) == len(geo_transects) * expected_per_transect
    assert all(wp.acoes == [{"actiontype": 1, "actionparam": 0}] for wp in route_foto.waypoints)

    for wp in route_foto.waypoints[0:expected_per_transect]:
        assert math.isclose(wp.heading, az_t0, abs_tol=1e-3)

    for wp in route_foto.waypoints[expected_per_transect : 2 * expected_per_transect]:
        assert math.isclose(wp.heading, az_t1, abs_tol=1e-3)

    route_warn = route_from_transects(
        geo_transects, trigger_mode="POR_FOTO", trigger_distance=trigger, max_waypoints=15
    )
    assert len(route_warn.warnings) == 1
    assert "exceeds the limit" in route_warn.warnings[0]


def test_rebase_route_to_takeoff_calc_above_terrain() -> None:
    wps = [
        RouteWaypoint(
            lat=-23.55,
            lon=-46.63,
            altura=110.0,
            velocidade=10.0,
            heading=90.0,
            gimbal_pitch=-90.0,
            acoes=[{"actiontype": 1}],
        ),
        RouteWaypoint(
            lat=-23.56,
            lon=-46.64,
            altura=120.0,
            velocidade=10.0,
            heading=90.0,
            gimbal_pitch=-90.0,
            acoes=[],
        ),
        RouteWaypoint(
            lat=-23.57,
            lon=-46.65,
            altura=130.0,
            velocidade=10.0,
            heading=90.0,
            gimbal_pitch=-90.0,
            acoes=[],
        ),
    ]
    route_orig = Route(
        waypoints=wps,
        modo_disparo="POR_FOTO",
        distancia_disparo=25.0,
        modo_altitude=DistanceMode.CALC_ABOVE_TERRAIN,
        avisos=["Aviso original"],
    )

    rebased = rebase_route_to_takeoff(route_orig, takeoff_elevation=60.0)

    assert rebased.modo_altitude == DistanceMode.RELATIVE
    assert [wp.altura for wp in rebased.waypoints] == [50.0, 60.0, 70.0]
    assert rebased.modo_disparo == "POR_FOTO"
    assert rebased.distancia_disparo == 25.0
    assert len(rebased.avisos) == 2
    assert rebased.avisos[0] == "Aviso original"
    assert rebased.avisos[1] == (
        "Terrain mode converted to height relative to the takeoff point "
        "(elevation 60.0 m); heights from 50.0 m to 70.0 m."
    )
    assert rebased.waypoints[0].velocidade == 10.0
    assert rebased.waypoints[0].heading == 90.0
    assert rebased.waypoints[0].gimbal_pitch == -90.0
    assert rebased.waypoints[0].acoes == [{"actiontype": 1}]

    # Original route not mutated
    assert route_orig.modo_altitude == DistanceMode.CALC_ABOVE_TERRAIN
    assert [wp.altura for wp in route_orig.waypoints] == [110.0, 120.0, 130.0]
    assert route_orig.avisos == ["Aviso original"]


def test_rebase_route_to_takeoff_already_relative() -> None:
    wps = [RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0)]
    route_rel = Route(
        waypoints=wps,
        modo_altitude=DistanceMode.RELATIVE,
        avisos=["Aviso preexistente"],
    )

    rebased = rebase_route_to_takeoff(route_rel, takeoff_elevation=60.0)

    assert rebased is route_rel
    assert rebased.modo_altitude == DistanceMode.RELATIVE
    assert [wp.altura for wp in rebased.waypoints] == [50.0]
    assert rebased.avisos == ["Aviso preexistente"]


def test_rebase_route_to_takeoff_negative_altitude_warning() -> None:
    wps = [
        RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0),
        RouteWaypoint(lat=-23.56, lon=-46.64, altura=60.0),
    ]
    route_orig = Route(
        waypoints=wps,
        modo_altitude=DistanceMode.ABSOLUTE,
    )

    rebased = rebase_route_to_takeoff(route_orig, takeoff_elevation=60.0)

    assert [wp.altura for wp in rebased.waypoints] == [-10.0, 0.0]
    assert rebased.modo_altitude == DistanceMode.RELATIVE
    assert len(rebased.avisos) == 2
    assert rebased.avisos[1] == (
        "Relative height ≤ 0 in 2 waypoint(s): the route goes below the takeoff point."
    )


def test_rebase_route_to_takeoff_preserves_wait_time() -> None:
    wp = RouteWaypoint(lat=-23.55, lon=-46.63, altura=110.0)
    wp.wait_time = 2.5
    route_orig = Route(waypoints=[wp], modo_altitude=DistanceMode.CALC_ABOVE_TERRAIN)

    rebased = rebase_route_to_takeoff(route_orig, takeoff_elevation=60.0)

    assert rebased.waypoints[0].altura == 50.0
    assert rebased.waypoints[0].wait_time == 2.5
    assert route_orig.waypoints[0].wait_time == 2.5
