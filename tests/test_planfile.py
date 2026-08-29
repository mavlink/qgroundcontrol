"""Tests for QGroundControl .plan file serializer in qgc4qgis.core.planfile."""

import json

from qgc4qgis.core.cameracalc import CameraCalc
from qgc4qgis.core.cameras import CameraSpec
from qgc4qgis.core.planfile import (
    CAMERA_CALC_VERSION,
    FILE_TYPE,
    GEOFENCE_VERSION,
    GROUND_STATION,
    MANUAL_CAMERA_NAME,
    MISSION_VERSION,
    PLAN_VERSION,
    RALLY_POINTS_VERSION,
    SURVEY_VERSION,
    TRANSECT_STYLE_VERSION,
    build_camera_calc_dict,
    build_plan_dict,
    build_survey_complex_item_dict,
    build_survey_item,
    build_transect_style_complex_item_dict,
    save_plan_file,
    serialize_plan,
)


def test_build_plan_dict_header_and_defaults():
    """Verify top-level header fields and default values."""
    home = (-23.5505, -46.6333, 750.0)
    plan = build_plan_dict(planned_home_position=home)

    assert plan["fileType"] == FILE_TYPE
    assert plan["version"] == PLAN_VERSION
    assert plan["groundStation"] == GROUND_STATION

    mission = plan["mission"]
    assert mission["version"] == MISSION_VERSION
    assert mission["plannedHomePosition"] == [-23.5505, -46.6333, 750.0]
    assert mission["cruiseSpeed"] == 15.0
    assert mission["hoverSpeed"] == 5.0
    assert mission["firmwareType"] == 12
    assert mission["vehicleType"] == 2
    assert mission["globalPlanAltitudeMode"] == 1
    assert mission["items"] == []

    geo_fence = plan["geoFence"]
    assert geo_fence["version"] == GEOFENCE_VERSION
    assert geo_fence["circles"] == []
    assert geo_fence["polygons"] == []

    rally_points = plan["rallyPoints"]
    assert rally_points["version"] == RALLY_POINTS_VERSION
    assert rally_points["points"] == []


def test_build_plan_dict_custom_parameters_and_items():
    """Verify custom parameters and mission items array."""
    home_2d = (-23.5505, -46.6333)
    custom_items = [
        {"type": "SimpleItem", "command": 22, "params": [15, 0, 0, None, -23.55, -46.63, 50]}
    ]

    plan = build_plan_dict(
        planned_home_position=home_2d,
        items=custom_items,
        cruise_speed=20.0,
        hover_speed=7.5,
        firmware_type=3,  # ArduPilot
        vehicle_type=1,  # Fixed wing
        global_plan_altitude_mode=0,
        ground_station="CustomGS",
    )

    assert plan["groundStation"] == "CustomGS"
    mission = plan["mission"]
    assert mission["plannedHomePosition"] == [-23.5505, -46.6333, 0.0]
    assert mission["cruiseSpeed"] == 20.0
    assert mission["hoverSpeed"] == 7.5
    assert mission["firmwareType"] == 3
    assert mission["vehicleType"] == 1
    assert mission["globalPlanAltitudeMode"] == 0
    assert len(mission["items"]) == 1
    assert mission["items"][0]["type"] == "SimpleItem"


def test_serialize_plan_json_output():
    """Verify serialize_plan produces valid JSON matching dictionary."""
    home = (10.0, 20.0, 30.0)
    json_str = serialize_plan(planned_home_position=home, indent=2)
    parsed = json.loads(json_str)

    assert parsed["fileType"] == "Plan"
    assert parsed["version"] == 1
    assert parsed["mission"]["plannedHomePosition"] == [10.0, 20.0, 30.0]


def test_save_plan_file(tmp_path):
    """Verify save_plan_file writes JSON file to disk."""
    home = (-12.0, -45.0, 100.0)
    file_path = tmp_path / "sub_dir" / "test_mission.plan"

    save_plan_file(filepath=file_path, planned_home_position=home)

    assert file_path.exists()
    content = json.loads(file_path.read_text(encoding="utf-8"))
    assert content["fileType"] == "Plan"
    assert content["mission"]["plannedHomePosition"] == [-12.0, -45.0, 100.0]


