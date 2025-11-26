#!/usr/bin/env python3
"""
Validate AM32 eeprom.h against the unified JSON schema.

This script parses the C header file and compares the struct offsets
against the schema definitions to catch any mismatches.

Usage:
    python validate_eeprom_h.py am32-eeprom-schema.json /path/to/eeprom.h
"""

import json
import re
import sys
from pathlib import Path


def load_schema(path: str) -> dict:
    with open(path, 'r') as f:
        return json.load(f)


def parse_eeprom_h(path: str) -> dict:
    """Parse eeprom.h and extract field offsets from comments."""
    with open(path, 'r') as f:
        content = f.read()

    # Extract fields with their offset comments
    # Pattern matches lines like: uint8_t field_name; //23
    pattern = r'^\s*(?:uint8_t|int8_t|char)\s+(\w+)(?:\[(\d+)\])?;\s*//\s*(\d+)'

    fields = {}
    for match in re.finditer(pattern, content, re.MULTILINE):
        name = match.group(1)
        size = int(match.group(2)) if match.group(2) else 1
        offset = int(match.group(3))
        fields[name] = {'offset': offset, 'size': size}

    return fields


def snake_to_camel(name: str) -> str:
    """Convert snake_case to camelCase."""
    components = name.split('_')
    return components[0].lower() + ''.join(x.title() for x in components[1:])


def camel_to_snake(name: str) -> str:
    """Convert camelCase to snake_case."""
    return ''.join(['_' + c.lower() if c.isupper() else c for c in name]).lstrip('_')


def validate(schema: dict, header_fields: dict) -> list:
    """Compare schema against parsed header fields."""
    errors = []
    warnings = []

    schema_fields = schema['fields']

    # Check each schema field against header
    for schema_name, schema_field in schema_fields.items():
        expected_offset = schema_field['offset']
        expected_size = schema_field.get('size', 1)

        # Try different name formats
        header_name = camel_to_snake(schema_name)

        if header_name not in header_fields:
            # Try some common variations
            variations = [
                header_name,
                header_name.replace('pid_', ''),  # e.g., currentPidP -> current_p
                schema_name.lower(),
            ]

            found = False
            for var in variations:
                if var in header_fields:
                    header_name = var
                    found = True
                    break

            if not found:
                warnings.append(f"Schema field '{schema_name}' not found in header (tried: {header_name})")
                continue

        header_field = header_fields[header_name]

        if header_field['offset'] != expected_offset:
            errors.append(
                f"Offset mismatch for '{schema_name}': "
                f"schema={expected_offset}, header={header_field['offset']}"
            )

        if header_field['size'] != expected_size:
            errors.append(
                f"Size mismatch for '{schema_name}': "
                f"schema={expected_size}, header={header_field['size']}"
            )

    # Check for header fields not in schema
    for header_name, header_field in header_fields.items():
        if header_name.startswith('reserved'):
            continue

        schema_name = snake_to_camel(header_name)
        if schema_name not in schema_fields:
            # Try some variations
            found = False
            for sname in schema_fields:
                if camel_to_snake(sname) == header_name:
                    found = True
                    break

            if not found:
                warnings.append(f"Header field '{header_name}' not in schema")

    return errors, warnings


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <schema.json> <eeprom.h>", file=sys.stderr)
        sys.exit(1)

    schema_path = sys.argv[1]
    header_path = sys.argv[2]

    schema = load_schema(schema_path)
    header_fields = parse_eeprom_h(header_path)

    print(f"Parsed {len(header_fields)} fields from {header_path}")
    print(f"Schema has {len(schema['fields'])} fields")
    print()

    errors, warnings = validate(schema, header_fields)

    if warnings:
        print("Warnings:")
        for w in warnings:
            print(f"  - {w}")
        print()

    if errors:
        print("ERRORS:")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("Validation PASSED - all offsets match!")


if __name__ == '__main__':
    main()
