#!/usr/bin/env python3
"""Parse MAVLink XML definitions and emit a C++ header mapping message IDs to instance field names.

Usage: mavlink_instance_fields.py <xml_dir> <dialect> <output_header>

The script resolves the dialect's <include> chain, finds all <field instance="true">
elements, and generates a QMap<uint32_t, QString> lookup table.
"""

import sys
from pathlib import Path

import defusedxml.ElementTree as ET


def resolve_includes(xml_dir: Path, dialect: str, visited: set | None = None) -> list[Path]:
    """Recursively resolve the include chain for a dialect, returning XML paths in dependency order."""
    if visited is None:
        visited = set()
    xml_path = xml_dir / f"{dialect}.xml"
    if dialect in visited or not xml_path.exists():
        return []
    visited.add(dialect)

    result = []
    tree = ET.parse(xml_path)
    root = tree.getroot()
    for include_elem in root.findall("include"):
        included_dialect = include_elem.text.replace(".xml", "")
        result.extend(resolve_includes(xml_dir, included_dialect, visited))
    result.append(xml_path)
    return result


def extract_instance_fields(xml_paths: list[Path]) -> dict[int, tuple[str, str]]:
    """Extract instance fields from XML files.

    Returns a dict of msg_id -> (msg_name, field_name), deduplicated by msg_id.
    """
    instance_fields: dict[int, tuple[str, str]] = {}
    for xml_path in xml_paths:
        tree = ET.parse(xml_path)
        root = tree.getroot()
        for message in root.iter("message"):
            msg_id = int(message.get("id"))
            msg_name = message.get("name")
            for field in message.findall("field"):
                if field.get("instance") == "true":
                    field_name = field.get("name")
                    if msg_id not in instance_fields:
                        instance_fields[msg_id] = (msg_name, field_name)
                    break  # Only one instance field per message
    return instance_fields


def generate_header(instance_fields: dict[int, tuple[str, str]]) -> str:
    """Generate the C++ header content."""
    lines = [
        "#pragma once",
        "",
        "/// @file MAVLinkInstanceFields.h",
        "/// @brief Maps MAVLink message IDs to their instance field names.",
        "///",
        "/// AUTO-GENERATED from MAVLink XML definitions during the build.",
        "/// Do not edit manually. Regenerate via tools/generators/mavlink_instance_fields.py.",
        "",
        "#include <QtCore/QMap>",
        "#include <QtCore/QString>",
        "",
        "/// Returns the instance field name for a given message ID, or empty QString if none.",
        "inline const QMap<quint32, QString> &mavlinkInstanceFields()",
        "{",
        "    static const QMap<quint32, QString> fields = {",
    ]

    for msg_id in sorted(instance_fields.keys()):
        msg_name, field_name = instance_fields[msg_id]
        lines.append(f'        {{{msg_id}, QStringLiteral("{field_name}")}},  // {msg_name}')

    lines.extend([
        "    };",
        "    return fields;",
        "}",
        "",
    ])
    return "\n".join(lines)


def write_if_changed(path: Path, content: str) -> bool:
    """Write file only if content changed. Returns True if written."""
    if path.exists() and path.read_text() == content:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    return True


def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <xml_dir> <dialect> <output_header>", file=sys.stderr)
        sys.exit(1)

    xml_dir = Path(sys.argv[1])
    dialect = sys.argv[2]
    output_path = Path(sys.argv[3])

    xml_paths = resolve_includes(xml_dir, dialect)
    if not xml_paths:
        print(f"Error: Could not find {dialect}.xml in {xml_dir}", file=sys.stderr)
        sys.exit(1)

    instance_fields = extract_instance_fields(xml_paths)
    header_content = generate_header(instance_fields)

    if write_if_changed(output_path, header_content):
        print(f"Generated {output_path} ({len(instance_fields)} instance fields)")
    else:
        print(f"Unchanged: {output_path}")


if __name__ == "__main__":
    main()
