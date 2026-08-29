"""DJI WPML (Wayline Processing Markup Language) exporter for QGC4QGIS.

Generates WPML XML template files (template.kml) conforming to DJI D9 WPML v1.0.2 specifications:
- KML namespace: http://www.opengis.net/kml/2.2
- WPML namespace: http://www.dji.com/wpmz/1.0.2
- wpml:missionConfig containing flyToWaylineMode, finishAction, exitOnRCLost,
  executeRCLostAction, globalTransitionalSpeed, and droneInfo (droneEnumValue=68)
- template.kml structure with author, createTime, and updateTime in milliseconds.
"""

import time
import zipfile
from collections.abc import Sequence
from pathlib import Path
from typing import Any

from qgc4qgis.core import xmlwrite as ET
from qgc4qgis.core.i18n import tr
from qgc4qgis.core.missionitems import DistanceMode

KML_NS = "http://www.opengis.net/kml/2.2"
WPML_NS = "http://www.dji.com/wpmz/1.0.2"
DEFAULT_DRONE_ENUM_VALUE = 68
# DJI Fly waypoint ceiling is not officially documented; community-reported value is 200.
MAX_WPML_WAYPOINTS = 200

# Register namespaces so ET outputs correct element prefixes.
ET.register_namespace("", KML_NS)
ET.register_namespace("wpml", WPML_NS)


def build_mission_config(
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
) -> ET.Element:
    """Build a wpml:missionConfig XML element for DJI WPML (D9 schema).

    :param fly_to_wayline_mode: Wayline approach mode (default "safely").
    :param finish_action: Action upon mission completion (default "goHome").
    :param exit_on_rc_lost: Behavior when RC signal is lost (default "executeLostAction").
    :param execute_rc_lost_action: Action executed when RC is lost (default "goBack").
    :param takeoff_security_height: Security takeoff height (default 20.0).
    :param takeoff_ref_point: Optional (lat, lon, alt) takeoff reference point tuple.
    :param takeoff_ref_point_agl_height: Takeoff ref point AGL height (default 0.0).
    :param global_transitional_speed: Global transitional speed in m/s (default 5.0).
    :param drone_enum_value: DJI drone model enum value (default 68 for D9).
    :param drone_sub_enum_value: DJI drone sub-model enum value (default 0).
    :param payload_enum_value: DJI payload model enum value (default 0).
    :param payload_sub_enum_value: DJI payload sub-model enum value (default 0).
    :param payload_position_index: DJI payload position index (default 0).
    :return: xml.etree.ElementTree.Element representing wpml:missionConfig.
    """
    mission_config = ET.Element(f"{{{WPML_NS}}}missionConfig")

    fly_mode_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}flyToWaylineMode")
    fly_mode_elem.text = str(fly_to_wayline_mode)

    finish_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}finishAction")
    finish_elem.text = str(finish_action)

    exit_rc_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}exitOnRCLost")
    exit_rc_elem.text = str(exit_on_rc_lost)

    exec_rc_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}executeRCLostAction")
    exec_rc_elem.text = str(execute_rc_lost_action)

    sec_height_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}takeOffSecurityHeight")
    sec_height_elem.text = _format_number(takeoff_security_height)

    if takeoff_ref_point is not None:
        lat, lon, alt = takeoff_ref_point
        ref_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}takeOffRefPoint")
        ref_elem.text = f"{_format_number(lat)},{_format_number(lon)},{_format_number(alt)}"

    agl_height_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}takeOffRefPointAGLHeight")
    agl_height_elem.text = _format_number(takeoff_ref_point_agl_height)

    speed_elem = ET.SubElement(mission_config, f"{{{WPML_NS}}}globalTransitionalSpeed")
    speed_elem.text = str(global_transitional_speed)

    drone_info = ET.SubElement(mission_config, f"{{{WPML_NS}}}droneInfo")

    drone_enum = ET.SubElement(drone_info, f"{{{WPML_NS}}}droneEnumValue")
    drone_enum.text = str(int(drone_enum_value))

    drone_sub_enum = ET.SubElement(drone_info, f"{{{WPML_NS}}}droneSubEnumValue")
    drone_sub_enum.text = str(int(drone_sub_enum_value))

    payload_info = ET.SubElement(mission_config, f"{{{WPML_NS}}}payloadInfo")

    payload_enum = ET.SubElement(payload_info, f"{{{WPML_NS}}}payloadEnumValue")
    payload_enum.text = str(int(payload_enum_value))

    payload_sub_enum = ET.SubElement(payload_info, f"{{{WPML_NS}}}payloadSubEnumValue")
    payload_sub_enum.text = str(int(payload_sub_enum_value))

    payload_pos = ET.SubElement(payload_info, f"{{{WPML_NS}}}payloadPositionIndex")
    payload_pos.text = str(int(payload_position_index))

    return mission_config


