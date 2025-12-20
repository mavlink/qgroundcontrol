"""
FactGroup Generator for QGroundControl

Generates all boilerplate files for a new FactGroup:
- VehicleXxxFactGroup.h
- VehicleXxxFactGroup.cc
- XxxFact.json
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

try:
    from jinja2 import Environment, FileSystemLoader
except ImportError:
    raise ImportError("Jinja2 is required. Install with: pip install jinja2")


@dataclass
class FactSpec:
    """Specification for a single Fact."""
    name: str
    value_type: str  # double, float, uint32, int8, string, bool, etc.
    units: str = ""
    short_desc: str = ""
    decimal_places: int = 2
    min_value: Optional[float] = None
    max_value: Optional[float] = None

    @property
    def cpp_type(self) -> str:
        """Get the C++ FactMetaData type constant."""
        type_map = {
            'double': 'FactMetaData::valueTypeDouble',
            'float': 'FactMetaData::valueTypeFloat',
            'uint8': 'FactMetaData::valueTypeUint8',
            'uint16': 'FactMetaData::valueTypeUint16',
            'uint32': 'FactMetaData::valueTypeUint32',
            'uint64': 'FactMetaData::valueTypeUint64',
            'int8': 'FactMetaData::valueTypeInt8',
            'int16': 'FactMetaData::valueTypeInt16',
            'int32': 'FactMetaData::valueTypeInt32',
            'int64': 'FactMetaData::valueTypeInt64',
            'string': 'FactMetaData::valueTypeString',
            'bool': 'FactMetaData::valueTypeBool',
        }
        return type_map.get(self.value_type, 'FactMetaData::valueTypeDouble')


@dataclass
class MavlinkMessageSpec:
    """Specification for a MAVLink message handler."""
    message_id: str  # e.g., "GPS_RAW_INT"

    @property
    def handler_name(self) -> str:
        """Get the handler method name (e.g., _handleGpsRawInt)."""
        # Convert GPS_RAW_INT -> GpsRawInt
        parts = self.message_id.split('_')
        camel = ''.join(p.capitalize() for p in parts)
        return f"_handle{camel}"

    @property
    def struct_name(self) -> str:
        """Get the MAVLink struct name (e.g., mavlink_gps_raw_int_t)."""
        return f"mavlink_{self.message_id.lower()}_t"

    @property
    def decode_func(self) -> str:
        """Get the MAVLink decode function name."""
        return f"mavlink_msg_{self.message_id.lower()}_decode"

    @property
    def msg_id_constant(self) -> str:
        """Get the MAVLink message ID constant."""
        return f"MAVLINK_MSG_ID_{self.message_id}"


@dataclass
class FactGroupSpec:
    """Complete specification for a FactGroup."""
    domain: str  # e.g., "Wind" -> VehicleWindFactGroup
    facts: list[FactSpec] = field(default_factory=list)
    mavlink_messages: list[MavlinkMessageSpec] = field(default_factory=list)
    update_rate_ms: int = 1000

    @property
    def class_name(self) -> str:
        return f"Vehicle{self.domain}FactGroup"

    @property
    def header_filename(self) -> str:
        return f"{self.class_name}.h"

    @property
    def source_filename(self) -> str:
        return f"{self.class_name}.cc"

    @property
    def json_filename(self) -> str:
        return f"{self.domain}Fact.json"

    @property
    def json_resource_path(self) -> str:
        return f":/json/Vehicle/{self.json_filename}"

    @property
    def factgroup_name(self) -> str:
        """Name used for registration (lowercase first letter)."""
        return self.domain[0].lower() + self.domain[1:]


class FactGroupGenerator:
    """Generate FactGroup files from specification."""

    def __init__(self, spec: FactGroupSpec, output_dir: Path):
        self.spec = spec
        self.output_dir = output_dir
        template_dir = Path(__file__).parent / 'templates'
        self.env = Environment(
            loader=FileSystemLoader(template_dir),
            trim_blocks=True,
            lstrip_blocks=True,
        )

    def generate_header(self) -> str:
        """Generate the header file content."""
        template = self.env.get_template('header.h.j2')
        return template.render(spec=self.spec)

    def generate_source(self) -> str:
        """Generate the source file content."""
        template = self.env.get_template('source.cc.j2')
        return template.render(spec=self.spec)

    def generate_json(self) -> str:
        """Generate the JSON metadata file content."""
        template = self.env.get_template('metadata.json.j2')
        return template.render(spec=self.spec)

    def generate_all(self, dry_run: bool = False) -> dict[str, str]:
        """
        Generate all files.

        Args:
            dry_run: If True, don't write files, just return content.

        Returns:
            Dictionary mapping filename to generated content.
        """
        files = {
            self.spec.header_filename: self.generate_header(),
            self.spec.source_filename: self.generate_source(),
            self.spec.json_filename: self.generate_json(),
        }

        if not dry_run:
            self.output_dir.mkdir(parents=True, exist_ok=True)
            for filename, content in files.items():
                file_path = self.output_dir / filename
                file_path.write_text(content)
                print(f"Generated: {file_path}")

        return files


def parse_facts_string(facts_str: str) -> list[FactSpec]:
    """
    Parse facts specification string.

    Format: "name:type:units,name:type:units,..."

    Examples:
        "temp:double:degC,pressure:double:Pa"
        "direction:double:deg,speed:double:m/s"
    """
    facts = []
    for fact_str in facts_str.split(','):
        parts = fact_str.strip().split(':')
        if len(parts) >= 2:
            name = parts[0].strip()
            value_type = parts[1].strip()
            units = parts[2].strip() if len(parts) > 2 else ""
            facts.append(FactSpec(
                name=name,
                value_type=value_type,
                units=units,
                short_desc=name.replace('_', ' ').title(),
            ))
    return facts


def parse_mavlink_string(mavlink_str: str) -> list[MavlinkMessageSpec]:
    """
    Parse MAVLink messages string.

    Format: "MSG_ID1,MSG_ID2,..."

    Examples:
        "WIND_COV,HIGH_LATENCY2"
        "GPS_RAW_INT,GPS_STATUS"
    """
    messages = []
    for msg_str in mavlink_str.split(','):
        msg_id = msg_str.strip().upper()
        if msg_id:
            messages.append(MavlinkMessageSpec(message_id=msg_id))
    return messages