def test_build_camera_calc_dict_manual():
    """Verify build_camera_calc_dict for manual camera."""
    calc_dict = build_camera_calc_dict(
        distance_to_surface=60.0,
        distance_mode=0,
        adjusted_footprint_side=30.0,
        adjusted_footprint_frontal=20.0,
    )
    assert calc_dict["version"] == CAMERA_CALC_VERSION
    assert calc_dict["CameraName"] == MANUAL_CAMERA_NAME
    assert calc_dict["DistanceToSurface"] == 60.0
    assert calc_dict["DistanceMode"] == 0
    assert calc_dict["AdjustedFootprintSide"] == 30.0
    assert calc_dict["AdjustedFootprintFrontal"] == 20.0
    assert "SensorWidth" not in calc_dict


def test_build_camera_calc_dict_with_spec():
    """Verify build_camera_calc_dict with a camera specification."""
    spec = CameraSpec(
        canonicalName="TestCamera",
        sensorWidth=23.5,
        sensorHeight=15.6,
        imageWidth=6000,
        imageHeight=4000,
        focalLength=16.0,
        landscape=True,
    )
    cc = CameraCalc(spec=spec, distance_to_surface=100.0)
    assert cc.recalculate() is True

    calc_dict = build_camera_calc_dict(camera_calc=cc)
    assert calc_dict["version"] == CAMERA_CALC_VERSION
    assert calc_dict["CameraName"] == "TestCamera"
    assert calc_dict["DistanceToSurface"] == 100.0
    assert calc_dict["SensorWidth"] == 23.5
    assert calc_dict["SensorHeight"] == 15.6
    assert calc_dict["ImageWidth"] == 6000.0
    assert calc_dict["ImageHeight"] == 4000.0
    assert calc_dict["FocalLength"] == 16.0
    assert calc_dict["Landscape"] is True


def test_build_survey_complex_item_v5():
    """Verify serialization of Survey v5 complex item with TransectStyleComplexItem v2."""
    poly_points = [(-23.55, -46.63), (-23.55, -46.62), (-23.54, -46.62)]
    visual_pts = [(-23.55, -46.63), (-23.54, -46.62)]
    simple_items = [
        {"type": "SimpleItem", "command": 16, "params": [0, 0, 0, None, -23.55, -46.63, 50]}
    ]

    calc_dict = build_camera_calc_dict(distance_to_surface=50.0)
    transect_dict = build_transect_style_complex_item_dict(
        camera_calc_dict=calc_dict,
        visual_transect_points=visual_pts,
        items=simple_items,
        turnaround_distance=10.0,
        camera_shots=5,
    )
    survey_dict = build_survey_complex_item_dict(
        polygon_points=poly_points,
        transect_style_dict=transect_dict,
        grid_angle=45.0,
        entry_location=1,
    )

    assert survey_dict["type"] == "ComplexItem"
    assert survey_dict["complexItemType"] == "survey"
    assert survey_dict["version"] == SURVEY_VERSION
    assert survey_dict["angle"] == 45.0
    assert survey_dict["entryLocation"] == 1
    assert survey_dict["polygon"] == [[-23.55, -46.63], [-23.55, -46.62], [-23.54, -46.62]]

    transect_block = survey_dict["TransectStyleComplexItem"]
    assert transect_block["version"] == TRANSECT_STYLE_VERSION
    assert transect_block["TurnAroundDistance"] == 10.0
    assert transect_block["CameraShots"] == 5
    assert transect_block["VisualTransectPoints"] == [[-23.55, -46.63], [-23.54, -46.62]]
    assert len(transect_block["Items"]) == 1

    cam_calc = transect_block["CameraCalc"]
    assert cam_calc["version"] == CAMERA_CALC_VERSION
    assert cam_calc["CameraName"] == MANUAL_CAMERA_NAME


def test_survey_in_full_plan_serialization():
    """Verify embedding a survey complex item in a complete plan file."""
    poly_points = [(10.0, 20.0), (10.0, 21.0), (11.0, 21.0)]
    survey_dict = build_survey_item(
        polygon_points=poly_points,
        visual_transect_points=[(10.0, 20.0), (11.0, 21.0)],
        items=[],
    )

    plan = build_plan_dict(
        planned_home_position=(10.0, 20.0, 100.0),
        items=[survey_dict],
    )

    json_str = json.dumps(plan, indent=2)
    parsed = json.loads(json_str)

    item = parsed["mission"]["items"][0]
    assert item["type"] == "ComplexItem"
    assert item["complexItemType"] == "survey"
    assert item["version"] == 5
    assert "TransectStyleComplexItem" in item
    assert "CameraCalc" in item["TransectStyleComplexItem"]