def build_template_kml(
    waypoints: Sequence[Any] | Any | None = None,
    author: str = "QGC4QGIS",
    create_time: int | None = None,
    update_time: int | None = None,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    template_id: int = 0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    heading_mode: str = "followWayline",
    gimbal_pitch_mode: str = "manual",
    use_straight_line: int = 1,
) -> ET.Element:
    """Build a complete template.kml XML root element for DJI WPML.

    :param author: Mission author name.
    :param create_time: Creation timestamp in milliseconds (defaults to current time).
    :param update_time: Update timestamp in milliseconds (defaults to create_time).
    :param mission_config: Optional pre-built wpml:missionConfig Element.
    :param fly_to_wayline_mode: Mode to fly to wayline.
    :param finish_action: Action upon mission completion.
    :param exit_on_rc_lost: Behavior when RC lost.
    :param execute_rc_lost_action: Specific action on RC lost.
    :param takeoff_security_height: Security takeoff height.
    :param takeoff_ref_point: Takeoff reference point (lat, lon, alt).
    :param takeoff_ref_point_agl_height: Takeoff reference point AGL height.
    :param global_transitional_speed: Speed in m/s.
    :param drone_enum_value: DJI drone enum value (default 68).
    :param drone_sub_enum_value: DJI drone sub enum value (default 0).
    :param payload_enum_value: DJI payload enum value (default 0).
    :param payload_sub_enum_value: DJI payload sub enum value (default 0).
    :param payload_position_index: DJI payload position index (default 0).
    :return: xml.etree.ElementTree.Element representing root <kml>.
    """
    now_ms = int(time.time() * 1000)
    c_time = int(create_time) if create_time is not None else now_ms
    u_time = int(update_time) if update_time is not None else c_time

    kml_root = ET.Element(f"{{{KML_NS}}}kml")
    doc = ET.SubElement(kml_root, f"{{{KML_NS}}}Document")

    author_elem = ET.SubElement(doc, f"{{{WPML_NS}}}author")
    author_elem.text = str(author)

    create_elem = ET.SubElement(doc, f"{{{WPML_NS}}}createTime")
    create_elem.text = str(c_time)

    update_elem = ET.SubElement(doc, f"{{{WPML_NS}}}updateTime")
    update_elem.text = str(u_time)

    if mission_config is None:
        mission_config = build_mission_config(
            fly_to_wayline_mode=fly_to_wayline_mode,
            finish_action=finish_action,
            exit_on_rc_lost=exit_on_rc_lost,
            execute_rc_lost_action=execute_rc_lost_action,
            takeoff_security_height=takeoff_security_height,
            takeoff_ref_point=takeoff_ref_point,
            takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
            global_transitional_speed=global_transitional_speed,
            drone_enum_value=drone_enum_value,
            drone_sub_enum_value=drone_sub_enum_value,
            payload_enum_value=payload_enum_value,
            payload_sub_enum_value=payload_sub_enum_value,
            payload_position_index=payload_position_index,
        )

    doc.append(mission_config)

    folder = ET.SubElement(doc, f"{{{KML_NS}}}Folder")

    template_type_elem = ET.SubElement(folder, f"{{{WPML_NS}}}templateType")
    template_type_elem.text = "waypoint"

    template_id_elem = ET.SubElement(folder, f"{{{WPML_NS}}}templateId")
    template_id_elem.text = str(int(template_id))

    sys_param = ET.SubElement(folder, f"{{{WPML_NS}}}waylineCoordinateSysParam")
    coord_mode = ET.SubElement(sys_param, f"{{{WPML_NS}}}coordinateMode")
    coord_mode.text = "WGS84"
    h_mode = ET.SubElement(sys_param, f"{{{WPML_NS}}}heightMode")
    h_mode.text = str(execute_height_mode)
    pos_type = ET.SubElement(sys_param, f"{{{WPML_NS}}}positioningType")
    pos_type.text = "GPS"

    auto_speed = ET.SubElement(folder, f"{{{WPML_NS}}}autoFlightSpeed")
    auto_speed.text = _format_number(auto_flight_speed)

    global_h = ET.SubElement(folder, f"{{{WPML_NS}}}globalHeight")
    global_h.text = _format_number(default_execute_height)

    gimbal_mode = ET.SubElement(folder, f"{{{WPML_NS}}}gimbalPitchMode")
    gimbal_mode.text = str(gimbal_pitch_mode)

    heading_param = ET.SubElement(folder, f"{{{WPML_NS}}}globalWaypointHeadingParam")
    heading_mode_elem = ET.SubElement(heading_param, f"{{{WPML_NS}}}waypointHeadingMode")
    heading_mode_elem.text = str(heading_mode)

    straight_elem = ET.SubElement(folder, f"{{{WPML_NS}}}globalUseStraightLine")
    straight_elem.text = str(int(use_straight_line))

    if hasattr(waypoints, "waypoints") and not isinstance(waypoints, (list, tuple)):
        route_obj = waypoints
        wp_list = list(getattr(route_obj, "waypoints", []))
    else:
        wp_list = list(waypoints) if waypoints is not None else []

    for index, wp in enumerate(wp_list):
        lon, lat, height, speed = _parse_waypoint(
            wp,
            default_execute_height=default_execute_height,
            default_waypoint_speed=default_waypoint_speed,
        )

        placemark = ET.SubElement(folder, f"{{{KML_NS}}}Placemark")

        point = ET.SubElement(placemark, f"{{{KML_NS}}}Point")
        coords = ET.SubElement(point, f"{{{KML_NS}}}coordinates")
        coords.text = f"{lon},{lat}"

        index_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}index")
        index_elem.text = str(index)

        use_global_height = ET.SubElement(placemark, f"{{{WPML_NS}}}useGlobalHeight")
        use_global_height.text = "0"

        height_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}height")
        height_elem.text = str(height)

        use_global_speed = ET.SubElement(placemark, f"{{{WPML_NS}}}useGlobalSpeed")
        use_global_speed.text = "0"

        wp_speed_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}waypointSpeed")
        wp_speed_elem.text = str(speed)

        use_global_heading = ET.SubElement(placemark, f"{{{WPML_NS}}}useGlobalHeadingParam")
        use_global_heading.text = "1"

        use_global_turn = ET.SubElement(placemark, f"{{{WPML_NS}}}useGlobalTurnParam")
        use_global_turn.text = "1"

    return kml_root


