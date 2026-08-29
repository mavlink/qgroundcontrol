"""Unit tests for WPML exporter module."""

import tempfile
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path

import pytest

from qgc4qgis.core.missionitems import DistanceMode
from qgc4qgis.core.route import Route, RouteWaypoint
from qgc4qgis.core.wpml import (
    KML_NS,
    WPML_NS,
    build_mission_config,
    build_template_kml,
    build_waylines_wpml,
    save_kmz,
    save_template_kml,
    save_waylines_wpml,
    serialize_template_kml,
    serialize_waylines_wpml,
    validate_wpml_route,
)


def test_build_mission_config_defaults():
    mc = build_mission_config()
    assert mc.tag == f"{{{WPML_NS}}}missionConfig"

    children_tags = [c.tag.replace(f"{{{WPML_NS}}}", "") for c in mc]
    assert children_tags == [
        "flyToWaylineMode",
        "finishAction",
        "exitOnRCLost",
        "executeRCLostAction",
        "takeOffSecurityHeight",
        "takeOffRefPointAGLHeight",
        "globalTransitionalSpeed",
        "droneInfo",
        "payloadInfo",
    ]

    assert mc.find(f"{{{WPML_NS}}}takeOffRefPoint") is None

    fly_mode = mc.find(f"{{{WPML_NS}}}flyToWaylineMode")
    assert fly_mode is not None and fly_mode.text == "safely"

    finish = mc.find(f"{{{WPML_NS}}}finishAction")
    assert finish is not None and finish.text == "goHome"

    exit_rc = mc.find(f"{{{WPML_NS}}}exitOnRCLost")
    assert exit_rc is not None and exit_rc.text == "executeLostAction"

    exec_rc = mc.find(f"{{{WPML_NS}}}executeRCLostAction")
    assert exec_rc is not None and exec_rc.text == "goBack"

    sec_height = mc.find(f"{{{WPML_NS}}}takeOffSecurityHeight")
    assert sec_height is not None and sec_height.text == "20"

    agl_height = mc.find(f"{{{WPML_NS}}}takeOffRefPointAGLHeight")
    assert agl_height is not None and agl_height.text == "0"

    speed = mc.find(f"{{{WPML_NS}}}globalTransitionalSpeed")
    assert speed is not None and speed.text == "5.0"

    drone_info = mc.find(f"{{{WPML_NS}}}droneInfo")
    assert drone_info is not None
    assert drone_info.find(f"{{{WPML_NS}}}droneEnumValue").text == "68"
    assert drone_info.find(f"{{{WPML_NS}}}droneSubEnumValue").text == "0"

    payload_info = mc.find(f"{{{WPML_NS}}}payloadInfo")
    assert payload_info is not None
    assert payload_info.find(f"{{{WPML_NS}}}payloadEnumValue").text == "0"
    assert payload_info.find(f"{{{WPML_NS}}}payloadSubEnumValue").text == "0"
    assert payload_info.find(f"{{{WPML_NS}}}payloadPositionIndex").text == "0"


def test_build_mission_config_custom():
    mc = build_mission_config(
        fly_to_wayline_mode="pointToPoint",
        finish_action="autoLand",
        exit_on_rc_lost="goBack",
        execute_rc_lost_action="landing",
        global_transitional_speed=10.0,
        drone_enum_value=77,
        drone_sub_enum_value=1,
    )
    assert mc.find(f"{{{WPML_NS}}}flyToWaylineMode").text == "pointToPoint"
    assert mc.find(f"{{{WPML_NS}}}finishAction").text == "autoLand"
    assert mc.find(f"{{{WPML_NS}}}exitOnRCLost").text == "goBack"
    assert mc.find(f"{{{WPML_NS}}}executeRCLostAction").text == "landing"
    assert mc.find(f"{{{WPML_NS}}}globalTransitionalSpeed").text == "10.0"
    drone_info = mc.find(f"{{{WPML_NS}}}droneInfo")
    assert drone_info.find(f"{{{WPML_NS}}}droneEnumValue").text == "77"
    assert drone_info.find(f"{{{WPML_NS}}}droneSubEnumValue").text == "1"