def test_plan_mandatory_keys_and_exact_version_numbers():
    """Verify all required keys are present with exact version values across all .plan structures."""
    home = (-23.5505, -46.6333, 750.0)
    survey_item = build_survey_item(
        polygon_points=[(-23.55, -46.63), (-23.55, -46.62), (-23.54, -46.62)],
        visual_transect_points=[(-23.55, -46.63), (-23.54, -46.62)],
        items=[],
    )
    plan = build_plan_dict(planned_home_position=home, items=[survey_item])

    # Top-level mandatory keys and version numbers
    assert set(plan.keys()) == {
        "fileType",
        "geoFence",
        "groundStation",
        "mission",
        "rallyPoints",
        "version",
    }
    assert plan["fileType"] == FILE_TYPE
    assert plan["version"] == PLAN_VERSION
    assert plan["groundStation"] == GROUND_STATION

    # Mission mandatory keys and version
    mission = plan["mission"]
    assert set(mission.keys()) == {
        "cruiseSpeed",
        "firmwareType",
        "globalPlanAltitudeMode",
        "hoverSpeed",
        "items",
        "plannedHomePosition",
        "vehicleType",
        "version",
    }
    assert mission["version"] == MISSION_VERSION

    # GeoFence mandatory keys and version
    geo_fence = plan["geoFence"]
    assert set(geo_fence.keys()) == {"circles", "polygons", "version"}
    assert geo_fence["version"] == GEOFENCE_VERSION

    # RallyPoints mandatory keys and version
    rally_points = plan["rallyPoints"]
    assert set(rally_points.keys()) == {"points", "version"}
    assert rally_points["version"] == RALLY_POINTS_VERSION

    # Survey ComplexItem mandatory keys and version
    survey = mission["items"][0]
    assert set(survey.keys()) == {
        "TransectStyleComplexItem",
        "angle",
        "complexItemType",
        "entryLocation",
        "flyAlternateTransects",
        "polygon",
        "splitConcavePolygons",
        "type",
        "version",
    }
    assert survey["type"] == "ComplexItem"
    assert survey["complexItemType"] == "survey"
    assert survey["version"] == SURVEY_VERSION

    # TransectStyleComplexItem mandatory keys and version
    transect = survey["TransectStyleComplexItem"]
    assert set(transect.keys()) == {
        "CameraCalc",
        "CameraShots",
        "CameraTriggerInTurnAround",
        "HoverAndCapture",
        "Items",
        "Refly90Degrees",
        "TurnAroundDistance",
        "VisualTransectPoints",
        "version",
    }
    assert transect["version"] == TRANSECT_STYLE_VERSION

    # CameraCalc mandatory keys and version
    cam_calc = transect["CameraCalc"]
    assert "version" in cam_calc
    assert cam_calc["version"] == CAMERA_CALC_VERSION


def test_plan_items_params_7_elements():
    """Verify that all SimpleItem mission items have params with exactly 7 elements."""
    wp1 = {
        "autoContinue": True,
        "command": 16,
        "doJumpId": 1,
        "frame": 3,
        "params": [0.0, 0.0, 0.0, None, -23.5505, -46.6333, 100.0],
        "type": "SimpleItem",
    }
    wp2 = {
        "autoContinue": True,
        "command": 206,
        "doJumpId": 2,
        "frame": 2,
        "params": [25.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0],
        "type": "SimpleItem",
    }
    survey_sub_item = {
        "autoContinue": True,
        "command": 16,
        "doJumpId": 3,
        "frame": 3,
        "params": [0.0, 0.0, 0.0, None, -23.54, -46.62, 100.0],
        "type": "SimpleItem",
    }
    survey_item = build_survey_item(
        polygon_points=[(-23.55, -46.63), (-23.55, -46.62), (-23.54, -46.62)],
        visual_transect_points=[(-23.55, -46.63), (-23.54, -46.62)],
        items=[survey_sub_item],
    )

    plan = build_plan_dict(
        planned_home_position=(-23.5505, -46.6333, 750.0),
        items=[wp1, wp2, survey_item],
    )

    for item in plan["mission"]["items"]:
        if item.get("type") == "SimpleItem":
            assert "params" in item
            assert len(item["params"]) == 7
        elif item.get("type") == "ComplexItem" and item.get("complexItemType") == "survey":
            sub_items = item["TransectStyleComplexItem"]["Items"]
            for sub_item in sub_items:
                if sub_item.get("type") == "SimpleItem":
                    assert "params" in sub_item
                    assert len(sub_item["params"]) == 7