def serialize_template_kml(
    waypoints: Sequence[Any] | Any | None = None,
    author: str = "QGC4QGIS",
    create_time: int | None = None,
    update_time: int | None = None,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    template_id: int = 0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    heading_mode: str = "followWayline",
    gimbal_pitch_mode: str = "manual",
    use_straight_line: int = 1,
    kml_root: ET.Element | None = None,
    indent: bool = True,
) -> str:
    """Serialize template.kml XML structure to a string.

    :param author: Mission author name.
    :param create_time: Creation timestamp in milliseconds.
    :param update_time: Update timestamp in milliseconds.
    :param mission_config: Optional pre-built wpml:missionConfig Element.
    :param fly_to_wayline_mode: Mode to fly to wayline.
    :param finish_action: Action upon mission completion.
    :param exit_on_rc_lost: Behavior when RC lost.
    :param execute_rc_lost_action: Specific action on RC lost.
    :param takeoff_security_height: Security takeoff height.
    :param takeoff_ref_point: Takeoff reference point (lat, lon, alt).
    :param takeoff_ref_point_agl_height: Takeoff reference point AGL height.
    :param global_transitional_speed: Speed in m/s.
    :param drone_enum_value: DJI drone enum value (default 68).
    :param drone_sub_enum_value: DJI drone sub enum value (default 0).
    :param payload_enum_value: DJI payload enum value (default 0).
    :param payload_sub_enum_value: DJI payload sub enum value (default 0).
    :param payload_position_index: DJI payload position index (default 0).
    :param kml_root: Optional pre-built root <kml> Element.
    :param indent: Whether to pretty-print indent the XML (default True).
    :return: Formatted XML string.
    """
    if kml_root is None:
        kml_root = build_template_kml(
            waypoints=waypoints,
            author=author,
            create_time=create_time,
            update_time=update_time,
            mission_config=mission_config,
            fly_to_wayline_mode=fly_to_wayline_mode,
            finish_action=finish_action,
            exit_on_rc_lost=exit_on_rc_lost,
            execute_rc_lost_action=execute_rc_lost_action,
            takeoff_security_height=takeoff_security_height,
            takeoff_ref_point=takeoff_ref_point,
            takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
            global_transitional_speed=global_transitional_speed,
            drone_enum_value=drone_enum_value,
            drone_sub_enum_value=drone_sub_enum_value,
            payload_enum_value=payload_enum_value,
            payload_sub_enum_value=payload_sub_enum_value,
            payload_position_index=payload_position_index,
            template_id=template_id,
            execute_height_mode=execute_height_mode,
            auto_flight_speed=auto_flight_speed,
            default_execute_height=default_execute_height,
            default_waypoint_speed=default_waypoint_speed,
            heading_mode=heading_mode,
            gimbal_pitch_mode=gimbal_pitch_mode,
            use_straight_line=use_straight_line,
        )

    if indent:
        ET.indent(kml_root, space="  ")

    xml_bytes = ET.tostring(kml_root, encoding="utf-8", xml_declaration=True)
    return xml_bytes.decode("utf-8")


def save_template_kml(
    filepath: str | Path,
    waypoints: Sequence[Any] | Any | None = None,
    author: str = "QGC4QGIS",
    create_time: int | None = None,
    update_time: int | None = None,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    template_id: int = 0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    heading_mode: str = "followWayline",
    gimbal_pitch_mode: str = "manual",
    use_straight_line: int = 1,
    kml_root: ET.Element | None = None,
) -> None:
    """Serialize template.kml XML structure and write to file.

    :param filepath: Output file path.
    """
    xml_str = serialize_template_kml(
        waypoints=waypoints,
        author=author,
        create_time=create_time,
        update_time=update_time,
        mission_config=mission_config,
        fly_to_wayline_mode=fly_to_wayline_mode,
        finish_action=finish_action,
        exit_on_rc_lost=exit_on_rc_lost,
        execute_rc_lost_action=execute_rc_lost_action,
        takeoff_security_height=takeoff_security_height,
        takeoff_ref_point=takeoff_ref_point,
        takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
        global_transitional_speed=global_transitional_speed,
        drone_enum_value=drone_enum_value,
        drone_sub_enum_value=drone_sub_enum_value,
        payload_enum_value=payload_enum_value,
        payload_sub_enum_value=payload_sub_enum_value,
        payload_position_index=payload_position_index,
        template_id=template_id,
        execute_height_mode=execute_height_mode,
        auto_flight_speed=auto_flight_speed,
        default_execute_height=default_execute_height,
        default_waypoint_speed=default_waypoint_speed,
        heading_mode=heading_mode,
        gimbal_pitch_mode=gimbal_pitch_mode,
        use_straight_line=use_straight_line,
        kml_root=kml_root,
    )
    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(xml_str, encoding="utf-8")