def test_build_mission_config_takeoff_ref_point_and_order():
    mc = build_mission_config(
        takeoff_security_height=30.0,
        takeoff_ref_point=(-23.5505, -46.6333, 100.0),
        takeoff_ref_point_agl_height=5.0,
        payload_enum_value=1,
        payload_sub_enum_value=2,
        payload_position_index=3,
    )
    children_tags = [c.tag.replace(f"{{{WPML_NS}}}", "") for c in mc]
    assert children_tags == [
        "flyToWaylineMode",
        "finishAction",
        "exitOnRCLost",
        "executeRCLostAction",
        "takeOffSecurityHeight",
        "takeOffRefPoint",
        "takeOffRefPointAGLHeight",
        "globalTransitionalSpeed",
        "droneInfo",
        "payloadInfo",
    ]

    ref_elem = mc.find(f"{{{WPML_NS}}}takeOffRefPoint")
    assert ref_elem is not None
    assert ref_elem.text == "-23.5505,-46.6333,100"

    sec_elem = mc.find(f"{{{WPML_NS}}}takeOffSecurityHeight")
    assert sec_elem is not None and sec_elem.text == "30"

    agl_elem = mc.find(f"{{{WPML_NS}}}takeOffRefPointAGLHeight")
    assert agl_elem is not None and agl_elem.text == "5"

    payload_info = mc.find(f"{{{WPML_NS}}}payloadInfo")
    assert payload_info is not None
    assert payload_info.find(f"{{{WPML_NS}}}payloadEnumValue").text == "1"
    assert payload_info.find(f"{{{WPML_NS}}}payloadSubEnumValue").text == "2"
    assert payload_info.find(f"{{{WPML_NS}}}payloadPositionIndex").text == "3"


def test_build_template_kml_structure():
    kml = build_template_kml(
        author="TestAuthor",
        create_time=1700000000000,
        update_time=1700000005000,
    )
    assert kml.tag == f"{{{KML_NS}}}kml"
    doc = kml.find(f"{{{KML_NS}}}Document")
    assert doc is not None

    author = doc.find(f"{{{WPML_NS}}}author")
    assert author is not None and author.text == "TestAuthor"

    c_time = doc.find(f"{{{WPML_NS}}}createTime")
    assert c_time is not None and c_time.text == "1700000000000"

    u_time = doc.find(f"{{{WPML_NS}}}updateTime")
    assert u_time is not None and u_time.text == "1700000005000"

    mc = doc.find(f"{{{WPML_NS}}}missionConfig")
    assert mc is not None


def test_serialize_template_kml():
    xml_str = serialize_template_kml(
        author="XMLAuthor",
        create_time=1600000000000,
        update_time=1600000000000,
    )
    assert 'xmlns="http://www.opengis.net/kml/2.2"' in xml_str
    assert 'xmlns:wpml="http://www.dji.com/wpmz/1.0.2"' in xml_str
    assert "<wpml:author>XMLAuthor</wpml:author>" in xml_str
    assert "<wpml:createTime>1600000000000</wpml:createTime>" in xml_str
    assert "<wpml:updateTime>1600000000000</wpml:updateTime>" in xml_str
    assert "<wpml:flyToWaylineMode>safely</wpml:flyToWaylineMode>" in xml_str
    assert "<wpml:droneEnumValue>68</wpml:droneEnumValue>" in xml_str

    # Parse serialized XML string to ensure valid XML
    root = ET.fromstring(xml_str)
    assert root.tag == f"{{{KML_NS}}}kml"


def test_save_template_kml():
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "template.kml"
        save_template_kml(
            filepath,
            author="SaveAuthor",
            create_time=1500000000000,
            update_time=1500000000000,
        )
        assert filepath.exists()
        content = filepath.read_text(encoding="utf-8")
        assert "<wpml:author>SaveAuthor</wpml:author>" in content


