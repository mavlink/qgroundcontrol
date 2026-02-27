"""
MAVLink message definitions for code completion.

This module provides MAVLink message metadata for IDE features like
autocomplete, hover documentation, and code generation.

When available, messages are loaded dynamically from MAVLink XML definitions.
Falls back to a curated subset of common messages used in QGC.

For the full MAVLink specification, see: https://mavlink.io/en/messages/common.html
"""

import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

logger = logging.getLogger(__name__)


@dataclass
class MAVLinkField:
    """A field within a MAVLink message."""
    name: str
    type: str
    description: str = ""
    units: str = ""
    enum: str = ""  # Enum type name if applicable


@dataclass
class MAVLinkMessage:
    """A MAVLink message definition."""
    name: str
    id: int
    description: str
    fields: list[MAVLinkField] = field(default_factory=list)
    category: str = ""  # e.g., "System", "Position", "Navigation"


# Common MAVLink messages used in QGC
MAVLINK_MESSAGES: list[MAVLinkMessage] = [
    # System messages
    MAVLinkMessage(
        name="HEARTBEAT", id=0, category="System",
        description="Vehicle heartbeat - sent at 1Hz to indicate the vehicle is alive",
        fields=[
            MAVLinkField("type", "uint8_t", "Vehicle type", enum="MAV_TYPE"),
            MAVLinkField("autopilot", "uint8_t", "Autopilot type", enum="MAV_AUTOPILOT"),
            MAVLinkField("base_mode", "uint8_t", "System mode bitmap", enum="MAV_MODE_FLAG"),
            MAVLinkField("custom_mode", "uint32_t", "Autopilot-specific mode"),
            MAVLinkField("system_status", "uint8_t", "System status", enum="MAV_STATE"),
            MAVLinkField("mavlink_version", "uint8_t", "MAVLink version"),
        ]
    ),
    MAVLinkMessage(
        name="SYS_STATUS", id=1, category="System",
        description="System status including battery and sensor health",
        fields=[
            MAVLinkField("onboard_control_sensors_present", "uint32_t", "Sensors present bitmap"),
            MAVLinkField("onboard_control_sensors_enabled", "uint32_t", "Sensors enabled bitmap"),
            MAVLinkField("onboard_control_sensors_health", "uint32_t", "Sensors health bitmap"),
            MAVLinkField("load", "uint16_t", "CPU load (0.1%)", units="d%"),
            MAVLinkField("voltage_battery", "uint16_t", "Battery voltage", units="mV"),
            MAVLinkField("current_battery", "int16_t", "Battery current", units="cA"),
            MAVLinkField("battery_remaining", "int8_t", "Battery remaining", units="%"),
            MAVLinkField("drop_rate_comm", "uint16_t", "Communication drop rate", units="c%"),
            MAVLinkField("errors_comm", "uint16_t", "Communication errors"),
        ]
    ),
    MAVLinkMessage(
        name="SYSTEM_TIME", id=2, category="System",
        description="System time synchronization",
        fields=[
            MAVLinkField("time_unix_usec", "uint64_t", "Unix timestamp", units="us"),
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
        ]
    ),
    MAVLinkMessage(
        name="PING", id=4, category="System",
        description="Ping for measuring latency",
        fields=[
            MAVLinkField("time_usec", "uint64_t", "Timestamp", units="us"),
            MAVLinkField("seq", "uint32_t", "Ping sequence number"),
            MAVLinkField("target_system", "uint8_t", "Target system ID"),
            MAVLinkField("target_component", "uint8_t", "Target component ID"),
        ]
    ),

    # Attitude and orientation
    MAVLinkMessage(
        name="ATTITUDE", id=30, category="Attitude",
        description="Vehicle attitude (roll, pitch, yaw)",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("roll", "float", "Roll angle", units="rad"),
            MAVLinkField("pitch", "float", "Pitch angle", units="rad"),
            MAVLinkField("yaw", "float", "Yaw angle", units="rad"),
            MAVLinkField("rollspeed", "float", "Roll angular speed", units="rad/s"),
            MAVLinkField("pitchspeed", "float", "Pitch angular speed", units="rad/s"),
            MAVLinkField("yawspeed", "float", "Yaw angular speed", units="rad/s"),
        ]
    ),
    MAVLinkMessage(
        name="ATTITUDE_QUATERNION", id=31, category="Attitude",
        description="Vehicle attitude as quaternion",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("q1", "float", "Quaternion w"),
            MAVLinkField("q2", "float", "Quaternion x"),
            MAVLinkField("q3", "float", "Quaternion y"),
            MAVLinkField("q4", "float", "Quaternion z"),
            MAVLinkField("rollspeed", "float", "Roll angular speed", units="rad/s"),
            MAVLinkField("pitchspeed", "float", "Pitch angular speed", units="rad/s"),
            MAVLinkField("yawspeed", "float", "Yaw angular speed", units="rad/s"),
        ]
    ),

    # Position
    MAVLinkMessage(
        name="LOCAL_POSITION_NED", id=32, category="Position",
        description="Local position in NED frame",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("x", "float", "X position (North)", units="m"),
            MAVLinkField("y", "float", "Y position (East)", units="m"),
            MAVLinkField("z", "float", "Z position (Down)", units="m"),
            MAVLinkField("vx", "float", "X velocity", units="m/s"),
            MAVLinkField("vy", "float", "Y velocity", units="m/s"),
            MAVLinkField("vz", "float", "Z velocity", units="m/s"),
        ]
    ),
    MAVLinkMessage(
        name="GLOBAL_POSITION_INT", id=33, category="Position",
        description="Global position (lat/lon/alt) as integers",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("lat", "int32_t", "Latitude (WGS84)", units="degE7"),
            MAVLinkField("lon", "int32_t", "Longitude (WGS84)", units="degE7"),
            MAVLinkField("alt", "int32_t", "Altitude (MSL)", units="mm"),
            MAVLinkField("relative_alt", "int32_t", "Altitude above home", units="mm"),
            MAVLinkField("vx", "int16_t", "Ground X speed", units="cm/s"),
            MAVLinkField("vy", "int16_t", "Ground Y speed", units="cm/s"),
            MAVLinkField("vz", "int16_t", "Ground Z speed", units="cm/s"),
            MAVLinkField("hdg", "uint16_t", "Heading", units="cdeg"),
        ]
    ),
    MAVLinkMessage(
        name="GPS_RAW_INT", id=24, category="Position",
        description="Raw GPS data",
        fields=[
            MAVLinkField("time_usec", "uint64_t", "Timestamp", units="us"),
            MAVLinkField("fix_type", "uint8_t", "GPS fix type", enum="GPS_FIX_TYPE"),
            MAVLinkField("lat", "int32_t", "Latitude (WGS84)", units="degE7"),
            MAVLinkField("lon", "int32_t", "Longitude (WGS84)", units="degE7"),
            MAVLinkField("alt", "int32_t", "Altitude (MSL)", units="mm"),
            MAVLinkField("eph", "uint16_t", "GPS HDOP", units="cm"),
            MAVLinkField("epv", "uint16_t", "GPS VDOP", units="cm"),
            MAVLinkField("vel", "uint16_t", "GPS ground speed", units="cm/s"),
            MAVLinkField("cog", "uint16_t", "Course over ground", units="cdeg"),
            MAVLinkField("satellites_visible", "uint8_t", "Number of satellites"),
        ]
    ),

    # Navigation
    MAVLinkMessage(
        name="NAV_CONTROLLER_OUTPUT", id=62, category="Navigation",
        description="Navigation controller output",
        fields=[
            MAVLinkField("nav_roll", "float", "Desired roll", units="deg"),
            MAVLinkField("nav_pitch", "float", "Desired pitch", units="deg"),
            MAVLinkField("nav_bearing", "int16_t", "Current bearing", units="deg"),
            MAVLinkField("target_bearing", "int16_t", "Target bearing", units="deg"),
            MAVLinkField("wp_dist", "uint16_t", "Distance to waypoint", units="m"),
            MAVLinkField("alt_error", "float", "Altitude error", units="m"),
            MAVLinkField("aspd_error", "float", "Airspeed error", units="m/s"),
            MAVLinkField("xtrack_error", "float", "Cross-track error", units="m"),
        ]
    ),
    MAVLinkMessage(
        name="MISSION_CURRENT", id=42, category="Navigation",
        description="Current mission item index",
        fields=[
            MAVLinkField("seq", "uint16_t", "Current mission item sequence number"),
        ]
    ),
    MAVLinkMessage(
        name="MISSION_ITEM_REACHED", id=46, category="Navigation",
        description="Notification when a mission item is reached",
        fields=[
            MAVLinkField("seq", "uint16_t", "Sequence number of reached item"),
        ]
    ),

    # RC and Control
    MAVLinkMessage(
        name="RC_CHANNELS", id=65, category="RC",
        description="RC channel values",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("chancount", "uint8_t", "Total number of RC channels"),
            MAVLinkField("chan1_raw", "uint16_t", "RC channel 1 value", units="us"),
            MAVLinkField("chan2_raw", "uint16_t", "RC channel 2 value", units="us"),
            MAVLinkField("chan3_raw", "uint16_t", "RC channel 3 value", units="us"),
            MAVLinkField("chan4_raw", "uint16_t", "RC channel 4 value", units="us"),
            MAVLinkField("chan5_raw", "uint16_t", "RC channel 5 value", units="us"),
            MAVLinkField("chan6_raw", "uint16_t", "RC channel 6 value", units="us"),
            MAVLinkField("chan7_raw", "uint16_t", "RC channel 7 value", units="us"),
            MAVLinkField("chan8_raw", "uint16_t", "RC channel 8 value", units="us"),
            MAVLinkField("rssi", "uint8_t", "Signal strength (0-255)"),
        ]
    ),
    MAVLinkMessage(
        name="RC_CHANNELS_OVERRIDE", id=70, category="RC",
        description="Override RC channels from GCS",
        fields=[
            MAVLinkField("target_system", "uint8_t", "Target system ID"),
            MAVLinkField("target_component", "uint8_t", "Target component ID"),
            MAVLinkField("chan1_raw", "uint16_t", "RC channel 1 override", units="us"),
            MAVLinkField("chan2_raw", "uint16_t", "RC channel 2 override", units="us"),
            MAVLinkField("chan3_raw", "uint16_t", "RC channel 3 override", units="us"),
            MAVLinkField("chan4_raw", "uint16_t", "RC channel 4 override", units="us"),
        ]
    ),
    MAVLinkMessage(
        name="SERVO_OUTPUT_RAW", id=36, category="RC",
        description="Servo/motor output values",
        fields=[
            MAVLinkField("time_usec", "uint32_t", "Timestamp", units="us"),
            MAVLinkField("port", "uint8_t", "Servo output port (0-based)"),
            MAVLinkField("servo1_raw", "uint16_t", "Servo 1 output", units="us"),
            MAVLinkField("servo2_raw", "uint16_t", "Servo 2 output", units="us"),
            MAVLinkField("servo3_raw", "uint16_t", "Servo 3 output", units="us"),
            MAVLinkField("servo4_raw", "uint16_t", "Servo 4 output", units="us"),
        ]
    ),

    # Parameters
    MAVLinkMessage(
        name="PARAM_VALUE", id=22, category="Parameters",
        description="Parameter value response",
        fields=[
            MAVLinkField("param_id", "char[16]", "Parameter name (null-terminated)"),
            MAVLinkField("param_value", "float", "Parameter value"),
            MAVLinkField("param_type", "uint8_t", "Parameter type", enum="MAV_PARAM_TYPE"),
            MAVLinkField("param_count", "uint16_t", "Total parameter count"),
            MAVLinkField("param_index", "uint16_t", "Index of this parameter"),
        ]
    ),
    MAVLinkMessage(
        name="PARAM_SET", id=23, category="Parameters",
        description="Set parameter request",
        fields=[
            MAVLinkField("target_system", "uint8_t", "Target system ID"),
            MAVLinkField("target_component", "uint8_t", "Target component ID"),
            MAVLinkField("param_id", "char[16]", "Parameter name (null-terminated)"),
            MAVLinkField("param_value", "float", "Parameter value"),
            MAVLinkField("param_type", "uint8_t", "Parameter type", enum="MAV_PARAM_TYPE"),
        ]
    ),

    # Commands
    MAVLinkMessage(
        name="COMMAND_LONG", id=76, category="Commands",
        description="Long command (7 float params)",
        fields=[
            MAVLinkField("target_system", "uint8_t", "Target system ID"),
            MAVLinkField("target_component", "uint8_t", "Target component ID"),
            MAVLinkField("command", "uint16_t", "Command ID", enum="MAV_CMD"),
            MAVLinkField("confirmation", "uint8_t", "Confirmation (0=first, 1+=retry)"),
            MAVLinkField("param1", "float", "Parameter 1"),
            MAVLinkField("param2", "float", "Parameter 2"),
            MAVLinkField("param3", "float", "Parameter 3"),
            MAVLinkField("param4", "float", "Parameter 4"),
            MAVLinkField("param5", "float", "Parameter 5 (often lat)"),
            MAVLinkField("param6", "float", "Parameter 6 (often lon)"),
            MAVLinkField("param7", "float", "Parameter 7 (often alt)"),
        ]
    ),
    MAVLinkMessage(
        name="COMMAND_ACK", id=77, category="Commands",
        description="Command acknowledgement",
        fields=[
            MAVLinkField("command", "uint16_t", "Command ID", enum="MAV_CMD"),
            MAVLinkField("result", "uint8_t", "Result", enum="MAV_RESULT"),
            MAVLinkField("progress", "uint8_t", "Progress (0-100)", units="%"),
            MAVLinkField("result_param2", "int32_t", "Additional result info"),
        ]
    ),

    # Telemetry
    MAVLinkMessage(
        name="VFR_HUD", id=74, category="Telemetry",
        description="VFR HUD data for pilot display",
        fields=[
            MAVLinkField("airspeed", "float", "Airspeed", units="m/s"),
            MAVLinkField("groundspeed", "float", "Ground speed", units="m/s"),
            MAVLinkField("heading", "int16_t", "Heading", units="deg"),
            MAVLinkField("throttle", "uint16_t", "Throttle", units="%"),
            MAVLinkField("alt", "float", "Altitude (MSL)", units="m"),
            MAVLinkField("climb", "float", "Climb rate", units="m/s"),
        ]
    ),
    MAVLinkMessage(
        name="ALTITUDE", id=141, category="Telemetry",
        description="Various altitude measurements",
        fields=[
            MAVLinkField("time_usec", "uint64_t", "Timestamp", units="us"),
            MAVLinkField("altitude_monotonic", "float", "Monotonic altitude", units="m"),
            MAVLinkField("altitude_amsl", "float", "Altitude above MSL", units="m"),
            MAVLinkField("altitude_local", "float", "Local altitude", units="m"),
            MAVLinkField("altitude_relative", "float", "Altitude above home", units="m"),
            MAVLinkField("altitude_terrain", "float", "Altitude above terrain", units="m"),
            MAVLinkField("bottom_clearance", "float", "Distance to ground", units="m"),
        ]
    ),
    MAVLinkMessage(
        name="BATTERY_STATUS", id=147, category="Telemetry",
        description="Battery status",
        fields=[
            MAVLinkField("id", "uint8_t", "Battery ID"),
            MAVLinkField("battery_function", "uint8_t", "Function", enum="MAV_BATTERY_FUNCTION"),
            MAVLinkField("type", "uint8_t", "Battery type", enum="MAV_BATTERY_TYPE"),
            MAVLinkField("temperature", "int16_t", "Temperature", units="cdegC"),
            MAVLinkField("voltages", "uint16_t[10]", "Cell voltages", units="mV"),
            MAVLinkField("current_battery", "int16_t", "Current", units="cA"),
            MAVLinkField("current_consumed", "int32_t", "Consumed charge", units="mAh"),
            MAVLinkField("energy_consumed", "int32_t", "Consumed energy", units="hJ"),
            MAVLinkField("battery_remaining", "int8_t", "Remaining", units="%"),
        ]
    ),

    # Sensors
    MAVLinkMessage(
        name="SCALED_IMU", id=26, category="Sensors",
        description="Scaled IMU data (accelerometer, gyro, magnetometer)",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("xacc", "int16_t", "X acceleration", units="mG"),
            MAVLinkField("yacc", "int16_t", "Y acceleration", units="mG"),
            MAVLinkField("zacc", "int16_t", "Z acceleration", units="mG"),
            MAVLinkField("xgyro", "int16_t", "X angular speed", units="mrad/s"),
            MAVLinkField("ygyro", "int16_t", "Y angular speed", units="mrad/s"),
            MAVLinkField("zgyro", "int16_t", "Z angular speed", units="mrad/s"),
            MAVLinkField("xmag", "int16_t", "X magnetic field", units="mgauss"),
            MAVLinkField("ymag", "int16_t", "Y magnetic field", units="mgauss"),
            MAVLinkField("zmag", "int16_t", "Z magnetic field", units="mgauss"),
        ]
    ),
    MAVLinkMessage(
        name="SCALED_PRESSURE", id=29, category="Sensors",
        description="Scaled pressure data",
        fields=[
            MAVLinkField("time_boot_ms", "uint32_t", "Time since boot", units="ms"),
            MAVLinkField("press_abs", "float", "Absolute pressure", units="hPa"),
            MAVLinkField("press_diff", "float", "Differential pressure", units="hPa"),
            MAVLinkField("temperature", "int16_t", "Temperature", units="cdegC"),
        ]
    ),
    MAVLinkMessage(
        name="WIND", id=168, category="Sensors",
        description="Wind estimation (ArduPilot)",
        fields=[
            MAVLinkField("direction", "float", "Wind direction", units="deg"),
            MAVLinkField("speed", "float", "Wind speed", units="m/s"),
            MAVLinkField("speed_z", "float", "Vertical wind speed", units="m/s"),
        ]
    ),

    # Status
    MAVLinkMessage(
        name="STATUSTEXT", id=253, category="Status",
        description="Status text message",
        fields=[
            MAVLinkField("severity", "uint8_t", "Severity", enum="MAV_SEVERITY"),
            MAVLinkField("text", "char[50]", "Status text message"),
        ]
    ),
    MAVLinkMessage(
        name="EXTENDED_SYS_STATE", id=245, category="Status",
        description="Extended system state (VTOL, landed)",
        fields=[
            MAVLinkField("vtol_state", "uint8_t", "VTOL state", enum="MAV_VTOL_STATE"),
            MAVLinkField("landed_state", "uint8_t", "Landed state", enum="MAV_LANDED_STATE"),
        ]
    ),

    # High Latency
    MAVLinkMessage(
        name="HIGH_LATENCY", id=234, category="High Latency",
        description="Summary message for high-latency links",
        fields=[
            MAVLinkField("base_mode", "uint8_t", "System mode", enum="MAV_MODE_FLAG"),
            MAVLinkField("custom_mode", "uint32_t", "Custom mode"),
            MAVLinkField("landed_state", "uint8_t", "Landed state", enum="MAV_LANDED_STATE"),
            MAVLinkField("roll", "int16_t", "Roll", units="cdeg"),
            MAVLinkField("pitch", "int16_t", "Pitch", units="cdeg"),
            MAVLinkField("heading", "uint16_t", "Heading", units="cdeg"),
            MAVLinkField("throttle", "int8_t", "Throttle", units="%"),
            MAVLinkField("heading_sp", "int16_t", "Heading setpoint", units="cdeg"),
            MAVLinkField("latitude", "int32_t", "Latitude", units="degE7"),
            MAVLinkField("longitude", "int32_t", "Longitude", units="degE7"),
            MAVLinkField("altitude_amsl", "int16_t", "Altitude MSL", units="m"),
            MAVLinkField("altitude_sp", "int16_t", "Altitude setpoint", units="m"),
            MAVLinkField("airspeed", "uint8_t", "Airspeed", units="m/s"),
            MAVLinkField("airspeed_sp", "uint8_t", "Airspeed setpoint", units="m/s"),
            MAVLinkField("groundspeed", "uint8_t", "Ground speed", units="m/s"),
            MAVLinkField("climb_rate", "int8_t", "Climb rate", units="m/s"),
            MAVLinkField("gps_nsat", "uint8_t", "GPS satellites"),
            MAVLinkField("gps_fix_type", "uint8_t", "GPS fix type", enum="GPS_FIX_TYPE"),
            MAVLinkField("battery_remaining", "uint8_t", "Battery remaining", units="%"),
            MAVLinkField("temperature", "int8_t", "Autopilot temperature", units="degC"),
            MAVLinkField("temperature_air", "int8_t", "Air temperature", units="degC"),
        ]
    ),
    MAVLinkMessage(
        name="HIGH_LATENCY2", id=235, category="High Latency",
        description="Summary message for high-latency links v2",
        fields=[
            MAVLinkField("timestamp", "uint32_t", "Timestamp", units="ms"),
            MAVLinkField("type", "uint8_t", "Vehicle type", enum="MAV_TYPE"),
            MAVLinkField("autopilot", "uint8_t", "Autopilot type", enum="MAV_AUTOPILOT"),
            MAVLinkField("custom_mode", "uint16_t", "Custom mode"),
            MAVLinkField("latitude", "int32_t", "Latitude", units="degE7"),
            MAVLinkField("longitude", "int32_t", "Longitude", units="degE7"),
            MAVLinkField("altitude", "int16_t", "Altitude MSL", units="m"),
            MAVLinkField("target_altitude", "int16_t", "Target altitude", units="m"),
            MAVLinkField("heading", "uint8_t", "Heading", units="deg/2"),
            MAVLinkField("target_heading", "uint8_t", "Target heading", units="deg/2"),
            MAVLinkField("target_distance", "uint16_t", "Target distance", units="dam"),
        ]
    ),
]

