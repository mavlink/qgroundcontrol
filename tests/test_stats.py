"""Unit tests for qgc4qgis core flight statistics module."""

import math

from qgc4qgis.core.stats import (
    FlightStats,
    calculate_camera_shots,
    calculate_flight_stats,
    calculate_flight_time,
    calculate_polygon_area,
    calculate_polyline_distance,
    calculate_time_between_shots,
    check_min_trigger_interval_warning,
)


def test_calculate_polygon_area_planar() -> None:
    # 100m x 100m square in planar coordinates
    square = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    area = calculate_polygon_area(square, is_geodetic=False)
    assert math.isclose(area, 10000.0, rel_tol=1e-6)


def test_calculate_polygon_area_geodetic() -> None:
    # Small geodetic polygon near origin
    poly = [(0.0, 0.0), (0.001, 0.0), (0.001, 0.001), (0.0, 0.001)]
    area = calculate_polygon_area(poly, is_geodetic=True)
    assert area > 0.0


def test_calculate_polygon_area_degenerate() -> None:
    assert calculate_polygon_area([(0.0, 0.0), (10.0, 10.0)], is_geodetic=False) == 0.0
    assert calculate_polygon_area([], is_geodetic=False) == 0.0


def test_calculate_polyline_distance_planar() -> None:
    pts = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0)]
    dist = calculate_polyline_distance(pts, is_geodetic=False)
    assert math.isclose(dist, 20.0, rel_tol=1e-6)


def test_calculate_polyline_distance_transects() -> None:
    transects = [
        [(0.0, 0.0), (100.0, 0.0)],
        [(100.0, 20.0), (0.0, 20.0)],
    ]
    dist = calculate_polyline_distance(transects, is_geodetic=False)
    # 100m + 20m + 100m = 220m
    assert math.isclose(dist, 220.0, rel_tol=1e-6)


def test_calculate_polyline_distance_empty() -> None:
    assert calculate_polyline_distance([], is_geodetic=False) == 0.0
    assert calculate_polyline_distance([(0.0, 0.0)], is_geodetic=False) == 0.0


def test_calculate_camera_shots_turnaround_true() -> None:
    # 100m distance, 30m trigger distance -> ceil(100/30) = 4 photos
    shots = calculate_camera_shots(
        total_distance=100.0,
        trigger_distance=30.0,
        camera_trigger_in_turnaround=True,
    )
    assert shots == 4


def test_calculate_camera_shots_turnaround_false() -> None:
    # 2 transects with turnaround distance = 10m
    # Each transect layout: [ta_entry, p_start, p_end, ta_exit]
    # Length of p_start to p_end = 50m.
    # Trigger distance = 20m -> ceil(50/20) = 3 shots per transect -> 6 total shots
    transects = [
        [(-10.0, 0.0), (0.0, 0.0), (50.0, 0.0), (60.0, 0.0)],
        [(60.0, 10.0), (50.0, 10.0), (0.0, 10.0), (-10.0, 10.0)],
    ]
    shots = calculate_camera_shots(
        total_distance=140.0,
        trigger_distance=20.0,
        camera_trigger_in_turnaround=False,
        transects=transects,
        turnaround_distance=10.0,
        hover_and_capture=False,
        is_geodetic=False,
    )
    assert shots == 6


def test_calculate_camera_shots_invalid_trigger_dist() -> None:
    assert calculate_camera_shots(total_distance=100.0, trigger_distance=0.0) == 0
    assert calculate_camera_shots(total_distance=-10.0, trigger_distance=5.0) == 0


def test_calculate_time_between_shots() -> None:
    # 30m trigger distance, 10m/s speed -> 3.0s interval
    tb = calculate_time_between_shots(trigger_distance=30.0, flight_speed=10.0)
    assert math.isclose(tb, 3.0, rel_tol=1e-6)

    assert calculate_time_between_shots(trigger_distance=0.0, flight_speed=10.0) == 0.0
    assert calculate_time_between_shots(trigger_distance=30.0, flight_speed=0.0) == 0.0


def test_calculate_flight_time() -> None:
    # 1000m total distance, 10m/s speed -> 100s base time
    ft = calculate_flight_time(total_distance=1000.0, flight_speed=10.0)
    assert math.isclose(ft, 100.0, rel_tol=1e-6)

    # Hover and capture with 2 transects of 2 points each, hover_delay = 5s -> +20s delay
    transects = [[(0.0, 0.0), (10.0, 0.0)], [(10.0, 10.0), (0.0, 10.0)]]
    ft_hover = calculate_flight_time(
        total_distance=1000.0,
        flight_speed=10.0,
        hover_and_capture=True,
        transects=transects,
        hover_delay=5.0,
    )
    assert math.isclose(ft_hover, 120.0, rel_tol=1e-6)


def test_check_min_trigger_interval_warning() -> None:
    # Trigger interval 1.0s < min interval 2.0s -> trigger warning
    is_warning, msg = check_min_trigger_interval_warning(
        time_between_shots=1.0, min_trigger_interval=2.0, camera_shots=5
    )
    assert is_warning is True
    assert msg is not None
    assert "below minimum interval (2.0 secs)" in msg

    # Trigger interval 2.5s >= min interval 2.0s -> no warning
    is_warning, msg = check_min_trigger_interval_warning(
        time_between_shots=2.5, min_trigger_interval=2.0, camera_shots=5
    )
    assert is_warning is False
    assert msg is None

    # No camera shots or no min_trigger_interval -> no warning
    is_warning, msg = check_min_trigger_interval_warning(
        time_between_shots=0.5, min_trigger_interval=2.0, camera_shots=0
    )
    assert is_warning is False
    assert msg is None


def test_calculate_flight_stats_full() -> None:
    poly = [(0.0, 0.0), (100.0, 0.0), (100.0, 100.0), (0.0, 100.0)]
    transects = [
        [(0.0, 0.0), (100.0, 0.0)],
        [(100.0, 50.0), (0.0, 50.0)],
    ]
    stats = calculate_flight_stats(
        polygon_points=poly,
        transects=transects,
        trigger_distance=25.0,
        flight_speed=10.0,
        camera_trigger_in_turnaround=True,
        min_trigger_interval=3.0,
        is_geodetic=False,
    )
    assert isinstance(stats, FlightStats)
    assert stats.polygon_area == 10000.0
    assert stats.total_distance == 250.0  # 100 + 50 + 100
    assert stats.camera_shots == 10  # ceil(250 / 25)
    assert stats.time_between_shots == 2.5  # 25 / 10
    assert stats.flight_time == 25.0  # 250 / 10
    assert stats.is_interval_too_short is True
    assert stats.warning_message is not None