def _parse_waypoint(
    wp: Any,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
) -> tuple[float, float, float, float]:
    """Extract (lon, lat, execute_height, waypoint_speed) from a waypoint object/dict/tuple."""
    if isinstance(wp, dict):
        lon = float(wp.get("lon", wp.get("longitude", wp.get("lng", 0.0))))
        lat = float(wp.get("lat", wp.get("latitude", 0.0)))
        height = float(
            wp.get(
                "executeHeight",
                wp.get(
                    "execute_height",
                    wp.get("height", wp.get("altitude", wp.get("alt", default_execute_height))),
                ),
            )
        )
        speed = float(
            wp.get(
                "waypointSpeed", wp.get("waypoint_speed", wp.get("speed", default_waypoint_speed))
            )
        )
        return lon, lat, height, speed
    if isinstance(wp, (list, tuple)):
        lon = float(wp[0]) if len(wp) > 0 else 0.0
        lat = float(wp[1]) if len(wp) > 1 else 0.0
        height = float(wp[2]) if len(wp) > 2 else default_execute_height
        speed = float(wp[3]) if len(wp) > 3 else default_waypoint_speed
        return lon, lat, height, speed
    lon = float(getattr(wp, "lon", getattr(wp, "longitude", 0.0)))
    lat = float(getattr(wp, "lat", getattr(wp, "latitude", 0.0)))
    height = float(
        getattr(
            wp,
            "executeHeight",
            getattr(
                wp,
                "execute_height",
                getattr(
                    wp,
                    "height",
                    getattr(
                        wp,
                        "altitude",
                        getattr(wp, "alt", getattr(wp, "altura", default_execute_height)),
                    ),
                ),
            ),
        )
    )
    speed = float(getattr(wp, "waypointSpeed", getattr(wp, "speed", default_waypoint_speed)))
    return lon, lat, height, speed


def _format_number(val: float | int) -> str:
    """Format a number as int string if integer value, else str(val)."""
    if isinstance(val, int) or (isinstance(val, float) and val.is_integer()):
        return str(int(val))
    return str(val)


def _extract_gimbal_pitch(wp: Any, default: float = -90.0) -> float:
    """Extract gimbal pitch angle from a waypoint object, dict, or tuple."""
    if isinstance(wp, dict):
        pitch = wp.get("gimbal_pitch", wp.get("gimbalPitch", wp.get("pitch")))
        if pitch is not None:
            return float(pitch)
    elif isinstance(wp, (list, tuple)):
        if len(wp) > 4:
            return float(wp[4])
    else:
        pitch = getattr(wp, "gimbal_pitch", getattr(wp, "gimbalPitch", getattr(wp, "pitch", None)))
        if pitch is not None:
            return float(pitch)
    return float(default)


def _extract_wait_time(wp: Any) -> float:
    """Extract wait/hover time in seconds from a waypoint object, dict, or tuple."""
    wt = None
    if isinstance(wp, dict):
        wt = wp.get(
            "wait_time",
            wp.get(
                "waitTime",
                wp.get("hover_time", wp.get("hoverTime", wp.get("wait", wp.get("pause")))),
            ),
        )
        if wt is None and "acoes" in wp:
            for ac in wp.get("acoes", []):
                if isinstance(ac, dict) and ac.get("type") in ("hover", "wait", "pause"):
                    wt = ac.get("param", ac.get("hoverTime", ac.get("time")))
                    break
    elif isinstance(wp, (list, tuple)):
        if len(wp) > 5:
            wt = wp[5]
    else:
        wt = getattr(
            wp,
            "wait_time",
            getattr(
                wp,
                "waitTime",
                getattr(
                    wp,
                    "hover_time",
                    getattr(wp, "hoverTime", getattr(wp, "wait", getattr(wp, "pause", None))),
                ),
            ),
        )
        if wt is None and hasattr(wp, "acoes"):
            for ac in getattr(wp, "acoes", []):
                if isinstance(ac, dict) and ac.get("type") in ("hover", "wait", "pause"):
                    wt = ac.get("param", ac.get("hoverTime", ac.get("time")))
                    break
    return float(wt) if wt is not None else 0.0


