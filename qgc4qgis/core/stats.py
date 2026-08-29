"""Flight statistics calculations ported from QGroundControl SurveyComplexItem and CameraCalc."""

import math
from collections.abc import Sequence
from dataclasses import dataclass

from qgc4qgis.core.geo import geodesic_distance, polygon_area_geo, shoelace_area


@dataclass
class FlightStats:
    """Flight statistics summary for survey flight plans."""

    polygon_area: float = 0.0  # Area in square meters (m²)
    total_distance: float = 0.0  # Total flight distance in meters (m)
    camera_shots: int = 0  # Total number of camera photos
    flight_time: float = 0.0  # Total flight duration in seconds (s)
    time_between_shots: float = 0.0  # Interval between photo triggers in seconds (s)
    min_trigger_interval: float = 0.0  # Minimum supported camera trigger interval (s)
    is_interval_too_short: bool = False  # True if time_between_shots < min_trigger_interval
    warning_message: str | None = None  # Warning message if interval is too short


def calculate_polygon_area(
    polygon: Sequence[tuple[float, float]], is_geodetic: bool = True
) -> float:
    """Calculate 2D polygon area in square meters using the Shoelace formula.

    :param polygon: Sequence of (lat, lon) or (x, y) vertices.
    :param is_geodetic: If True, projects (lat, lon) to AEQD meters first. If False, assumes (x, y) in meters.
    :return: Non-negative area in square meters.
    """
    if len(polygon) < 3:
        return 0.0
    if is_geodetic:
        return polygon_area_geo(polygon)
    return shoelace_area(polygon)


def calculate_polyline_distance(
    points: Sequence[tuple[float, float]] | Sequence[Sequence[tuple[float, float]]],
    is_geodetic: bool = True,
) -> float:
    """Calculate total polyline distance in meters along points or sequence of transects.

    :param points: Sequence of 2D coordinates [(x1, y1), ...] or list of transect point sequences.
    :param is_geodetic: If True, uses WGS84 geodesic distance. If False, uses 2D planar Euclidean distance.
    :return: Total distance in meters.
    """
    if not points:
        return 0.0

    flat_pts: list[tuple[float, float]] = []
    if (
        isinstance(points[0], (list, tuple))
        and points[0]
        and isinstance(points[0][0], (list, tuple))
    ):
        for transect in points:
            flat_pts.extend(transect)  # type: ignore[arg-type]
    else:
        flat_pts = list(points)  # type: ignore[arg-type]

    if len(flat_pts) < 2:
        return 0.0

    total_dist = 0.0
    for i in range(len(flat_pts) - 1):
        p1 = flat_pts[i]
        p2 = flat_pts[i + 1]
        if is_geodetic:
            total_dist += geodesic_distance(p1[0], p1[1], p2[0], p2[1])
        else:
            total_dist += math.hypot(p2[0] - p1[0], p2[1] - p1[1])

    return total_dist


def calculate_camera_shots(
    total_distance: float,
    trigger_distance: float,
    camera_trigger_in_turnaround: bool = True,
    transects: Sequence[Sequence[tuple[float, float]]] | None = None,
    turnaround_distance: float = 0.0,
    hover_and_capture: bool = False,
    is_geodetic: bool = True,
) -> int:
    """Calculate total number of camera photo triggers.

    Supports two variants of CameraTriggerInTurnAround:
    1. camera_trigger_in_turnaround=True:
       `camera_shots = ceil(total_distance / trigger_distance)`
    2. camera_trigger_in_turnaround=False:
       Per-transect calculation excluding turnaround segments when applicable:
       `camera_shots = sum(ceil(transect_survey_distance / trigger_distance))`

    :param total_distance: Total flight path distance in meters.
    :param trigger_distance: Trigger distance between photos in meters.
    :param camera_trigger_in_turnaround: If True, camera triggers during turnarounds.
    :param transects: Sequence of transects, where each transect is a sequence of 2D coordinates.
    :param turnaround_distance: Turnaround extension distance in meters.
    :param hover_and_capture: If True, hover and capture mode is active.
    :param is_geodetic: If True, uses WGS84 geodesic distance for transect segments.
    :return: Integer count of photo shots.
    """
    if trigger_distance <= 0.0:
        return 0

    if camera_trigger_in_turnaround or not transects:
        if total_distance <= 0.0:
            return 0
        return math.ceil(total_distance / trigger_distance)

    total_shots = 0
    has_turnaround = turnaround_distance > 0.0

    for transect in transects:
        if not transect:
            continue

        if has_turnaround and not hover_and_capture and len(transect) >= 4:
            survey_pts = transect[1:-1]
        else:
            survey_pts = transect

        t_dist = calculate_polyline_distance(survey_pts, is_geodetic=is_geodetic)
        total_shots += math.ceil(t_dist / trigger_distance)

    return total_shots


def calculate_time_between_shots(trigger_distance: float, flight_speed: float) -> float:
    """Calculate time interval in seconds between photo captures.

    :param trigger_distance: Trigger distance in meters.
    :param flight_speed: Flight speed in m/s.
    :return: Time in seconds (0.0 if speed <= 0 or trigger_distance <= 0).
    """
    if trigger_distance <= 0.0 or flight_speed <= 0.0:
        return 0.0
    return trigger_distance / flight_speed


