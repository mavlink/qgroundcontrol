"""QGroundControl .plan file serializer for QGC4QGIS.

Generates QGroundControl Plan files conforming to QGC file format specs:
- Header: fileType="Plan", version=1, groundStation="QGroundControl"
- Mission object (v2): plannedHomePosition [lat, lon, alt], cruiseSpeed, hoverSpeed,
  firmwareType, vehicleType, globalPlanAltitudeMode, items list
- GeoFence object (v2): empty circles and polygons lists
- RallyPoints object (v2): empty points list
"""

import json
from collections.abc import Sequence
from pathlib import Path
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from qgc4qgis.core.cameracalc import CameraCalc

FILE_TYPE = "Plan"
PLAN_VERSION = 1
MISSION_VERSION = 2
GEOFENCE_VERSION = 2
RALLY_POINTS_VERSION = 2
GROUND_STATION = "QGroundControl"

SURVEY_VERSION = 5
TRANSECT_STYLE_VERSION = 2
CAMERA_CALC_VERSION = 2
MANUAL_CAMERA_NAME = "Manual (no camera specs)"
CUSTOM_CAMERA_NAME = "Custom Camera"


def build_plan_dict(
    planned_home_position: Sequence[float],
    items: Sequence[dict[str, Any]] | None = None,
    cruise_speed: float = 15.0,
    hover_speed: float = 5.0,
    firmware_type: int = 12,
    vehicle_type: int = 2,
    global_plan_altitude_mode: int = 1,
    ground_station: str = GROUND_STATION,
) -> dict[str, Any]:
    """Build a QGroundControl Plan dictionary (Plan v1, mission v2, geoFence v2, rallyPoints v2).

    :param planned_home_position: (latitude, longitude, altitude) tuple or list.
    :param items: List of mission item dictionaries (SimpleItem or ComplexItem).
    :param cruise_speed: Cruise speed in m/s (default 15.0).
    :param hover_speed: Hover speed in m/s (default 5.0).
    :param firmware_type: MAV_AUTOPILOT enum value (default 12 = PX4).
    :param vehicle_type: MAV_TYPE enum value (default 2 = Quadrotor).
    :param global_plan_altitude_mode: Altitude mode (default 1 = RelativeToHome).
    :param ground_station: Ground station software name (default "QGroundControl").
    :return: Serialized dictionary structure for a QGC .plan JSON file.
    """
    if len(planned_home_position) < 3:
        home_pos = [
            float(planned_home_position[0]),
            float(planned_home_position[1]),
            0.0,
        ]
    else:
        home_pos = [
            float(planned_home_position[0]),
            float(planned_home_position[1]),
            float(planned_home_position[2]),
        ]

    mission_items = list(items) if items is not None else []

    return {
        "fileType": FILE_TYPE,
        "geoFence": {
            "circles": [],
            "polygons": [],
            "version": GEOFENCE_VERSION,
        },
        "groundStation": ground_station,
        "mission": {
            "cruiseSpeed": float(cruise_speed),
            "firmwareType": int(firmware_type),
            "globalPlanAltitudeMode": int(global_plan_altitude_mode),
            "hoverSpeed": float(hover_speed),
            "items": mission_items,
            "plannedHomePosition": home_pos,
            "vehicleType": int(vehicle_type),
            "version": MISSION_VERSION,
        },
        "rallyPoints": {
            "points": [],
            "version": RALLY_POINTS_VERSION,
        },
        "version": PLAN_VERSION,
    }


def serialize_plan(
    planned_home_position: Sequence[float],
    items: Sequence[dict[str, Any]] | None = None,
    cruise_speed: float = 15.0,
    hover_speed: float = 5.0,
    firmware_type: int = 12,
    vehicle_type: int = 2,
    global_plan_altitude_mode: int = 1,
    ground_station: str = GROUND_STATION,
    indent: int = 2,
) -> str:
    """Serialize a plan to a formatted JSON string.

    :return: Formatted JSON string representing the QGC .plan file.
    """
    plan_dict = build_plan_dict(
        planned_home_position=planned_home_position,
        items=items,
        cruise_speed=cruise_speed,
        hover_speed=hover_speed,
        firmware_type=firmware_type,
        vehicle_type=vehicle_type,
        global_plan_altitude_mode=global_plan_altitude_mode,
        ground_station=ground_station,
    )
    return json.dumps(plan_dict, indent=indent)


def save_plan_file(
    filepath: str | Path,
    planned_home_position: Sequence[float],
    items: Sequence[dict[str, Any]] | None = None,
    cruise_speed: float = 15.0,
    hover_speed: float = 5.0,
    firmware_type: int = 12,
    vehicle_type: int = 2,
    global_plan_altitude_mode: int = 1,
    ground_station: str = GROUND_STATION,
    indent: int = 2,
) -> None:
    """Serialize a plan and write it to a file.

    :param filepath: Output path for the .plan file.
    """
    json_str = serialize_plan(
        planned_home_position=planned_home_position,
        items=items,
        cruise_speed=cruise_speed,
        hover_speed=hover_speed,
        firmware_type=firmware_type,
        vehicle_type=vehicle_type,
        global_plan_altitude_mode=global_plan_altitude_mode,
        ground_station=ground_station,
        indent=indent,
    )
    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json_str, encoding="utf-8")