def build_waylines_wpml(
    waypoints: Sequence[Any] | Any | None = None,
    template_id: int = 0,
    wayline_id: int = 0,
    distance: float = 0.0,
    duration: float = 0.0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    heading_mode: str = "followWayline",
    heading_path_mode: str = "followBadArc",
    turn_mode: str = "toPointAndStopWithDiscontinuityCurvature",
    turn_damp: float = 0.0,
    use_straight_line: int = 1,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    default_gimbal_pitch: float = -90.0,
    trigger_mode: str = "POR_DISTANCIA",
    trigger_distance: float = 0.0,
    gimbal_pitch: float | None = None,
    modo_disparo: str | None = None,
    distancia_disparo: float | None = None,
) -> ET.Element:
    """Build a complete waylines.wpml XML root element for DJI WPML.

    :param waypoints: Sequence of waypoint dicts, tuples, objects, or Route instance.
    :param template_id: Template ID (default 0).
    :param wayline_id: Wayline ID (default 0).
    :param distance: Total wayline distance (default 0).
    :param duration: Total wayline duration (default 0).
    :param execute_height_mode: Height mode (default "relativeToStartPoint").
    :param auto_flight_speed: Automatic flight speed in m/s (default 5.0).
    :param mission_config: Optional pre-built wpml:missionConfig Element.
    :return: xml.etree.ElementTree.Element representing root <kml>.
    """
    kml_root = ET.Element(f"{{{KML_NS}}}kml")
    doc = ET.SubElement(kml_root, f"{{{KML_NS}}}Document")

    if mission_config is None:
        mission_config = build_mission_config(
            fly_to_wayline_mode=fly_to_wayline_mode,
            finish_action=finish_action,
            exit_on_rc_lost=exit_on_rc_lost,
            execute_rc_lost_action=execute_rc_lost_action,
            takeoff_security_height=takeoff_security_height,
            takeoff_ref_point=takeoff_ref_point,
            takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
            global_transitional_speed=global_transitional_speed,
            drone_enum_value=drone_enum_value,
            drone_sub_enum_value=drone_sub_enum_value,
            payload_enum_value=payload_enum_value,
            payload_sub_enum_value=payload_sub_enum_value,
            payload_position_index=payload_position_index,
        )

    doc.append(mission_config)

    folder = ET.SubElement(doc, f"{{{KML_NS}}}Folder")

    template_id_elem = ET.SubElement(folder, f"{{{WPML_NS}}}templateId")
    template_id_elem.text = str(int(template_id))

    execute_height_mode_elem = ET.SubElement(folder, f"{{{WPML_NS}}}executeHeightMode")
    execute_height_mode_elem.text = str(execute_height_mode)

    wayline_id_elem = ET.SubElement(folder, f"{{{WPML_NS}}}waylineId")
    wayline_id_elem.text = str(int(wayline_id))

    distance_elem = ET.SubElement(folder, f"{{{WPML_NS}}}distance")
    distance_elem.text = _format_number(distance)

    duration_elem = ET.SubElement(folder, f"{{{WPML_NS}}}duration")
    duration_elem.text = _format_number(duration)

    speed_elem = ET.SubElement(folder, f"{{{WPML_NS}}}autoFlightSpeed")
    speed_elem.text = str(auto_flight_speed)

    if hasattr(waypoints, "waypoints") and not isinstance(waypoints, (list, tuple)):
        route_obj = waypoints
        wp_list = list(getattr(route_obj, "waypoints", []))
        trig_mode = (
            modo_disparo
            if modo_disparo is not None
            else getattr(
                route_obj,
                "modo_disparo",
                getattr(route_obj, "trigger_mode", trigger_mode),
            )
        )
        trig_dist = (
            distancia_disparo
            if distancia_disparo is not None
            else getattr(
                route_obj,
                "distancia_disparo",
                getattr(route_obj, "trigger_distance", trigger_distance),
            )
        )
    else:
        wp_list = list(waypoints) if waypoints is not None else []
        trig_mode = modo_disparo if modo_disparo is not None else trigger_mode
        trig_dist = distancia_disparo if distancia_disparo is not None else trigger_distance

    gimbal_pitch_val = float(gimbal_pitch) if gimbal_pitch is not None else default_gimbal_pitch

    if wp_list:
        for index, wp in enumerate(wp_list):
            lon, lat, height, speed = _parse_waypoint(
                wp,
                default_execute_height=default_execute_height,
                default_waypoint_speed=default_waypoint_speed,
            )

            placemark = ET.SubElement(folder, f"{{{KML_NS}}}Placemark")

            point = ET.SubElement(placemark, f"{{{KML_NS}}}Point")
            coords = ET.SubElement(point, f"{{{KML_NS}}}coordinates")
            coords.text = f"{lon},{lat}"

            index_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}index")
            index_elem.text = str(index)

            height_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}executeHeight")
            height_elem.text = str(height)

            wp_speed_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}waypointSpeed")
            wp_speed_elem.text = str(speed)

            heading_param = ET.SubElement(placemark, f"{{{WPML_NS}}}waypointHeadingParam")
            heading_mode_elem = ET.SubElement(heading_param, f"{{{WPML_NS}}}waypointHeadingMode")
            heading_mode_elem.text = str(heading_mode)
            heading_path_elem = ET.SubElement(
                heading_param, f"{{{WPML_NS}}}waypointHeadingPathMode"
            )
            heading_path_elem.text = str(heading_path_mode)

            turn_param = ET.SubElement(placemark, f"{{{WPML_NS}}}waypointTurnParam")
            turn_mode_elem = ET.SubElement(turn_param, f"{{{WPML_NS}}}waypointTurnMode")
            turn_mode_elem.text = str(turn_mode)
            turn_damp_elem = ET.SubElement(turn_param, f"{{{WPML_NS}}}waypointTurnDampingDist")
            turn_damp_elem.text = _format_number(turn_damp)

            straight_elem = ET.SubElement(placemark, f"{{{WPML_NS}}}useStraightLine")
            straight_elem.text = str(int(use_straight_line))

        action_group_id = 0
        num_wps = len(wp_list)

        for index, wp in enumerate(wp_list):
            actions_for_wp: list[tuple[str, dict[str, Any]]] = []

            if index == 0:
                pitch = _extract_gimbal_pitch(
                    wp,
                    default=gimbal_pitch_val if gimbal_pitch is not None else default_gimbal_pitch,
                )
                actions_for_wp.append(
                    (
                        "gimbalEvenlyRotate",
                        {
                            "gimbalPitchRotateAngle": pitch,
                            "gimbalRollRotateAngle": 0,
                            "gimbalYawRotateAngle": 0,
                            "gimbalRotateTime": 0,
                            "payloadPositionIndex": 0,
                        },
                    )
                )

            wait_time = _extract_wait_time(wp)
            if wait_time > 0:
                actions_for_wp.append(("hover", {"hoverTime": wait_time}))

            if trig_mode == "POR_FOTO":
                actions_for_wp.append(("takePhoto", {"payloadPositionIndex": 0}))

            if actions_for_wp:
                ag = ET.SubElement(folder, f"{{{WPML_NS}}}actionGroup")
                ag_id = ET.SubElement(ag, f"{{{WPML_NS}}}actionGroupId")
                ag_id.text = str(action_group_id)

                ag_start = ET.SubElement(ag, f"{{{WPML_NS}}}actionGroupStartIndex")
                ag_start.text = str(index)

                ag_end = ET.SubElement(ag, f"{{{WPML_NS}}}actionGroupEndIndex")
                ag_end.text = str(index)

                ag_mode = ET.SubElement(ag, f"{{{WPML_NS}}}actionGroupMode")
                ag_mode.text = "sequence"

                ag_trig = ET.SubElement(ag, f"{{{WPML_NS}}}actionTrigger")
                ag_trig_type = ET.SubElement(ag_trig, f"{{{WPML_NS}}}actionTriggerType")
                ag_trig_type.text = "reachPoint"

                for act_idx, (func_name, func_params) in enumerate(actions_for_wp):
                    act_elem = ET.SubElement(ag, f"{{{WPML_NS}}}action")
                    act_id_elem = ET.SubElement(act_elem, f"{{{WPML_NS}}}actionId")
                    act_id_elem.text = str(act_idx)

                    act_func_elem = ET.SubElement(act_elem, f"{{{WPML_NS}}}actionActuatorFunc")
                    act_func_elem.text = func_name

                    param_elem = ET.SubElement(act_elem, f"{{{WPML_NS}}}actionActuatorFuncParam")
                    for p_key, p_val in func_params.items():
                        sub_param = ET.SubElement(param_elem, f"{{{WPML_NS}}}{p_key}")
                        sub_param.text = _format_number(p_val)

                action_group_id += 1

        if trig_mode == "POR_DISTANCIA":
            ag_dist = ET.SubElement(folder, f"{{{WPML_NS}}}actionGroup")
            ag_d_id = ET.SubElement(ag_dist, f"{{{WPML_NS}}}actionGroupId")
            ag_d_id.text = str(action_group_id)

            ag_d_start = ET.SubElement(ag_dist, f"{{{WPML_NS}}}actionGroupStartIndex")
            ag_d_start.text = "0"

            ag_d_end = ET.SubElement(ag_dist, f"{{{WPML_NS}}}actionGroupEndIndex")
            ag_d_end.text = str(num_wps - 1)

            ag_d_mode = ET.SubElement(ag_dist, f"{{{WPML_NS}}}actionGroupMode")
            ag_d_mode.text = "sequence"

            ag_d_trig = ET.SubElement(ag_dist, f"{{{WPML_NS}}}actionTrigger")
            ag_d_trig_type = ET.SubElement(ag_d_trig, f"{{{WPML_NS}}}actionTriggerType")
            ag_d_trig_type.text = "multipleDistance"

            ag_d_param = ET.SubElement(ag_d_trig, f"{{{WPML_NS}}}actionTriggerParam")
            ag_d_param.text = _format_number(trig_dist)

            act_elem = ET.SubElement(ag_dist, f"{{{WPML_NS}}}action")
            act_id_elem = ET.SubElement(act_elem, f"{{{WPML_NS}}}actionId")
            act_id_elem.text = "0"

            act_func_elem = ET.SubElement(act_elem, f"{{{WPML_NS}}}actionActuatorFunc")
            act_func_elem.text = "takePhoto"

            param_elem = ET.SubElement(act_elem, f"{{{WPML_NS}}}actionActuatorFuncParam")
            payload_elem = ET.SubElement(param_elem, f"{{{WPML_NS}}}payloadPositionIndex")
            payload_elem.text = "0"

            action_group_id += 1

    return kml_root