def test_build_waylines_wpml_structure():
    waypoints = [
        {"lon": -46.6333, "lat": -23.5505, "executeHeight": 30.0, "waypointSpeed": 4.5},
        {"lon": -46.6334, "lat": -23.5506, "executeHeight": 35.0, "waypointSpeed": 5.0},
    ]
    kml = build_waylines_wpml(
        waypoints=waypoints,
        template_id=0,
        wayline_id=0,
        distance=0,
        duration=0,
        execute_height_mode="relativeToStartPoint",
        auto_flight_speed=5.0,
    )
    assert kml.tag == f"{{{KML_NS}}}kml"
    doc = kml.find(f"{{{KML_NS}}}Document")
    assert doc is not None

    mc = doc.find(f"{{{WPML_NS}}}missionConfig")
    assert mc is not None

    folder = doc.find(f"{{{KML_NS}}}Folder")
    assert folder is not None

    assert folder.find(f"{{{WPML_NS}}}templateId").text == "0"
    assert folder.find(f"{{{WPML_NS}}}executeHeightMode").text == "relativeToStartPoint"
    assert folder.find(f"{{{WPML_NS}}}waylineId").text == "0"
    assert folder.find(f"{{{WPML_NS}}}distance").text == "0"
    assert folder.find(f"{{{WPML_NS}}}duration").text == "0"
    assert folder.find(f"{{{WPML_NS}}}autoFlightSpeed").text == "5.0"

    placemarks = folder.findall(f"{{{KML_NS}}}Placemark")
    assert len(placemarks) == 2

    # Check first Placemark
    pm0 = placemarks[0]
    coords0 = pm0.find(f"{{{KML_NS}}}Point/{f'{{{KML_NS}}}coordinates'}")
    assert coords0 is not None and coords0.text == "-46.6333,-23.5505"
    assert pm0.find(f"{{{WPML_NS}}}index").text == "0"
    assert pm0.find(f"{{{WPML_NS}}}executeHeight").text == "30.0"
    assert pm0.find(f"{{{WPML_NS}}}waypointSpeed").text == "4.5"

    heading_param0 = pm0.find(f"{{{WPML_NS}}}waypointHeadingParam")
    assert heading_param0 is not None
    assert heading_param0.find(f"{{{WPML_NS}}}waypointHeadingMode").text == "followWayline"
    assert heading_param0.find(f"{{{WPML_NS}}}waypointHeadingPathMode").text == "followBadArc"

    turn_param0 = pm0.find(f"{{{WPML_NS}}}waypointTurnParam")
    assert turn_param0 is not None
    assert (
        turn_param0.find(f"{{{WPML_NS}}}waypointTurnMode").text
        == "toPointAndStopWithDiscontinuityCurvature"
    )
    assert turn_param0.find(f"{{{WPML_NS}}}waypointTurnDampingDist").text == "0"

    assert pm0.find(f"{{{WPML_NS}}}useStraightLine").text == "1"

    # Check second Placemark index
    pm1 = placemarks[1]
    assert pm1.find(f"{{{WPML_NS}}}index").text == "1"


def test_serialize_waylines_wpml():
    waypoints = [(-46.6, -23.5, 40.0, 6.0)]
    xml_str = serialize_waylines_wpml(waypoints=waypoints)
    assert "<wpml:templateId>0</wpml:templateId>" in xml_str
    assert "<wpml:executeHeightMode>relativeToStartPoint</wpml:executeHeightMode>" in xml_str
    assert "<coordinates>-46.6,-23.5</coordinates>" in xml_str
    assert "<wpml:index>0</wpml:index>" in xml_str
    assert "<wpml:executeHeight>40.0</wpml:executeHeight>" in xml_str
    assert "<wpml:waypointSpeed>6.0</wpml:waypointSpeed>" in xml_str
    assert "<wpml:waypointHeadingMode>followWayline</wpml:waypointHeadingMode>" in xml_str
    assert "<wpml:waypointHeadingPathMode>followBadArc</wpml:waypointHeadingPathMode>" in xml_str
    assert (
        "<wpml:waypointTurnMode>toPointAndStopWithDiscontinuityCurvature</wpml:waypointTurnMode>"
        in xml_str
    )
    assert "<wpml:waypointTurnDampingDist>0</wpml:waypointTurnDampingDist>" in xml_str
    assert "<wpml:useStraightLine>1</wpml:useStraightLine>" in xml_str


