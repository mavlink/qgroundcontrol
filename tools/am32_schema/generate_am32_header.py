#!/usr/bin/env python3
"""
Generate AM32 eeprom.h from the unified JSON schema.

This script generates the C header file that defines the EEprom_t union
used by the AM32 firmware. It ensures the struct layout matches the schema.

Usage:
    python generate_am32_header.py am32-eeprom-schema.json > eeprom.h
"""

import json
import sys
from datetime import datetime
from typing import Any


def load_schema(path: str) -> dict:
    with open(path, 'r') as f:
        return json.load(f)


def get_c_type(field: dict) -> str:
    """Map schema type to C type."""
    field_type = field.get('type', 'uint8')
    if field_type in ('bool', 'uint8', 'enum'):
        return 'uint8_t'
    elif field_type == 'int8':
        return 'int8_t'
    elif field_type == 'uint16':
        return 'uint16_t'
    elif field_type == 'number':
        return 'uint8_t'  # Numbers are stored as uint8 in EEPROM
    elif field_type == 'rtttl':
        return 'uint8_t'  # Array type, handled separately
    return 'uint8_t'


def generate_header(schema: dict) -> str:
    """Generate the C header file content."""
    fields = schema['fields']

    # Sort fields by offset
    sorted_fields = sorted(
        [(name, f) for name, f in fields.items()],
        key=lambda x: x[1]['offset']
    )

    lines = []
    lines.append(f"// AUTO-GENERATED from am32-eeprom-schema.json - DO NOT EDIT")
    lines.append(f"// Generated: {datetime.now().isoformat()}")
    lines.append(f"// Schema version: {schema.get('version', 'unknown')}")
    lines.append("")
    lines.append("#include \"main.h\"")
    lines.append("")
    lines.append("#pragma once")
    lines.append("")
    lines.append("typedef union EEprom_u {")
    lines.append("    struct {")

    current_offset = 0
    reserved_count = 0

    for name, field in sorted_fields:
        offset = field['offset']
        size = field.get('size', 1)
        c_type = get_c_type(field)
        description = field.get('description', '')

        # Handle gaps with reserved fields
        if offset > current_offset:
            gap = offset - current_offset
            lines.append(f"        char reserved_{reserved_count}[{gap}]; //{current_offset}-{offset-1}")
            reserved_count += 1

        # Convert camelCase to snake_case for C
        c_name = ''.join(['_' + c.lower() if c.isupper() else c for c in name]).lstrip('_')

        # Handle array types
        if size > 1:
            if field.get('type') == 'rtttl':
                lines.append(f"        uint8_t {c_name}[{size}]; //{offset} {description}")
            else:
                lines.append(f"        uint8_t {c_name}[{size}]; //{offset}")
        else:
            # Add version info if present
            version_info = ""
            if 'minEepromVersion' in field:
                version_info += f" (EEPROM v{field['minEepromVersion']}+)"

            lines.append(f"        {c_type} {c_name}; // {offset} {description}{version_info}")

        current_offset = offset + size

    # Get max EEPROM size from versions
    max_size = max(v.get('size', 48) for v in schema.get('eepromVersions', {}).values())

    lines.append("    };")
    lines.append(f"    uint8_t buffer[{max_size}];")
    lines.append("} EEprom_t;")
    lines.append("")
    lines.append("extern EEprom_t eepromBuffer;")
    lines.append("")
    lines.append("void read_flash_bin(uint8_t* data, uint32_t add, int out_buff_len);")
    lines.append("void save_flash_nolib(uint8_t* data, int length, uint32_t add);")
    lines.append("")

    return '\n'.join(lines)


def generate_offsets_header(schema: dict) -> str:
    """Generate a header with offset constants for validation."""
    fields = schema['fields']

    lines = []
    lines.append(f"// AM32 EEPROM offset constants - AUTO-GENERATED")
    lines.append(f"// Use these for compile-time validation")
    lines.append("")
    lines.append("#pragma once")
    lines.append("")

    for name, field in sorted(fields.items(), key=lambda x: x[1]['offset']):
        const_name = ''.join(['_' + c.lower() if c.isupper() else c for c in name]).lstrip('_').upper()
        lines.append(f"#define EEPROM_OFFSET_{const_name} {field['offset']}")

    lines.append("")
    return '\n'.join(lines)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <schema.json> [--offsets]", file=sys.stderr)
        sys.exit(1)

    schema = load_schema(sys.argv[1])

    if '--offsets' in sys.argv:
        print(generate_offsets_header(schema))
    else:
        print(generate_header(schema))


if __name__ == '__main__':
    main()
