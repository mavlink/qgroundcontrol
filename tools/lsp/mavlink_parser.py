"""
MAVLink XML definition parser.

Parses MAVLink message definitions from XML files to extract message
metadata for IDE completion features.

The parser handles the MAVLink XML format including:
- Message definitions with fields
- Include directives (for dialect inheritance)
- Enum definitions
- Field units and types
"""

import logging
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from .mavlink_data import MAVLinkField, MAVLinkMessage

logger = logging.getLogger(__name__)


@dataclass
class MAVLinkDialect:
    """A parsed MAVLink dialect (XML file)."""
    name: str
    version: int
    messages: list[MAVLinkMessage] = field(default_factory=list)
    enums: dict[str, list[tuple[str, int, str]]] = field(default_factory=dict)
    includes: list[str] = field(default_factory=list)


def find_mavlink_definitions(project_root: Path) -> Optional[Path]:
    """Find the MAVLink message_definitions directory in a project.

    Searches common locations:
    - build/cpm_modules/mavlink/*/message_definitions/v1.0/
    - libs/mavlink/message_definitions/v1.0/
    - submodules/mavlink/message_definitions/v1.0/
    """
    search_patterns = [
        "build/cpm_modules/mavlink/*/message_definitions/v1.0",
        "libs/mavlink/message_definitions/v1.0",
        "submodules/mavlink/message_definitions/v1.0",
        "mavlink/message_definitions/v1.0",
    ]

    for pattern in search_patterns:
        import glob
        matches = glob.glob(str(project_root / pattern))
        for match in matches:
            path = Path(match)
            if path.is_dir() and (path / "common.xml").exists():
                logger.info(f"Found MAVLink definitions at: {path}")
                return path

    return None


def parse_mavlink_xml(xml_path: Path, definitions_dir: Optional[Path] = None) -> MAVLinkDialect:
    """Parse a MAVLink XML file and its includes.

    Args:
        xml_path: Path to the XML file to parse
        definitions_dir: Directory containing XML files (for resolving includes)

    Returns:
        MAVLinkDialect with parsed messages and enums
    """
    if definitions_dir is None:
        definitions_dir = xml_path.parent

    dialect = MAVLinkDialect(name=xml_path.stem, version=0)

    try:
        tree = ET.parse(xml_path)
        root = tree.getroot()
    except ET.ParseError as e:
        logger.error(f"Failed to parse {xml_path}: {e}")
        return dialect

    # Parse version
    version_elem = root.find("version")
    if version_elem is not None and version_elem.text:
        try:
            dialect.version = int(version_elem.text)
        except ValueError:
            pass  # Non-numeric version, use default 0

    # Parse includes (process these first to get base messages)
    for include in root.findall("include"):
        if include.text:
            dialect.includes.append(include.text)
            include_path = definitions_dir / include.text
            if include_path.exists():
                included = parse_mavlink_xml(include_path, definitions_dir)
                # Merge included messages (earlier messages can be overridden)
                existing_ids = {m.id for m in dialect.messages}
                for msg in included.messages:
                    if msg.id not in existing_ids:
                        dialect.messages.append(msg)
                # Merge enums
                dialect.enums.update(included.enums)

    # Parse enums
    enums_elem = root.find("enums")
    if enums_elem is not None:
        for enum_elem in enums_elem.findall("enum"):
            enum_name = enum_elem.get("name", "")
            entries = []
            for entry in enum_elem.findall("entry"):
                entry_name = entry.get("name", "")
                entry_value = int(entry.get("value", "0"))
                desc_elem = entry.find("description")
                entry_desc = desc_elem.text.strip() if desc_elem is not None and desc_elem.text else ""
                entries.append((entry_name, entry_value, entry_desc))
            if enum_name:
                dialect.enums[enum_name] = entries

    # Parse messages
    messages_elem = root.find("messages")
    if messages_elem is not None:
        for msg_elem in messages_elem.findall("message"):
            msg = _parse_message(msg_elem, dialect.enums)
            if msg:
                # Remove any existing message with same ID (override)
                dialect.messages = [m for m in dialect.messages if m.id != msg.id]
                dialect.messages.append(msg)

    # Sort messages by ID
    dialect.messages.sort(key=lambda m: m.id)

    return dialect