def serialize_waylines_wpml(
    waypoints: Sequence[Any] | Any | None = None,
    template_id: int = 0,
    wayline_id: int = 0,
    distance: float = 0.0,
    duration: float = 0.0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    heading_mode: str = "followWayline",
    heading_path_mode: str = "followBadArc",
    turn_mode: str = "toPointAndStopWithDiscontinuityCurvature",
    turn_damp: float = 0.0,
    use_straight_line: int = 1,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    default_gimbal_pitch: float = -90.0,
    trigger_mode: str = "POR_DISTANCIA",
    trigger_distance: float = 0.0,
    gimbal_pitch: float | None = None,
    modo_disparo: str | None = None,
    distancia_disparo: float | None = None,
    kml_root: ET.Element | None = None,
    indent: bool = True,
) -> str:
    """Serialize waylines.wpml XML structure to a string.

    :return: Formatted XML string.
    """
    if kml_root is None:
        kml_root = build_waylines_wpml(
            waypoints=waypoints,
            template_id=template_id,
            wayline_id=wayline_id,
            distance=distance,
            duration=duration,
            execute_height_mode=execute_height_mode,
            auto_flight_speed=auto_flight_speed,
            mission_config=mission_config,
            fly_to_wayline_mode=fly_to_wayline_mode,
            finish_action=finish_action,
            exit_on_rc_lost=exit_on_rc_lost,
            execute_rc_lost_action=execute_rc_lost_action,
            takeoff_security_height=takeoff_security_height,
            takeoff_ref_point=takeoff_ref_point,
            takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
            global_transitional_speed=global_transitional_speed,
            drone_enum_value=drone_enum_value,
            drone_sub_enum_value=drone_sub_enum_value,
            payload_enum_value=payload_enum_value,
            payload_sub_enum_value=payload_sub_enum_value,
            payload_position_index=payload_position_index,
            heading_mode=heading_mode,
            heading_path_mode=heading_path_mode,
            turn_mode=turn_mode,
            turn_damp=turn_damp,
            use_straight_line=use_straight_line,
            default_execute_height=default_execute_height,
            default_waypoint_speed=default_waypoint_speed,
            default_gimbal_pitch=default_gimbal_pitch,
            trigger_mode=trigger_mode,
            trigger_distance=trigger_distance,
            gimbal_pitch=gimbal_pitch,
            modo_disparo=modo_disparo,
            distancia_disparo=distancia_disparo,
        )

    if indent:
        ET.indent(kml_root, space="  ")

    xml_bytes = ET.tostring(kml_root, encoding="utf-8", xml_declaration=True)
    return xml_bytes.decode("utf-8")