def test_save_waylines_wpml():
    waypoints = [(-46.6, -23.5)]
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "waylines.wpml"
        save_waylines_wpml(filepath, waypoints=waypoints)
        assert filepath.exists()
        content = filepath.read_text(encoding="utf-8")
        assert "<wpml:useStraightLine>1</wpml:useStraightLine>" in content


def test_action_group_por_distancia():
    waypoints = [
        {
            "lon": -46.633,
            "lat": -23.550,
            "executeHeight": 50.0,
            "wait_time": 3.0,
            "gimbal_pitch": -45.0,
        },
        {"lon": -46.634, "lat": -23.551, "executeHeight": 50.0, "wait_time": 0.0},
    ]
    kml = build_waylines_wpml(
        waypoints=waypoints,
        trigger_mode="POR_DISTANCIA",
        trigger_distance=15.0,
    )
    folder = kml.find(f"{{{KML_NS}}}Document/{f'{{{KML_NS}}}Folder'}")
    assert folder is not None

    action_groups = folder.findall(f"{{{WPML_NS}}}actionGroup")
    assert (
        len(action_groups) == 2
    )  # ag0 (reachPoint at wp0 with gimbal & hover), ag1 (multipleDistance)

    # ag0: reachPoint at wp 0 (startIndex==endIndex==0)
    ag0 = action_groups[0]
    assert ag0.find(f"{{{WPML_NS}}}actionGroupId").text == "0"
    assert ag0.find(f"{{{WPML_NS}}}actionGroupStartIndex").text == "0"
    assert ag0.find(f"{{{WPML_NS}}}actionGroupEndIndex").text == "0"
    assert (
        ag0.find(f"{{{WPML_NS}}}actionTrigger/{f'{{{WPML_NS}}}actionTriggerType'}").text
        == "reachPoint"
    )

    actions0 = ag0.findall(f"{{{WPML_NS}}}action")
    assert len(actions0) == 2
    # Action 0: gimbalEvenlyRotate
    assert actions0[0].find(f"{{{WPML_NS}}}actionId").text == "0"
    assert actions0[0].find(f"{{{WPML_NS}}}actionActuatorFunc").text == "gimbalEvenlyRotate"
    assert (
        actions0[0]
        .find(f"{{{WPML_NS}}}actionActuatorFuncParam/{f'{{{WPML_NS}}}gimbalPitchRotateAngle'}")
        .text
        == "-45"
    )
    # Action 1: hover
    assert actions0[1].find(f"{{{WPML_NS}}}actionId").text == "1"
    assert actions0[1].find(f"{{{WPML_NS}}}actionActuatorFunc").text == "hover"
    assert (
        actions0[1].find(f"{{{WPML_NS}}}actionActuatorFuncParam/{f'{{{WPML_NS}}}hoverTime'}").text
        == "3"
    )

    # ag1: multipleDistance
    ag1 = action_groups[1]
    assert ag1.find(f"{{{WPML_NS}}}actionGroupId").text == "1"
    assert ag1.find(f"{{{WPML_NS}}}actionGroupStartIndex").text == "0"
    assert ag1.find(f"{{{WPML_NS}}}actionGroupEndIndex").text == "1"
    assert (
        ag1.find(f"{{{WPML_NS}}}actionTrigger/{f'{{{WPML_NS}}}actionTriggerType'}").text
        == "multipleDistance"
    )
    assert ag1.find(f"{{{WPML_NS}}}actionTrigger/{f'{{{WPML_NS}}}actionTriggerParam'}").text == "15"

    actions1 = ag1.findall(f"{{{WPML_NS}}}action")
    assert len(actions1) == 1
    assert actions1[0].find(f"{{{WPML_NS}}}actionId").text == "0"
    assert actions1[0].find(f"{{{WPML_NS}}}actionActuatorFunc").text == "takePhoto"


