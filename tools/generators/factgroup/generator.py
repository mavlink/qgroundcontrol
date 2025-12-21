"""
FactGroup Generator for QGroundControl

Generates all boilerplate files for a new FactGroup:
- VehicleXxxFactGroup.h
- VehicleXxxFactGroup.cc
- XxxFact.json

Supports both CLI arguments and YAML spec files.
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Any
import json

try:
    from jinja2 import Environment, FileSystemLoader
except ImportError:
    raise ImportError("Jinja2 is required. Install with: pip install jinja2")

try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False


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
class FieldMapping:
    """Mapping from MAVLink message field to Fact."""
    fact_name: str          # Name of the Fact to update (e.g., "speed")
    source_field: str       # MAVLink struct field name (e.g., "vel")
    scaling: str = ""       # Scaling expression (e.g., "/ 100.0", "* 1e-7")
    transform: str = ""     # Full transform (e.g., "qRadiansToDegrees({field})")
    # If transform is set, it overrides source_field + scaling


@dataclass
class MavlinkMessageSpec:
    """Specification for a MAVLink message handler."""
    message_id: str                                     # e.g., "GPS_RAW_INT"
    field_mappings: list[FieldMapping] = field(default_factory=list)
    dialect: str = ""                                   # e.g., "ardupilot" for #ifndef guards

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

    @property
    def local_var_name(self) -> str:
        """Get the local variable name for decoded struct."""
        # Convert GPS_RAW_INT -> gpsRawInt
        parts = self.message_id.lower().split('_')
        return parts[0] + ''.join(p.capitalize() for p in parts[1:])

    @property
    def is_ardupilot_dialect(self) -> bool:
        """Check if this message requires ArduPilot dialect guard."""
        return self.dialect.lower() == "ardupilot"


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


def parse_field_mapping(mapping_data: dict[str, Any]) -> FieldMapping:
    """Parse a field mapping from spec data."""
    return FieldMapping(
        fact_name=mapping_data['fact'],
        source_field=mapping_data.get('field', mapping_data['fact']),  # Default to fact name
        scaling=mapping_data.get('scaling', ''),
        transform=mapping_data.get('transform', ''),
    )


def load_spec_from_file(spec_path: Path) -> FactGroupSpec:
    """
    Load FactGroup specification from a YAML or JSON file.

    Args:
        spec_path: Path to spec file (.yaml, .yml, or .json)

    Returns:
        FactGroupSpec parsed from the file

    Example YAML spec:
        domain: Wind
        update_rate_ms: 1000
        facts:
          - name: direction
            type: double
            units: deg
            short_desc: Wind direction
            decimal_places: 1
          - name: speed
            type: double
            units: m/s
        mavlink_messages:
          - WIND_COV
          - HIGH_LATENCY2
    """
    content = spec_path.read_text()

    if spec_path.suffix in ('.yaml', '.yml'):
        if not HAS_YAML:
            raise ImportError("PyYAML is required for YAML specs. Install with: pip install pyyaml")
        data = yaml.safe_load(content)
    elif spec_path.suffix == '.json':
        data = json.loads(content)
    else:
        raise ValueError(f"Unsupported spec file format: {spec_path.suffix}")

    return parse_spec_dict(data)


def parse_spec_dict(data: dict[str, Any]) -> FactGroupSpec:
    """
    Parse a specification dictionary into a FactGroupSpec.

    Args:
        data: Dictionary with spec data

    Returns:
        FactGroupSpec instance
    """
    facts = []
    for fact_data in data.get('facts', []):
        facts.append(FactSpec(
            name=fact_data['name'],
            value_type=fact_data.get('type', 'double'),
            units=fact_data.get('units', ''),
            short_desc=fact_data.get('short_desc', fact_data['name'].replace('_', ' ').title()),
            decimal_places=fact_data.get('decimal_places', 2),
            min_value=fact_data.get('min'),
            max_value=fact_data.get('max'),
        ))

    mavlink_messages = []
    for msg in data.get('mavlink_messages', []):
        if isinstance(msg, str):
            mavlink_messages.append(MavlinkMessageSpec(message_id=msg.upper()))
        elif isinstance(msg, dict):
            # Parse field mappings if present
            field_mappings = []
            for mapping_data in msg.get('mappings', []):
                field_mappings.append(parse_field_mapping(mapping_data))

            mavlink_messages.append(MavlinkMessageSpec(
                message_id=msg['id'].upper(),
                field_mappings=field_mappings,
                dialect=msg.get('dialect', ''),
            ))

    return FactGroupSpec(
        domain=data['domain'],
        facts=facts,
        mavlink_messages=mavlink_messages,
        update_rate_ms=data.get('update_rate_ms', 1000),
    )


def validate_spec(spec: FactGroupSpec) -> list[str]:
    """
    Validate a FactGroupSpec for common errors.

    Args:
        spec: The specification to validate

    Returns:
        List of error messages (empty if valid)
    """
    errors = []

    if not spec.domain:
        errors.append("Domain name is required")

    if not spec.facts:
        errors.append("At least one fact is required")

    # Check for duplicate fact names
    names = [f.name for f in spec.facts]
    duplicates = [n for n in names if names.count(n) > 1]
    if duplicates:
        errors.append(f"Duplicate fact names: {set(duplicates)}")

    # Validate fact types
    valid_types = {'double', 'float', 'uint8', 'uint16', 'uint32', 'uint64',
                   'int8', 'int16', 'int32', 'int64', 'string', 'bool'}
    for fact in spec.facts:
        if fact.value_type not in valid_types:
            errors.append(f"Invalid type '{fact.value_type}' for fact '{fact.name}'")

    return errors
