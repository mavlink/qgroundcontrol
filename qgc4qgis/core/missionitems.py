"""Mission item generation and expansion utilities ported from QGroundControl."""

from collections.abc import Sequence
from typing import Any


class DistanceMode:
    """Altitude frame distance mode matching QGC CameraCalc / TransectStyleComplexItem."""

    RELATIVE = 1
    ABSOLUTE = 2
    CALC_ABOVE_TERRAIN = 3
    TERRAIN = 4


class MavFrame:
    """MAVLink frame enum values used in mission items."""

    GLOBAL = 0
    MISSION = 2
    GLOBAL_RELATIVE_ALT = 3
    GLOBAL_TERRAIN_ALT = 10


class MavCmd:
    """MAVLink command enum values used in mission items."""

    NAV_WAYPOINT = 16
    DO_SET_CAM_TRIGG_DIST = 206


def mav_frame_from_distance_mode(distance_mode: int) -> int:
    """Derive MAV_FRAME from DistanceMode.

    - RELATIVE (1) -> MAV_FRAME_GLOBAL_RELATIVE_ALT (3)
    - ABSOLUTE (2) -> MAV_FRAME_GLOBAL (0)
    - CALC_ABOVE_TERRAIN (3) -> MAV_FRAME_GLOBAL (0)
    - TERRAIN (4) -> MAV_FRAME_GLOBAL_TERRAIN_ALT (10)
    """
    if distance_mode in (DistanceMode.RELATIVE, 0):
        return MavFrame.GLOBAL_RELATIVE_ALT
    if distance_mode in (DistanceMode.ABSOLUTE, DistanceMode.CALC_ABOVE_TERRAIN):
        return MavFrame.GLOBAL
    if distance_mode == DistanceMode.TERRAIN:
        return MavFrame.GLOBAL_TERRAIN_ALT
    return MavFrame.GLOBAL_RELATIVE_ALT


def create_waypoint_item(
    seq: int,
    lat: float,
    lon: float,
    alt: float,
    frame: int = MavFrame.GLOBAL_RELATIVE_ALT,
    hold_time: float = 0.0,
    auto_continue: bool = True,
) -> dict[str, Any]:
    """Create a SimpleItem dictionary for MAV_CMD_NAV_WAYPOINT (16)."""
    return {
        "autoContinue": auto_continue,
        "command": MavCmd.NAV_WAYPOINT,
        "doJumpId": seq,
        "frame": frame,
        "params": [
            float(hold_time),
            0.0,
            0.0,
            None,
            float(lat),
            float(lon),
            float(alt),
        ],
        "type": "SimpleItem",
    }


def create_camera_trigger_distance_item(
    seq: int,
    trigger_distance: float,
    frame: int = MavFrame.MISSION,
    auto_continue: bool = True,
) -> dict[str, Any]:
    """Create a SimpleItem dictionary for MAV_CMD_DO_SET_CAM_TRIGG_DIST (206)."""
    return {
        "autoContinue": auto_continue,
        "command": MavCmd.DO_SET_CAM_TRIGG_DIST,
        "doJumpId": seq,
        "frame": frame,
        "params": [
            float(trigger_distance),
            0.0,
            1.0 if trigger_distance > 0 else 0.0,
            0.0,
            0.0,
            0.0,
            0.0,
        ],
        "type": "SimpleItem",
    }


def expand_transects_to_mission_items(
    transects: Sequence[Sequence[tuple[float, ...]]],
    trigger_distance: float = 0.0,
    camera_trigger_in_turnaround: bool = False,
    has_turnaround: bool = False,
    distance_mode: int = DistanceMode.RELATIVE,
    default_altitude: float = 50.0,
    start_sequence: int = 1,
) -> list[dict[str, Any]]:
    """Expand survey transects into a list of QGC SimpleItem mission item dicts.

    - Generates MAV_CMD_NAV_WAYPOINT (16) for each transect point.
    - Generates MAV_CMD_DO_SET_CAM_TRIGG_DIST (206) at survey entry/exit and turnaround points
      according to camera_trigger_in_turnaround and has_turnaround.
    - Derives MAV_FRAME from distance_mode for waypoints.

    :param transects: Sequence of transects, each containing geodetic point tuples (lat, lon) or (lat, lon, alt).
    :param trigger_distance: Distance in meters between camera shots (0.0 disables trigger commands).
    :param camera_trigger_in_turnaround: If True, keeps camera triggering active through turnarounds.
    :param has_turnaround: If True, transects contain turnaround points as first/last points.
    :param distance_mode: DistanceMode enum value for MAV_FRAME derivation.
    :param default_altitude: Default altitude in meters if points are 2D (lat, lon).
    :param start_sequence: Starting sequence number (doJumpId) for items.
    :return: List of SimpleItem mission item dictionaries.
    """
    items: list[dict[str, Any]] = []
    seq = start_sequence
    frame = mav_frame_from_distance_mode(distance_mode)
    num_transects = len(transects)

    for transect_idx, transect in enumerate(transects):
        is_first_transect = transect_idx == 0
        is_last_transect = transect_idx == num_transects - 1
        num_pts = len(transect)

        for p_idx, pt in enumerate(transect):
            lat, lon = float(pt[0]), float(pt[1])
            alt = float(pt[2]) if len(pt) >= 3 else float(default_altitude)

            if has_turnaround and num_pts >= 4:
                if p_idx == 0:
                    # Entry turnaround
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
                    if camera_trigger_in_turnaround and is_first_transect and trigger_distance > 0:
                        items.append(create_camera_trigger_distance_item(seq, trigger_distance))
                        seq += 1
                elif p_idx == 1:
                    # Survey entry
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
                    if trigger_distance > 0:
                        items.append(create_camera_trigger_distance_item(seq, trigger_distance))
                        seq += 1
                elif p_idx == num_pts - 2:
                    # Survey exit
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
                    if trigger_distance > 0 and not camera_trigger_in_turnaround:
                        items.append(create_camera_trigger_distance_item(seq, 0.0))
                        seq += 1
                elif p_idx == num_pts - 1:
                    # Exit turnaround
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
                    if camera_trigger_in_turnaround and is_last_transect and trigger_distance > 0:
                        items.append(create_camera_trigger_distance_item(seq, 0.0))
                        seq += 1
                else:
                    # Interior point
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
            else:
                if p_idx == 0:
                    # Survey entry
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
                    if trigger_distance > 0:
                        items.append(create_camera_trigger_distance_item(seq, trigger_distance))
                        seq += 1
                elif p_idx == num_pts - 1:
                    # Survey exit
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1
                    if trigger_distance > 0 and (
                        not camera_trigger_in_turnaround or is_last_transect
                    ):
                        items.append(create_camera_trigger_distance_item(seq, 0.0))
                        seq += 1
                else:
                    # Interior point
                    items.append(create_waypoint_item(seq, lat, lon, alt, frame=frame))
                    seq += 1

    return items
