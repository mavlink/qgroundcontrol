"""Project parameter persistence manager for QGC4QGIS."""

from typing import Any

from qgis.core import QgsProject, QgsRasterLayer, QgsVectorLayer

PROJECT_SCOPE = "qgc4qgis"

# Default setting values for QGC4QGIS panel
DEFAULT_SETTINGS: dict[str, Any] = {
    "LAYER_ID": "",
    "ELEVATION_LAYER_ID": "",
    "FEATURE_ID": None,
    "CAMERA": 0,
    "ALTITUDE": 100.0,
    "GSD": 2.0,
    "MODE_ALTITUDE": True,
    "OVERLAP_SIDE": 70.0,
    "OVERLAP_FRONTAL": 70.0,
    "ANGLE": 0.0,
    "TURNAROUND": 0.0,
    "ENTRY_LOCATION": 0,
    "REFLY": False,
    "SENSOR_WIDTH": 35.9,
    "SENSOR_HEIGHT": 24.0,
    "IMAGE_WIDTH": 7952,
    "IMAGE_HEIGHT": 5304,
    "FOCAL_LENGTH": 35.0,
    "TOLERANCE": 10.0,
    "TRIGGER_MODE": 0,
    "SPEED": 5.0,
    "GIMBAL_PITCH": -90.0,
    "WAYPOINT_WAIT": 0.0,
}


