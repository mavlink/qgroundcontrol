"""Parse MAVLink XML definitions and emit a C++ header mapping message IDs to instance field names.

Usage: mavlink_instance_fields.py <xml_dir> <dialect> <output_header>

The script resolves the dialect's <include> chain, finds all <field instance="true">
elements, and generates a QMap<uint32_t, QString> lookup table.
"""

import sys
from pathlib import Path
from xml.etree.ElementTree import Element

_tools_dir = Path(__file__).resolve().parents[1]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from _bootstrap import ensure_tools_dir  # noqa: E402

ensure_tools_dir(__file__)

from common.io import write_text_if_changed  # noqa: E402
from common.xml import xml_parse  # noqa: E402


def _required_attribute(element: Element, name: str, source: Path) -> str:
    """Return a required XML attribute with source context on failure."""
    value = element.get(name)
    if value is None:
        raise ValueError(f"{source}: <{element.tag}> is missing required {name!r} attribute")
    return value


def resolve_includes(xml_dir: Path, dialect: str, visited: set[str] | None = None) -> list[Path]:
    """Recursively resolve the include chain for a dialect, returning XML paths in dependency order."""
    if visited is None:
        visited = set()
    xml_path = xml_dir / f"{dialect}.xml"
    if dialect in visited or not xml_path.exists():
        return []
    visited.add(dialect)

    result: list[Path] = []
    tree = xml_parse(xml_path)
    root = tree.getroot()
    if root is None:
        raise ValueError(f"{xml_path}: XML document has no root element")
    for include_elem in root.findall("include"):
        if not include_elem.text:
            raise ValueError(f"{xml_path}: <include> must name a dialect")
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
        tree = xml_parse(xml_path)
        root = tree.getroot()
        if root is None:
            raise ValueError(f"{xml_path}: XML document has no root element")
        for message in root.iter("message"):
            msg_id = int(_required_attribute(message, "id", xml_path))
            msg_name = _required_attribute(message, "name", xml_path)
            for field in message.findall("field"):
                if field.get("instance") == "true":
                    field_name = _required_attribute(field, "name", xml_path)
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

    if write_text_if_changed(output_path, header_content):
        print(f"Generated {output_path} ({len(instance_fields)} instance fields)")
    else:
        print(f"Unchanged: {output_path}")


if __name__ == "__main__":
    main()