def test_action_group_por_foto():
    waypoints = [
        {"lon": -46.633, "lat": -23.550},
        {"lon": -46.634, "lat": -23.551},
    ]
    kml = build_waylines_wpml(
        waypoints=waypoints,
        trigger_mode="POR_FOTO",
    )
    folder = kml.find(f"{{{KML_NS}}}Document/{f'{{{KML_NS}}}Folder'}")
    assert folder is not None

    action_groups = folder.findall(f"{{{WPML_NS}}}actionGroup")
    assert len(action_groups) == 2  # ag0 at wp0 (gimbal + takePhoto), ag1 at wp1 (takePhoto)

    ag0 = action_groups[0]
    assert ag0.find(f"{{{WPML_NS}}}actionGroupId").text == "0"
    assert ag0.find(f"{{{WPML_NS}}}actionGroupStartIndex").text == "0"
    assert ag0.find(f"{{{WPML_NS}}}actionGroupEndIndex").text == "0"
    actions0 = ag0.findall(f"{{{WPML_NS}}}action")
    assert len(actions0) == 2
    assert actions0[0].find(f"{{{WPML_NS}}}actionActuatorFunc").text == "gimbalEvenlyRotate"
    assert actions0[1].find(f"{{{WPML_NS}}}actionActuatorFunc").text == "takePhoto"

    ag1 = action_groups[1]
    assert ag1.find(f"{{{WPML_NS}}}actionGroupId").text == "1"
    assert ag1.find(f"{{{WPML_NS}}}actionGroupStartIndex").text == "1"
    assert ag1.find(f"{{{WPML_NS}}}actionGroupEndIndex").text == "1"
    actions1 = ag1.findall(f"{{{WPML_NS}}}action")
    assert len(actions1) == 1
    assert actions1[0].find(f"{{{WPML_NS}}}actionActuatorFunc").text == "takePhoto"


def test_save_kmz_wpmz_dir_default():
    waypoints = [(-46.633, -23.550, 50.0, 5.0)]
    with tempfile.TemporaryDirectory() as tmpdir:
        kmz_path = Path(tmpdir) / "mission.kmz"
        warnings = save_kmz(kmz_path, waypoints=waypoints)
        assert kmz_path.exists()
        assert warnings == []

        with zipfile.ZipFile(kmz_path, "r") as zf:
            namelist = zf.namelist()
            assert "wpmz/template.kml" in namelist
            assert "wpmz/waylines.wpml" in namelist
            template_content = zf.read("wpmz/template.kml").decode("utf-8")
            assert "<wpml:author>QGC4QGIS</wpml:author>" in template_content


def test_save_kmz_root_dir():
    waypoints = [(-46.633, -23.550, 50.0, 5.0)]
    with tempfile.TemporaryDirectory() as tmpdir:
        kmz_path = Path(tmpdir) / "mission_root.kmz"
        warnings = save_kmz(kmz_path, waypoints=waypoints, in_wpmz_dir=False)
        assert kmz_path.exists()
        assert warnings == []

        with zipfile.ZipFile(kmz_path, "r") as zf:
            namelist = zf.namelist()
            assert "template.kml" in namelist
            assert "waylines.wpml" in namelist
            assert "wpmz/template.kml" not in namelist