def save_project_settings(params: dict[str, Any], project: QgsProject | None = None) -> None:
    """Save flight planning parameters to QgsProject entries.

    :param params: Dictionary of parameter names and values.
    :param project: QgsProject instance, or None to use QgsProject.instance().
    """
    if project is None:
        project = QgsProject.instance()

    # Extract layer ID
    layer_val = params.get("INPUT") or params.get("LAYER_ID")
    if isinstance(layer_val, QgsVectorLayer):
        layer_id = layer_val.id()
    elif isinstance(layer_val, str):
        layer_id = layer_val
    else:
        layer_id = DEFAULT_SETTINGS["LAYER_ID"]
    project.writeEntry(PROJECT_SCOPE, "LAYER_ID", layer_id)

    # Extract elevation layer ID
    elev_layer_val = params.get("ELEVATION_LAYER") or params.get("ELEVATION_LAYER_ID")
    if isinstance(elev_layer_val, QgsRasterLayer):
        elev_layer_id = elev_layer_val.id()
    elif isinstance(elev_layer_val, str):
        elev_layer_id = elev_layer_val
    else:
        elev_layer_id = DEFAULT_SETTINGS["ELEVATION_LAYER_ID"]
    project.writeEntry(PROJECT_SCOPE, "ELEVATION_LAYER_ID", elev_layer_id)

    # Feature ID (-1 represents None)
    feat_id = params.get("FEATURE_ID")
    feat_id_int = int(feat_id) if feat_id is not None else -1
    project.writeEntry(PROJECT_SCOPE, "FEATURE_ID", feat_id_int)

    # Int entries
    project.writeEntry(
        PROJECT_SCOPE, "CAMERA", int(params.get("CAMERA", DEFAULT_SETTINGS["CAMERA"]))
    )
    project.writeEntry(
        PROJECT_SCOPE,
        "ENTRY_LOCATION",
        int(params.get("ENTRY_LOCATION", DEFAULT_SETTINGS["ENTRY_LOCATION"])),
    )
    project.writeEntry(
        PROJECT_SCOPE,
        "IMAGE_WIDTH",
        int(params.get("IMAGE_WIDTH", DEFAULT_SETTINGS["IMAGE_WIDTH"])),
    )
    project.writeEntry(
        PROJECT_SCOPE,
        "IMAGE_HEIGHT",
        int(params.get("IMAGE_HEIGHT", DEFAULT_SETTINGS["IMAGE_HEIGHT"])),
    )
    project.writeEntry(
        PROJECT_SCOPE,
        "TRIGGER_MODE",
        int(params.get("TRIGGER_MODE", DEFAULT_SETTINGS["TRIGGER_MODE"])),
    )

    # Double entries
    project.writeEntryDouble(
        PROJECT_SCOPE, "ALTITUDE", float(params.get("ALTITUDE", DEFAULT_SETTINGS["ALTITUDE"]))
    )
    project.writeEntryDouble(
        PROJECT_SCOPE, "GSD", float(params.get("GSD", DEFAULT_SETTINGS["GSD"]))
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "OVERLAP_SIDE",
        float(params.get("OVERLAP_SIDE", DEFAULT_SETTINGS["OVERLAP_SIDE"])),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "OVERLAP_FRONTAL",
        float(params.get("OVERLAP_FRONTAL", DEFAULT_SETTINGS["OVERLAP_FRONTAL"])),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE, "ANGLE", float(params.get("ANGLE", DEFAULT_SETTINGS["ANGLE"]))
    )
    project.writeEntryDouble(
        PROJECT_SCOPE, "TURNAROUND", float(params.get("TURNAROUND", DEFAULT_SETTINGS["TURNAROUND"]))
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "SENSOR_WIDTH",
        float(params.get("SENSOR_WIDTH", DEFAULT_SETTINGS["SENSOR_WIDTH"])),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "SENSOR_HEIGHT",
        float(params.get("SENSOR_HEIGHT", DEFAULT_SETTINGS["SENSOR_HEIGHT"])),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "FOCAL_LENGTH",
        float(params.get("FOCAL_LENGTH", DEFAULT_SETTINGS["FOCAL_LENGTH"])),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "TOLERANCE",
        float(
            params.get("TOLERANCE", params.get("TERRAIN_TOLERANCE", DEFAULT_SETTINGS["TOLERANCE"]))
        ),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE, "SPEED", float(params.get("SPEED", DEFAULT_SETTINGS["SPEED"]))
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "GIMBAL_PITCH",
        float(params.get("GIMBAL_PITCH", DEFAULT_SETTINGS["GIMBAL_PITCH"])),
    )
    project.writeEntryDouble(
        PROJECT_SCOPE,
        "WAYPOINT_WAIT",
        float(params.get("WAYPOINT_WAIT", DEFAULT_SETTINGS["WAYPOINT_WAIT"])),
    )

    # Bool entries
    project.writeEntryBool(
        PROJECT_SCOPE,
        "MODE_ALTITUDE",
        bool(params.get("MODE_ALTITUDE", DEFAULT_SETTINGS["MODE_ALTITUDE"])),
    )
    project.writeEntryBool(
        PROJECT_SCOPE, "REFLY", bool(params.get("REFLY", DEFAULT_SETTINGS["REFLY"]))
    )