def build_camera_calc_dict(
    camera_calc: "CameraCalc | None" = None,
    distance_to_surface: float = 50.0,
    distance_mode: int = 0,
    camera_name: str | None = None,
    adjusted_footprint_side: float = 0.0,
    adjusted_footprint_frontal: float = 0.0,
) -> dict[str, Any]:
    """Build a QGroundControl CameraCalc dictionary (version 2).

    :param camera_calc: Optional CameraCalc instance.
    :param distance_to_surface: Altitude/distance to surface in meters.
    :param distance_mode: Altitude mode (0: Mixed, 1: Relative, 2: Absolute, 3: CalcAboveTerrain, 4: Terrain).
    :param camera_name: Camera name string.
    :param adjusted_footprint_side: Side footprint adjusted for overlap in meters.
    :param adjusted_footprint_frontal: Frontal footprint adjusted for overlap in meters.
    :return: Serialized CameraCalc dictionary (v2).
    """
    if camera_calc is not None:
        dist_surface = float(camera_calc.distance_to_surface)
        adj_side = float(camera_calc.adjusted_footprint_side)
        adj_frontal = float(camera_calc.adjusted_footprint_frontal)
    else:
        dist_surface = float(distance_to_surface)
        adj_side = float(adjusted_footprint_side)
        adj_frontal = float(adjusted_footprint_frontal)

    if camera_name is not None:
        cam_name = str(camera_name)
    elif camera_calc is not None and camera_calc.spec is not None:
        cam_name = str(camera_calc.spec.canonicalName or CUSTOM_CAMERA_NAME)
    else:
        cam_name = MANUAL_CAMERA_NAME

    is_manual = cam_name == MANUAL_CAMERA_NAME

    res: dict[str, Any] = {
        "AdjustedFootprintFrontal": adj_frontal,
        "AdjustedFootprintSide": adj_side,
        "CameraName": cam_name,
        "DistanceMode": int(distance_mode),
        "DistanceToSurface": dist_surface,
        "version": CAMERA_CALC_VERSION,
    }

    if not is_manual and camera_calc is not None and camera_calc.spec is not None:
        spec = camera_calc.spec
        res.update(
            {
                "FixedOrientation": bool(spec.fixedOrientation),
                "FocalLength": float(spec.focalLength),
                "FrontalOverlap": float(camera_calc.frontal_overlap),
                "ImageDensity": float(camera_calc.image_density),
                "ImageHeight": float(spec.imageHeight),
                "ImageWidth": float(spec.imageWidth),
                "Landscape": bool(spec.landscape),
                "MinTriggerInterval": float(spec.minTriggerInterval),
                "SensorHeight": float(spec.sensorHeight),
                "SensorWidth": float(spec.sensorWidth),
                "SideOverlap": float(camera_calc.side_overlap),
                "ValueSetIsDistance": bool(camera_calc.value_set_is_distance),
            }
        )

    return res


def build_transect_style_complex_item_dict(
    camera_calc_dict: dict[str, Any],
    visual_transect_points: Sequence[Sequence[float]],
    items: Sequence[dict[str, Any]],
    turnaround_distance: float = 0.0,
    camera_trigger_in_turnaround: bool = True,
    hover_and_capture: bool = False,
    refly_90_degrees: bool = False,
    camera_shots: int = 0,
    version: int = TRANSECT_STYLE_VERSION,
    terrain_adjust_tolerance: float = 10.0,
    terrain_adjust_max_climb_rate: float = 0.0,
    terrain_adjust_max_descent_rate: float = 0.0,
    terrain_flight_speed: float = 5.0,
) -> dict[str, Any]:
    """Build a QGroundControl TransectStyleComplexItem dictionary (version 2).

    :param camera_calc_dict: CameraCalc dictionary (v2).
    :param visual_transect_points: Sequence of 2D (lat, lon) transect points.
    :param items: Sequence of expanded mission item dicts (SimpleItems).
    :param turnaround_distance: Turnaround distance in meters.
    :param camera_trigger_in_turnaround: Keep camera trigger active during turnarounds.
    :param hover_and_capture: Hover and capture at each waypoint.
    :param refly_90_degrees: Cross-grid 90 degree survey.
    :param camera_shots: Count of camera shots.
    :param version: TransectStyleComplexItem version (default 2).
    :param terrain_adjust_tolerance: Terrain adjust tolerance in vertical meters.
    :param terrain_adjust_max_climb_rate: Maximum climb rate for terrain adjust (m/s).
    :param terrain_adjust_max_descent_rate: Maximum descent rate for terrain adjust (m/s).
    :param terrain_flight_speed: Flight speed when adjusting for terrain (m/s).
    :return: Serialized TransectStyleComplexItem dictionary (v2).
    """
    pts = [[float(pt[0]), float(pt[1])] for pt in visual_transect_points]
    mission_items = list(items)

    res: dict[str, Any] = {
        "CameraCalc": camera_calc_dict,
        "CameraShots": int(camera_shots),
        "CameraTriggerInTurnAround": bool(camera_trigger_in_turnaround),
        "HoverAndCapture": bool(hover_and_capture),
        "Items": mission_items,
        "Refly90Degrees": bool(refly_90_degrees),
        "TurnAroundDistance": float(turnaround_distance),
        "VisualTransectPoints": pts,
        "version": int(version),
    }

    if camera_calc_dict.get("DistanceMode") == 3:
        res["TerrainAdjustTolerance"] = float(terrain_adjust_tolerance)
        res["TerrainAdjustMaxClimbRate"] = float(terrain_adjust_max_climb_rate)
        res["TerrainAdjustMaxDescentRate"] = float(terrain_adjust_max_descent_rate)
        res["TerrainFlightSpeed"] = float(terrain_flight_speed)

    return res