def save_waylines_wpml(
    filepath: str | Path,
    waypoints: Sequence[Any] | Any | None = None,
    template_id: int = 0,
    wayline_id: int = 0,
    distance: float = 0.0,
    duration: float = 0.0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    heading_mode: str = "followWayline",
    heading_path_mode: str = "followBadArc",
    turn_mode: str = "toPointAndStopWithDiscontinuityCurvature",
    turn_damp: float = 0.0,
    use_straight_line: int = 1,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    default_gimbal_pitch: float = -90.0,
    trigger_mode: str = "POR_DISTANCIA",
    trigger_distance: float = 0.0,
    gimbal_pitch: float | None = None,
    modo_disparo: str | None = None,
    distancia_disparo: float | None = None,
    kml_root: ET.Element | None = None,
) -> None:
    """Serialize waylines.wpml XML structure and write to file.

    :param filepath: Output file path.
    """
    xml_str = serialize_waylines_wpml(
        waypoints=waypoints,
        template_id=template_id,
        wayline_id=wayline_id,
        distance=distance,
        duration=duration,
        execute_height_mode=execute_height_mode,
        auto_flight_speed=auto_flight_speed,
        mission_config=mission_config,
        fly_to_wayline_mode=fly_to_wayline_mode,
        finish_action=finish_action,
        exit_on_rc_lost=exit_on_rc_lost,
        execute_rc_lost_action=execute_rc_lost_action,
        takeoff_security_height=takeoff_security_height,
        takeoff_ref_point=takeoff_ref_point,
        takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
        global_transitional_speed=global_transitional_speed,
        drone_enum_value=drone_enum_value,
        drone_sub_enum_value=drone_sub_enum_value,
        payload_enum_value=payload_enum_value,
        payload_sub_enum_value=payload_sub_enum_value,
        payload_position_index=payload_position_index,
        heading_mode=heading_mode,
        heading_path_mode=heading_path_mode,
        turn_mode=turn_mode,
        turn_damp=turn_damp,
        use_straight_line=use_straight_line,
        default_execute_height=default_execute_height,
        default_waypoint_speed=default_waypoint_speed,
        default_gimbal_pitch=default_gimbal_pitch,
        trigger_mode=trigger_mode,
        trigger_distance=trigger_distance,
        gimbal_pitch=gimbal_pitch,
        modo_disparo=modo_disparo,
        distancia_disparo=distancia_disparo,
        kml_root=kml_root,
    )
    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(xml_str, encoding="utf-8")


def validate_wpml_route(
    waypoints: Sequence[Any] | Any | None = None,
    route: Any | None = None,
    distance_mode: int | str | None = None,
    execute_height_mode: str = "relativeToStartPoint",
    max_waypoints: int = MAX_WPML_WAYPOINTS,
) -> list[str]:
    """Validate a route or waypoint sequence for DJI WPML export.

    :param waypoints: Waypoints list or Route instance.
    :param route: Optional Route instance.
    :param distance_mode: Altitude/distance mode value.
    :param execute_height_mode: WPML execute height mode string.
    :param max_waypoints: Maximum allowed waypoints limit.
    :return: List of warning strings.
    :raises ValueError: If route is in terrain mode (D10 refusal).
    """
    route_obj = (
        route
        if route is not None
        else (
            waypoints
            if hasattr(waypoints, "waypoints") and not isinstance(waypoints, (list, tuple))
            else None
        )
    )

    alt_mode = None
    if distance_mode is not None:
        alt_mode = distance_mode
    elif route_obj is not None:
        alt_mode = getattr(route_obj, "modo_altitude", getattr(route_obj, "distance_mode", None))

    terrain_like_height_mode = isinstance(
        execute_height_mode, str
    ) and execute_height_mode.lower() in ("realtimefollowsurface", "terrain", "wgs84")
    relative_modes = (DistanceMode.RELATIVE, 0, "RELATIVE", "relative")
    if terrain_like_height_mode or (alt_mode is not None and alt_mode not in relative_modes):
        raise ValueError(
            tr("WPML export only accepts height relative to the takeoff point; ")
            + tr("convert the route with rebase_route_to_takeoff() before exporting (D10).")
        )

    warnings: list[str] = []
    if route_obj is not None:
        raw_avisos = getattr(route_obj, "avisos", getattr(route_obj, "warnings", []))
        if raw_avisos:
            warnings.extend(raw_avisos)

    if route_obj is not None:
        wp_list = list(getattr(route_obj, "waypoints", []))
    elif isinstance(waypoints, (list, tuple)):
        wp_list = list(waypoints)
    else:
        wp_list = []

    le_zero_count = sum(1 for wp in wp_list if _parse_waypoint(wp)[2] <= 0)
    if le_zero_count > 0:
        warnings.append(
            tr(
                "Relative height ≤ 0 in {le_zero_count} waypoint(s): the route goes below the takeoff point."
            ).format(le_zero_count=le_zero_count)
        )

    if max_waypoints > 0 and len(wp_list) > max_waypoints:
        warnings.append(
            tr(
                "Number of waypoints ({n_waypoints}) exceeds the DJI WPML limit ({max_waypoints})."
            ).format(n_waypoints=len(wp_list), max_waypoints=max_waypoints)
        )

    return warnings