def test_save_kmz_terrain_mode_refusal():
    route_terrain = Route(
        waypoints=[RouteWaypoint(lat=-23.55, lon=-46.63, altura=50.0)],
        modo_altitude=DistanceMode.TERRAIN,
    )
    msg_regex = r"rebase_route_to_takeoff\(\) before exporting \(D10\)"
    with tempfile.TemporaryDirectory() as tmpdir:
        kmz_path = Path(tmpdir) / "terrain.kmz"
        with pytest.raises(ValueError, match=msg_regex):
            save_kmz(kmz_path, route=route_terrain)

    with pytest.raises(ValueError, match=msg_regex):
        validate_wpml_route(distance_mode=DistanceMode.TERRAIN)

    with pytest.raises(ValueError, match=msg_regex):
        validate_wpml_route(execute_height_mode="realTimeFollowSurface")


def test_validate_wpml_route_le_zero_height_warning():
    route_le_zero = Route(
        waypoints=[
            RouteWaypoint(lat=-23.55, lon=-46.63, altura=0.0),
            RouteWaypoint(lat=-23.56, lon=-46.64, altura=-5.0),
            RouteWaypoint(lat=-23.57, lon=-46.65, altura=10.0),
        ],
        modo_altitude=DistanceMode.RELATIVE,
    )
    warnings = validate_wpml_route(route=route_le_zero)
    assert len(warnings) == 1
    assert warnings[0] == (
        "Relative height ≤ 0 in 2 waypoint(s): the route goes below the takeoff point."
    )


def test_save_kmz_waypoint_limit_warning():
    waypoints = [(-46.633, -23.550, 50.0, 5.0)] * 10
    with tempfile.TemporaryDirectory() as tmpdir:
        kmz_path = Path(tmpdir) / "limit.kmz"
        warnings = save_kmz(kmz_path, waypoints=waypoints, max_waypoints=5)
        assert len(warnings) == 1
        assert "exceeds the" in warnings[0]


def test_wpml_kmz_xml_validation():
    waypoints = [
        {"lon": -46.6333, "lat": -23.5505, "executeHeight": 30.0, "gimbal_pitch": -45.0},
        {"lon": -46.6334, "lat": -23.5506, "executeHeight": 35.0},
        {"lon": -46.6335, "lat": -23.5507, "executeHeight": 40.0},
    ]

    with tempfile.TemporaryDirectory() as tmpdir:
        # Layout 1: wpmz directory (in_wpmz_dir=True)
        kmz_path_wpmz = Path(tmpdir) / "mission_wpmz.kmz"
        save_kmz(kmz_path_wpmz, waypoints=waypoints, in_wpmz_dir=True)

        with zipfile.ZipFile(kmz_path_wpmz, "r") as zf:
            namelist_wpmz = zf.namelist()
            assert set(namelist_wpmz) == {"wpmz/template.kml", "wpmz/waylines.wpml"}
            template_bytes = zf.read("wpmz/template.kml")
            waylines_bytes = zf.read("wpmz/waylines.wpml")

        # Layout 2: root directory (in_wpmz_dir=False)
        kmz_path_root = Path(tmpdir) / "mission_root.kmz"
        save_kmz(kmz_path_root, waypoints=waypoints, in_wpmz_dir=False)

        with zipfile.ZipFile(kmz_path_root, "r") as zf:
            namelist_root = zf.namelist()
            assert set(namelist_root) == {"template.kml", "waylines.wpml"}

        # Reparse XML and check namespaces
        t_root = ET.fromstring(template_bytes)
        w_root = ET.fromstring(waylines_bytes)

        assert t_root.tag == f"{{{KML_NS}}}kml"
        assert w_root.tag == f"{{{KML_NS}}}kml"

        t_doc = t_root.find(f"{{{KML_NS}}}Document")
        assert t_doc is not None
        assert t_doc.find(f"{{{WPML_NS}}}author") is not None
        assert t_doc.find(f"{{{WPML_NS}}}missionConfig") is not None

        w_doc = w_root.find(f"{{{KML_NS}}}Document")
        assert w_doc is not None
        w_folder = w_doc.find(f"{{{KML_NS}}}Folder")
        assert w_folder is not None

        # Check distance and duration present and zeroed
        dist_elem = w_folder.find(f"{{{WPML_NS}}}distance")
        dur_elem = w_folder.find(f"{{{WPML_NS}}}duration")
        assert dist_elem is not None and dist_elem.text == "0"
        assert dur_elem is not None and dur_elem.text == "0"

        # Check Placemarks: index from 0 to n-1 without gaps, coordinates lon,lat
        placemarks = w_folder.findall(f"{{{KML_NS}}}Placemark")
        assert len(placemarks) == len(waypoints)

        indices = []
        for i, pm in enumerate(placemarks):
            idx_elem = pm.find(f"{{{WPML_NS}}}index")
            assert idx_elem is not None
            indices.append(int(idx_elem.text))

            coords_elem = pm.find(f"{{{KML_NS}}}Point/{f'{{{KML_NS}}}coordinates'}")
            assert coords_elem is not None
            coords_text = coords_elem.text
            lon_str, lat_str = coords_text.split(",")
            assert float(lon_str) == pytest.approx(waypoints[i]["lon"])
            assert float(lat_str) == pytest.approx(waypoints[i]["lat"])

        assert indices == list(range(len(waypoints)))

        # Check single gimbalEvenlyRotate across all action groups
        gimbal_actions = []
        for ag in w_folder.findall(f"{{{WPML_NS}}}actionGroup"):
            for act in ag.findall(f"{{{WPML_NS}}}action"):
                func_elem = act.find(f"{{{WPML_NS}}}actionActuatorFunc")
                if func_elem is not None and func_elem.text == "gimbalEvenlyRotate":
                    gimbal_actions.append(act)

        assert len(gimbal_actions) == 1