# Build lookup dictionaries for fast access
MESSAGES_BY_NAME: dict[str, MAVLinkMessage] = {msg.name: msg for msg in MAVLINK_MESSAGES}
MESSAGES_BY_ID: dict[int, MAVLinkMessage] = {msg.id: msg for msg in MAVLINK_MESSAGES}


def get_message(name_or_id: str | int) -> Optional[MAVLinkMessage]:
    """Get a MAVLink message by name or ID."""
    if isinstance(name_or_id, int):
        return MESSAGES_BY_ID.get(name_or_id)
    return MESSAGES_BY_NAME.get(name_or_id.upper())


def get_messages_by_category(category: str) -> list[MAVLinkMessage]:
    """Get all messages in a category."""
    return [msg for msg in MAVLINK_MESSAGES if msg.category == category]


def get_categories() -> list[str]:
    """Get list of all message categories."""
    return sorted(set(msg.category for msg in MAVLINK_MESSAGES if msg.category))


# Dynamic loading support
_dynamic_messages: Optional[list[MAVLinkMessage]] = None
_dynamic_loaded = False


def load_messages_from_xml(project_root: Path) -> list[MAVLinkMessage]:
    """Load MAVLink messages from XML definitions if available.

    Args:
        project_root: Root directory of the QGC project

    Returns:
        List of MAVLinkMessage objects from XML, or empty list if not found
    """
    try:
        from .mavlink_parser import load_all_messages
        return load_all_messages(project_root)
    except ImportError:
        logger.debug("mavlink_parser not available")
        return []
    except Exception as e:
        logger.warning(f"Failed to load MAVLink XML: {e}")
        return []