def build_survey_complex_item_dict(
    polygon_points: Sequence[Sequence[float]],
    transect_style_dict: dict[str, Any],
    grid_angle: float = 0.0,
    entry_location: int = 0,
    fly_alternate_transects: bool = False,
    split_concave_polygons: bool = False,
    version: int = SURVEY_VERSION,
) -> dict[str, Any]:
    """Build a QGroundControl Survey ComplexItem dictionary (Survey v5).

    :param polygon_points: Sequence of 2D (lat, lon) polygon vertices.
    :param transect_style_dict: TransectStyleComplexItem dictionary (v2).
    :param grid_angle: Grid angle in degrees.
    :param entry_location: Entry location index (0..3).
    :param fly_alternate_transects: Fly alternate transects.
    :param split_concave_polygons: Split concave polygons.
    :param version: Survey complex item version (default 5).
    :return: Serialized Survey ComplexItem dictionary (v5).
    """
    poly = [[float(pt[0]), float(pt[1])] for pt in polygon_points]

    return {
        "TransectStyleComplexItem": transect_style_dict,
        "angle": float(grid_angle),
        "complexItemType": "survey",
        "entryLocation": int(entry_location),
        "flyAlternateTransects": bool(fly_alternate_transects),
        "polygon": poly,
        "splitConcavePolygons": bool(split_concave_polygons),
        "type": "ComplexItem",
        "version": int(version),
    }


def build_survey_item(
    polygon_points: Sequence[Sequence[float]],
    visual_transect_points: Sequence[Sequence[float]],
    items: Sequence[dict[str, Any]],
    camera_calc: "CameraCalc | None" = None,
    grid_angle: float = 0.0,
    entry_location: int = 0,
    turnaround_distance: float = 0.0,
    camera_trigger_in_turnaround: bool = True,
    hover_and_capture: bool = False,
    refly_90_degrees: bool = False,
    fly_alternate_transects: bool = False,
    split_concave_polygons: bool = False,
    distance_mode: int = 0,
    distance_to_surface: float = 50.0,
    camera_name: str | None = None,
    camera_shots: int = 0,
    terrain_adjust_tolerance: float = 10.0,
    terrain_adjust_max_climb_rate: float = 0.0,
    terrain_adjust_max_descent_rate: float = 0.0,
    terrain_flight_speed: float = 5.0,
) -> dict[str, Any]:
    """Convenience function to build a complete Survey ComplexItem dict (v5)."""
    camera_calc_dict = build_camera_calc_dict(
        camera_calc=camera_calc,
        distance_to_surface=distance_to_surface,
        distance_mode=distance_mode,
        camera_name=camera_name,
    )
    transect_style_dict = build_transect_style_complex_item_dict(
        camera_calc_dict=camera_calc_dict,
        visual_transect_points=visual_transect_points,
        items=items,
        turnaround_distance=turnaround_distance,
        camera_trigger_in_turnaround=camera_trigger_in_turnaround,
        hover_and_capture=hover_and_capture,
        refly_90_degrees=refly_90_degrees,
        camera_shots=camera_shots,
        terrain_adjust_tolerance=terrain_adjust_tolerance,
        terrain_adjust_max_climb_rate=terrain_adjust_max_climb_rate,
        terrain_adjust_max_descent_rate=terrain_adjust_max_descent_rate,
        terrain_flight_speed=terrain_flight_speed,
    )
    return build_survey_complex_item_dict(
        polygon_points=polygon_points,
        transect_style_dict=transect_style_dict,
        grid_angle=grid_angle,
        entry_location=entry_location,
        fly_alternate_transects=fly_alternate_transects,
        split_concave_polygons=split_concave_polygons,
    )