def _parse_message(msg_elem: ET.Element, enums: dict) -> Optional[MAVLinkMessage]:
    """Parse a single message element."""
    msg_id_str = msg_elem.get("id", "")
    msg_name = msg_elem.get("name", "")

    if not msg_id_str or not msg_name:
        return None

    try:
        msg_id = int(msg_id_str)
    except ValueError:
        return None

    # Get description
    desc_elem = msg_elem.find("description")
    description = ""
    if desc_elem is not None and desc_elem.text:
        # Clean up multi-line descriptions
        description = " ".join(desc_elem.text.split())

    # Parse fields (note: extensions marker separates core from extension fields)
    fields = []

    for child in msg_elem:
        if child.tag == "extensions":
            # Extension fields follow, but we include all fields for completion
            continue
        if child.tag == "field":
            field_obj = _parse_field(child)
            if field_obj:
                fields.append(field_obj)

    # Determine category from message ID ranges (approximate)
    category = _categorize_message(msg_id, msg_name)

    return MAVLinkMessage(
        name=msg_name,
        id=msg_id,
        description=description,
        fields=fields,
        category=category,
    )


def _parse_field(field_elem: ET.Element) -> Optional[MAVLinkField]:
    """Parse a single field element."""
    name = field_elem.get("name", "")
    field_type = field_elem.get("type", "")
    units = field_elem.get("units", "")
    enum = field_elem.get("enum", "")

    if not name or not field_type:
        return None

    # Get description from text content
    description = ""
    if field_elem.text:
        description = " ".join(field_elem.text.split())

    return MAVLinkField(
        name=name,
        type=field_type,
        description=description,
        units=units,
        enum=enum,
    )


def _categorize_message(msg_id: int, msg_name: str) -> str:
    """Categorize a message based on ID range and name patterns."""
    # Check name patterns first
    name_upper = msg_name.upper()

    if any(x in name_upper for x in ["HEARTBEAT", "SYS_STATUS", "SYSTEM", "PING", "AUTH"]):
        return "System"
    if any(x in name_upper for x in ["ATTITUDE", "QUATERNION"]):
        return "Attitude"
    if any(x in name_upper for x in ["GPS", "GLOBAL_POSITION", "LOCAL_POSITION", "POSITION"]):
        return "Position"
    if any(x in name_upper for x in ["NAV_", "MISSION", "WAYPOINT"]):
        return "Navigation"
    if any(x in name_upper for x in ["RC_CHANNEL", "SERVO", "ACTUATOR"]):
        return "RC"
    if any(x in name_upper for x in ["PARAM"]):
        return "Parameters"
    if any(x in name_upper for x in ["COMMAND"]):
        return "Commands"
    if any(x in name_upper for x in ["VFR_HUD", "ALTITUDE", "BATTERY", "POWER"]):
        return "Telemetry"
    if any(x in name_upper for x in ["IMU", "PRESSURE", "MAG", "SENSOR", "OPTICAL", "DISTANCE"]):
        return "Sensors"
    if any(x in name_upper for x in ["STATUSTEXT", "EXTENDED_SYS"]):
        return "Status"
    if any(x in name_upper for x in ["HIGH_LATENCY"]):
        return "High Latency"
    if any(x in name_upper for x in ["LOG", "FILE", "FTP"]):
        return "Logging"
    if any(x in name_upper for x in ["CAMERA", "VIDEO", "GIMBAL", "MOUNT"]):
        return "Camera"

    # Fallback to ID-based categorization
    if msg_id < 10:
        return "System"
    if 20 <= msg_id < 40:
        return "Parameters"
    if 40 <= msg_id < 60:
        return "Navigation"
    if 60 <= msg_id < 100:
        return "RC"
    if 100 <= msg_id < 150:
        return "Telemetry"

    return "Other"


def load_all_messages(project_root: Path) -> list[MAVLinkMessage]:
    """Load all MAVLink messages from the project's definitions.

    This is the main entry point for loading MAVLink data.
    It finds the definitions directory and parses common.xml (which
    includes all standard messages).

    Args:
        project_root: Root directory of the QGC project

    Returns:
        List of MAVLinkMessage objects
    """
    definitions_dir = find_mavlink_definitions(project_root)

    if definitions_dir is None:
        logger.warning("MAVLink definitions not found, using built-in messages")
        return []

    # Parse common.xml which includes standard.xml and minimal.xml
    common_xml = definitions_dir / "common.xml"
    if not common_xml.exists():
        logger.warning(f"common.xml not found at {common_xml}")
        return []

    dialect = parse_mavlink_xml(common_xml, definitions_dir)
    logger.info(f"Loaded {len(dialect.messages)} messages from MAVLink definitions")

    return dialect.messages