def get_all_messages(project_root: Optional[Path] = None) -> list[MAVLinkMessage]:
    """Get all available MAVLink messages.

    Tries to load from XML definitions first, falls back to hardcoded messages.

    Args:
        project_root: Optional project root for XML loading

    Returns:
        List of MAVLinkMessage objects
    """
    global _dynamic_messages, _dynamic_loaded

    # Try dynamic loading once if project root provided
    if project_root is not None and not _dynamic_loaded:
        _dynamic_loaded = True
        _dynamic_messages = load_messages_from_xml(project_root)
        if _dynamic_messages:
            logger.info(f"Using {len(_dynamic_messages)} messages from MAVLink XML")

    # Return dynamic messages if available, otherwise fallback
    if _dynamic_messages:
        return _dynamic_messages

    return MAVLINK_MESSAGES


def reset_dynamic_messages():
    """Reset dynamic message cache (for testing)."""
    global _dynamic_messages, _dynamic_loaded
    _dynamic_messages = None
    _dynamic_loaded = False


def get_message_by_name(name: str, project_root: Optional[Path] = None) -> Optional[MAVLinkMessage]:
    """Get a message by name, checking dynamic messages first."""
    messages = get_all_messages(project_root)
    for msg in messages:
        if msg.name == name.upper():
            return msg
    return None