def test_plan_roundtrip_generation_read_geometry_and_counts(tmp_path):
    """Verify roundtrip (generate -> save -> read) preserves exact geometry and item counts."""
    home_pos = [-23.5505, -46.6333, 750.0]
    poly_points = [[-23.5500, -46.6300], [-23.5500, -46.6200], [-23.5400, -46.6200]]
    visual_transect_pts = [[-23.5500, -46.6300], [-23.5400, -46.6200]]

    simple_item_1 = {
        "autoContinue": True,
        "command": 16,
        "doJumpId": 1,
        "frame": 3,
        "params": [0.0, 0.0, 0.0, None, -23.5505, -46.6333, 100.0],
        "type": "SimpleItem",
    }
    simple_item_2 = {
        "autoContinue": True,
        "command": 16,
        "doJumpId": 2,
        "frame": 3,
        "params": [0.0, 0.0, 0.0, None, -23.5400, -46.6200, 100.0],
        "type": "SimpleItem",
    }
    survey_item = build_survey_item(
        polygon_points=poly_points,
        visual_transect_points=visual_transect_pts,
        items=[simple_item_2],
    )

    plan_path = tmp_path / "roundtrip.plan"
    save_plan_file(
        filepath=plan_path,
        planned_home_position=home_pos,
        items=[simple_item_1, survey_item],
    )

    assert plan_path.exists()
    content = json.loads(plan_path.read_text(encoding="utf-8"))

    # Verify top-level mandatory keys and version numbers
    assert content["fileType"] == FILE_TYPE
    assert content["version"] == PLAN_VERSION
    assert content["groundStation"] == GROUND_STATION
    assert content["mission"]["version"] == MISSION_VERSION
    assert content["geoFence"]["version"] == GEOFENCE_VERSION
    assert content["rallyPoints"]["version"] == RALLY_POINTS_VERSION

    # Item count check
    items = content["mission"]["items"]
    assert len(items) == 2

    # Geometries check
    assert content["mission"]["plannedHomePosition"] == home_pos

    item_0 = items[0]
    assert item_0["type"] == "SimpleItem"
    assert len(item_0["params"]) == 7
    assert item_0["params"][4] == -23.5505
    assert item_0["params"][5] == -46.6333
    assert item_0["params"][6] == 100.0

    item_1 = items[1]
    assert item_1["type"] == "ComplexItem"
    assert item_1["complexItemType"] == "survey"
    assert item_1["version"] == SURVEY_VERSION
    assert item_1["polygon"] == poly_points
    assert len(item_1["polygon"]) == len(poly_points)

    transect = item_1["TransectStyleComplexItem"]
    assert transect["version"] == TRANSECT_STYLE_VERSION
    assert transect["VisualTransectPoints"] == visual_transect_pts
    assert len(transect["VisualTransectPoints"]) == len(visual_transect_pts)

    sub_items = transect["Items"]
    assert len(sub_items) == 1
    assert sub_items[0]["type"] == "SimpleItem"
    assert len(sub_items[0]["params"]) == 7
    assert sub_items[0]["params"][4] == -23.5400
    assert sub_items[0]["params"][5] == -46.6200
    assert sub_items[0]["params"][6] == 100.0


def test_survey_terrain_mode_emission():
    """Verify DistanceMode=3 (CalcAboveTerrain) emits terrain adjustment keys in TransectStyleComplexItem."""
    poly_points = [[-23.5500, -46.6300], [-23.5500, -46.6200], [-23.5400, -46.6200]]
    visual_pts = [[-23.5500, -46.6300], [-23.5400, -46.6200]]

    survey_item = build_survey_item(
        polygon_points=poly_points,
        visual_transect_points=visual_pts,
        items=[],
        distance_mode=3,
        terrain_adjust_tolerance=12.5,
        terrain_adjust_max_climb_rate=2.0,
        terrain_adjust_max_descent_rate=3.0,
        terrain_flight_speed=8.0,
    )

    transect = survey_item["TransectStyleComplexItem"]
    assert transect["CameraCalc"]["DistanceMode"] == 3
    assert transect["TerrainAdjustTolerance"] == 12.5
    assert transect["TerrainAdjustMaxClimbRate"] == 2.0
    assert transect["TerrainAdjustMaxDescentRate"] == 3.0
    assert transect["TerrainFlightSpeed"] == 8.0