def test_template_kml_waypoint_heights_equal_waylines_execute_heights():
    waypoints = [
        {"lon": -46.633, "lat": -23.550, "executeHeight": 41.0},
        {"lon": -46.634, "lat": -23.551, "executeHeight": 52.0},
        {"lon": -46.635, "lat": -23.552, "executeHeight": 63.0},
    ]
    with tempfile.TemporaryDirectory() as tmpdir:
        kmz_path = Path(tmpdir) / "heights_test.kmz"
        save_kmz(kmz_path, waypoints=waypoints)
        assert kmz_path.exists()

        with zipfile.ZipFile(kmz_path, "r") as zf:
            template_bytes = zf.read("wpmz/template.kml")
            waylines_bytes = zf.read("wpmz/waylines.wpml")

        t_root = ET.fromstring(template_bytes)
        w_root = ET.fromstring(waylines_bytes)

        t_placemarks = t_root.findall(
            f"{{{KML_NS}}}Document/{{{KML_NS}}}Folder/{{{KML_NS}}}Placemark"
        )
        w_placemarks = w_root.findall(
            f"{{{KML_NS}}}Document/{{{KML_NS}}}Folder/{{{KML_NS}}}Placemark"
        )

        assert len(t_placemarks) == 3
        assert len(w_placemarks) == 3

        t_heights = [pm.find(f"{{{WPML_NS}}}height").text for pm in t_placemarks]
        w_heights = [pm.find(f"{{{WPML_NS}}}executeHeight").text for pm in w_placemarks]

        assert t_heights == ["41.0", "52.0", "63.0"]
        assert w_heights == ["41.0", "52.0", "63.0"]
        assert t_heights == w_heights


def test_template_kml_template_type_and_height_mode():
    waypoints = [(-46.633, -23.550, 50.0)]
    kml = build_template_kml(waypoints=waypoints, execute_height_mode="relativeToStartPoint")
    folder = kml.find(f"{{{KML_NS}}}Document/{{{KML_NS}}}Folder")
    assert folder is not None

    template_type = folder.find(f"{{{WPML_NS}}}templateType")
    assert template_type is not None and template_type.text == "waypoint"

    sys_param = folder.find(f"{{{WPML_NS}}}waylineCoordinateSysParam")
    assert sys_param is not None
    height_mode = sys_param.find(f"{{{WPML_NS}}}heightMode")
    assert height_mode is not None and height_mode.text == "relativeToStartPoint"