def calculate_flight_time(
    total_distance: float,
    flight_speed: float,
    hover_and_capture: bool = False,
    transects: Sequence[Sequence[tuple[float, float]]] | None = None,
    hover_delay: float = 0.0,
) -> float:
    """Calculate total flight time in seconds.

    :param total_distance: Total flight path distance in meters.
    :param flight_speed: Flight speed in m/s.
    :param hover_and_capture: If True, includes hover delays at waypoints.
    :param transects: Sequence of transect points.
    :param hover_delay: Hover delay per waypoint in seconds.
    :return: Total flight time in seconds.
    """
    if flight_speed <= 0.0 or total_distance <= 0.0:
        base_time = 0.0
    else:
        base_time = total_distance / flight_speed

    additional_delay = 0.0
    if hover_and_capture and transects and hover_delay > 0.0:
        for transect in transects:
            additional_delay += len(transect) * hover_delay

    return base_time + additional_delay


def check_min_trigger_interval_warning(
    time_between_shots: float, min_trigger_interval: float, camera_shots: int = 1
) -> tuple[bool, str | None]:
    """Check if time between shots is below the camera's minimum trigger interval.

    :param time_between_shots: Calculated time between photo triggers in seconds.
    :param min_trigger_interval: Minimum trigger interval supported by camera in seconds.
    :param camera_shots: Number of photo shots planned.
    :return: Tuple of (is_too_short, warning_message).
    """
    if (
        camera_shots > 0
        and min_trigger_interval > 0.0
        and time_between_shots < min_trigger_interval
    ):
        msg = (
            f"WARNING: Photo interval is below minimum interval "
            f"({min_trigger_interval:.1f} secs) supported by camera."
        )
        return True, msg
    return False, None


def calculate_flight_stats(
    polygon_points: Sequence[tuple[float, float]] | None = None,
    transects: Sequence[Sequence[tuple[float, float]]]
    | Sequence[tuple[float, float]]
    | None = None,
    trigger_distance: float = 0.0,
    flight_speed: float = 15.0,
    camera_trigger_in_turnaround: bool = True,
    turnaround_distance: float = 0.0,
    hover_and_capture: bool = False,
    hover_delay: float = 0.0,
    min_trigger_interval: float = 0.0,
    is_geodetic: bool = True,
) -> FlightStats:
    """Compute flight statistics summary for survey plan.

    :param polygon_points: Boundary polygon vertices [(lat, lon), ...] or [(x, y), ...].
    :param transects: Sequence of transects or flattened polyline points.
    :param trigger_distance: Distance between photo captures in meters.
    :param flight_speed: Vehicle cruise/flight speed in m/s.
    :param camera_trigger_in_turnaround: If True, keep camera triggering during turnarounds.
    :param turnaround_distance: Turnaround extension distance in meters.
    :param hover_and_capture: If True, hover and capture mode enabled.
    :param hover_delay: Delay in seconds at each hover waypoint.
    :param min_trigger_interval: Camera minimum trigger interval in seconds.
    :param is_geodetic: If True, coordinates are WGS84 (lat, lon). If False, 2D planar (x, y) meters.
    :return: FlightStats dataclass with all computed metrics and warning flags.
    """
    area = (
        calculate_polygon_area(polygon_points, is_geodetic=is_geodetic) if polygon_points else 0.0
    )
    dist = calculate_polyline_distance(transects, is_geodetic=is_geodetic) if transects else 0.0

    nested_transects: list[list[tuple[float, float]]] | None = None
    if (
        transects
        and isinstance(transects[0], (list, tuple))
        and transects[0]
        and isinstance(transects[0][0], (list, tuple))
    ):
        nested_transects = [list(t) for t in transects]  # type: ignore[union-attr]

    shots = calculate_camera_shots(
        total_distance=dist,
        trigger_distance=trigger_distance,
        camera_trigger_in_turnaround=camera_trigger_in_turnaround,
        transects=nested_transects,
        turnaround_distance=turnaround_distance,
        hover_and_capture=hover_and_capture,
        is_geodetic=is_geodetic,
    )

    tb_shots = calculate_time_between_shots(trigger_distance, flight_speed)
    ftime = calculate_flight_time(
        total_distance=dist,
        flight_speed=flight_speed,
        hover_and_capture=hover_and_capture,
        transects=nested_transects,
        hover_delay=hover_delay,
    )

    is_too_short, warn_msg = check_min_trigger_interval_warning(
        time_between_shots=tb_shots,
        min_trigger_interval=min_trigger_interval,
        camera_shots=shots,
    )

    return FlightStats(
        polygon_area=area,
        total_distance=dist,
        camera_shots=shots,
        flight_time=ftime,
        time_between_shots=tb_shots,
        min_trigger_interval=min_trigger_interval,
        is_interval_too_short=is_too_short,
        warning_message=warn_msg,
    )