def save_kmz(
    filepath: str | Path,
    waypoints: Sequence[Any] | Any | None = None,
    route: Any | None = None,
    in_wpmz_dir: bool = True,
    use_wpmz_dir: bool | None = None,
    author: str = "QGC4QGIS",
    create_time: int | None = None,
    update_time: int | None = None,
    template_id: int = 0,
    wayline_id: int = 0,
    distance: float = 0.0,
    duration: float = 0.0,
    execute_height_mode: str = "relativeToStartPoint",
    auto_flight_speed: float = 5.0,
    mission_config: ET.Element | None = None,
    fly_to_wayline_mode: str = "safely",
    finish_action: str = "goHome",
    exit_on_rc_lost: str = "executeLostAction",
    execute_rc_lost_action: str = "goBack",
    takeoff_security_height: float = 20.0,
    takeoff_ref_point: tuple[float, float, float] | None = None,
    takeoff_ref_point_agl_height: float = 0.0,
    global_transitional_speed: float = 5.0,
    drone_enum_value: int = DEFAULT_DRONE_ENUM_VALUE,
    drone_sub_enum_value: int = 0,
    payload_enum_value: int = 0,
    payload_sub_enum_value: int = 0,
    payload_position_index: int = 0,
    heading_mode: str = "followWayline",
    heading_path_mode: str = "followBadArc",
    turn_mode: str = "toPointAndStopWithDiscontinuityCurvature",
    turn_damp: float = 0.0,
    use_straight_line: int = 1,
    default_execute_height: float = 50.0,
    default_waypoint_speed: float = 5.0,
    default_gimbal_pitch: float = -90.0,
    trigger_mode: str = "POR_DISTANCIA",
    trigger_distance: float = 0.0,
    gimbal_pitch: float | None = None,
    modo_disparo: str | None = None,
    distancia_disparo: float | None = None,
    distance_mode: int | str | None = None,
    max_waypoints: int = MAX_WPML_WAYPOINTS,
) -> list[str]:
    """Serialize template.kml and waylines.wpml XML structures into a KMZ archive.

    :param filepath: Target path for .kmz zip file.
    :param waypoints: Sequence of waypoints or Route instance.
    :param route: Optional Route instance.
    :param in_wpmz_dir: Put files under 'wpmz/' folder inside zip if True (default), else at root.
    :param use_wpmz_dir: Alias for in_wpmz_dir.
    :return: List of warning strings.
    :raises ValueError: If route is in terrain mode (D10 refusal).
    """
    target_wps = route if route is not None else waypoints
    wpmz_flag = use_wpmz_dir if use_wpmz_dir is not None else in_wpmz_dir

    warnings = validate_wpml_route(
        waypoints=target_wps,
        route=route,
        distance_mode=distance_mode,
        execute_height_mode=execute_height_mode,
        max_waypoints=max_waypoints,
    )

    template_xml = serialize_template_kml(
        waypoints=target_wps,
        author=author,
        create_time=create_time,
        update_time=update_time,
        mission_config=mission_config,
        fly_to_wayline_mode=fly_to_wayline_mode,
        finish_action=finish_action,
        exit_on_rc_lost=exit_on_rc_lost,
        execute_rc_lost_action=execute_rc_lost_action,
        takeoff_security_height=takeoff_security_height,
        takeoff_ref_point=takeoff_ref_point,
        takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
        global_transitional_speed=global_transitional_speed,
        drone_enum_value=drone_enum_value,
        drone_sub_enum_value=drone_sub_enum_value,
        payload_enum_value=payload_enum_value,
        payload_sub_enum_value=payload_sub_enum_value,
        payload_position_index=payload_position_index,
        template_id=template_id,
        execute_height_mode=execute_height_mode,
        auto_flight_speed=auto_flight_speed,
        default_execute_height=default_execute_height,
        default_waypoint_speed=default_waypoint_speed,
        heading_mode=heading_mode,
        use_straight_line=use_straight_line,
    )

    waylines_xml = serialize_waylines_wpml(
        waypoints=target_wps,
        template_id=template_id,
        wayline_id=wayline_id,
        distance=distance,
        duration=duration,
        execute_height_mode=execute_height_mode,
        auto_flight_speed=auto_flight_speed,
        mission_config=mission_config,
        fly_to_wayline_mode=fly_to_wayline_mode,
        finish_action=finish_action,
        exit_on_rc_lost=exit_on_rc_lost,
        execute_rc_lost_action=execute_rc_lost_action,
        takeoff_security_height=takeoff_security_height,
        takeoff_ref_point=takeoff_ref_point,
        takeoff_ref_point_agl_height=takeoff_ref_point_agl_height,
        global_transitional_speed=global_transitional_speed,
        drone_enum_value=drone_enum_value,
        drone_sub_enum_value=drone_sub_enum_value,
        payload_enum_value=payload_enum_value,
        payload_sub_enum_value=payload_sub_enum_value,
        payload_position_index=payload_position_index,
        heading_mode=heading_mode,
        heading_path_mode=heading_path_mode,
        turn_mode=turn_mode,
        turn_damp=turn_damp,
        use_straight_line=use_straight_line,
        default_execute_height=default_execute_height,
        default_waypoint_speed=default_waypoint_speed,
        default_gimbal_pitch=default_gimbal_pitch,
        trigger_mode=trigger_mode,
        trigger_distance=trigger_distance,
        gimbal_pitch=gimbal_pitch,
        modo_disparo=modo_disparo,
        distancia_disparo=distancia_disparo,
    )

    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)

    prefix = "wpmz/" if wpmz_flag else ""
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"{prefix}template.kml", template_xml)
        zf.writestr(f"{prefix}waylines.wpml", waylines_xml)

    return warnings