def load_project_settings(project: QgsProject | None = None) -> dict[str, Any]:
    """Load flight planning parameters from QgsProject entries.

    :param project: QgsProject instance, or None to use QgsProject.instance().
    :return: Dictionary containing loaded parameters with fallback to defaults.
    """
    if project is None:
        project = QgsProject.instance()

    layer_id, _ = project.readEntry(PROJECT_SCOPE, "LAYER_ID", DEFAULT_SETTINGS["LAYER_ID"])
    elev_layer_id, _ = project.readEntry(
        PROJECT_SCOPE, "ELEVATION_LAYER_ID", DEFAULT_SETTINGS["ELEVATION_LAYER_ID"]
    )
    feat_id_raw, _ = project.readNumEntry(PROJECT_SCOPE, "FEATURE_ID", -1)
    feat_id = None if feat_id_raw == -1 else feat_id_raw

    camera, _ = project.readNumEntry(PROJECT_SCOPE, "CAMERA", DEFAULT_SETTINGS["CAMERA"])
    entry_loc, _ = project.readNumEntry(
        PROJECT_SCOPE, "ENTRY_LOCATION", DEFAULT_SETTINGS["ENTRY_LOCATION"]
    )
    img_w, _ = project.readNumEntry(PROJECT_SCOPE, "IMAGE_WIDTH", DEFAULT_SETTINGS["IMAGE_WIDTH"])
    img_h, _ = project.readNumEntry(PROJECT_SCOPE, "IMAGE_HEIGHT", DEFAULT_SETTINGS["IMAGE_HEIGHT"])
    trig_mode, _ = project.readNumEntry(
        PROJECT_SCOPE, "TRIGGER_MODE", DEFAULT_SETTINGS["TRIGGER_MODE"]
    )

    alt, _ = project.readDoubleEntry(PROJECT_SCOPE, "ALTITUDE", DEFAULT_SETTINGS["ALTITUDE"])
    gsd, _ = project.readDoubleEntry(PROJECT_SCOPE, "GSD", DEFAULT_SETTINGS["GSD"])
    ov_side, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "OVERLAP_SIDE", DEFAULT_SETTINGS["OVERLAP_SIDE"]
    )
    ov_frontal, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "OVERLAP_FRONTAL", DEFAULT_SETTINGS["OVERLAP_FRONTAL"]
    )
    angle, _ = project.readDoubleEntry(PROJECT_SCOPE, "ANGLE", DEFAULT_SETTINGS["ANGLE"])
    turnaround, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "TURNAROUND", DEFAULT_SETTINGS["TURNAROUND"]
    )
    sensor_w, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "SENSOR_WIDTH", DEFAULT_SETTINGS["SENSOR_WIDTH"]
    )
    sensor_h, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "SENSOR_HEIGHT", DEFAULT_SETTINGS["SENSOR_HEIGHT"]
    )
    focal, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "FOCAL_LENGTH", DEFAULT_SETTINGS["FOCAL_LENGTH"]
    )
    tolerance, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "TOLERANCE", DEFAULT_SETTINGS["TOLERANCE"]
    )
    speed, _ = project.readDoubleEntry(PROJECT_SCOPE, "SPEED", DEFAULT_SETTINGS["SPEED"])
    gimbal_pitch, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "GIMBAL_PITCH", DEFAULT_SETTINGS["GIMBAL_PITCH"]
    )
    wp_wait, _ = project.readDoubleEntry(
        PROJECT_SCOPE, "WAYPOINT_WAIT", DEFAULT_SETTINGS["WAYPOINT_WAIT"]
    )

    mode_alt, _ = project.readBoolEntry(
        PROJECT_SCOPE, "MODE_ALTITUDE", DEFAULT_SETTINGS["MODE_ALTITUDE"]
    )
    refly, _ = project.readBoolEntry(PROJECT_SCOPE, "REFLY", DEFAULT_SETTINGS["REFLY"])

    return {
        "LAYER_ID": layer_id,
        "ELEVATION_LAYER_ID": elev_layer_id,
        "FEATURE_ID": feat_id,
        "CAMERA": camera,
        "ALTITUDE": alt,
        "GSD": gsd,
        "MODE_ALTITUDE": mode_alt,
        "OVERLAP_SIDE": ov_side,
        "OVERLAP_FRONTAL": ov_frontal,
        "ANGLE": angle,
        "TURNAROUND": turnaround,
        "ENTRY_LOCATION": entry_loc,
        "REFLY": refly,
        "SENSOR_WIDTH": sensor_w,
        "SENSOR_HEIGHT": sensor_h,
        "IMAGE_WIDTH": img_w,
        "IMAGE_HEIGHT": img_h,
        "FOCAL_LENGTH": focal,
        "TOLERANCE": tolerance,
        "TRIGGER_MODE": trig_mode,
        "SPEED": speed,
        "GIMBAL_PITCH": gimbal_pitch,
        "WAYPOINT_WAIT": wp_wait,
    }


def clear_project_settings(project: QgsProject | None = None) -> None:
    """Clear all QGC4QGIS entries from QgsProject.

    :param project: QgsProject instance, or None to use QgsProject.instance().
    """
    if project is None:
        project = QgsProject.instance()
    project.removeEntry(PROJECT_SCOPE, "")
