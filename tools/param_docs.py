#!/usr/bin/env python3
"""
Generate Parameter Documentation from QGC JSON Metadata

Extracts parameter definitions from FactMetaData JSON files and generates
documentation in various formats (Markdown, HTML, or JSON).

Usage:
    ./param-docs.py                           # Generate markdown for all params
    ./param-docs.py --format html             # Generate HTML
    ./param-docs.py --format json             # Generate JSON
    ./param-docs.py --group "Battery"         # Filter by group
    ./param-docs.py --output params.md        # Custom output file
    ./param-docs.py src/Vehicle/              # Scan specific directory

The script finds all *.FactMetaData.json files and extracts parameter info.
"""

import argparse
import html
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Parameter:
    """Represents a parameter definition."""

    name: str
    short_desc: str = ""
    long_desc: str = ""
    type: str = "unknown"
    units: str = ""
    default: str = ""
    min_value: str = ""
    max_value: str = ""
    increment: str = ""
    decimal_places: int = 0
    enum_strings: list[str] = field(default_factory=list)
    enum_values: list[str] = field(default_factory=list)
    group: str = ""
    source_file: str = ""


def find_factmetadata_files(search_path: Path) -> list[Path]:
    """Find all FactMetaData JSON files."""
    files = []
    for pattern in ["*.FactMetaData.json", "*FactMetaData.json"]:
        files.extend(search_path.rglob(pattern))
    return sorted(set(files))


def parse_factmetadata(filepath: Path) -> list[Parameter]:
    """Parse a FactMetaData JSON file."""
    params = []

    try:
        with open(filepath, encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        print(f"Warning: Could not parse {filepath}: {e}", file=sys.stderr)
        return []

    # Handle both formats: direct list or wrapped in object
    if isinstance(data, dict):
        # Look for parameters in common keys
        for key in ["parameters", "facts", "items", ""]:
            if key in data and isinstance(data[key], list):
                data = data[key]
                break
        else:
            # Maybe it's a single parameter object
            if "name" in data:
                data = [data]
            else:
                return []

    if not isinstance(data, list):
        return []

    for item in data:
        if not isinstance(item, dict):
            continue

        param = Parameter(
            name=item.get("name", "unknown"),
            short_desc=item.get("shortDesc", item.get("shortDescription", "")),
            long_desc=item.get("longDesc", item.get("longDescription", "")),
            type=item.get("type", "unknown"),
            units=item.get("units", ""),
            default=str(item.get("default", item.get("defaultValue", ""))),
            min_value=str(item.get("min", "")),
            max_value=str(item.get("max", "")),
            increment=str(item.get("increment", "")),
            decimal_places=item.get("decimalPlaces", 0),
            group=item.get("group", item.get("category", "")),
            source_file=str(filepath.name),
        )

        # Handle enums
        if "enumStrings" in item:
            enum_str = item["enumStrings"]
            if isinstance(enum_str, str):
                param.enum_strings = [s.strip() for s in enum_str.split(",")]
            elif isinstance(enum_str, list):
                param.enum_strings = enum_str

        if "enumValues" in item:
            enum_val = item["enumValues"]
            if isinstance(enum_val, str):
                param.enum_values = [v.strip() for v in enum_val.split(",")]
            elif isinstance(enum_val, list):
                param.enum_values = [str(v) for v in enum_val]

        params.append(param)

    return params


def generate_markdown(params: list[Parameter], group_by: str = "group") -> str:
    """Generate Markdown documentation."""
    lines = []
    lines.append("# QGroundControl Parameters\n")
    lines.append("This document describes the parameters used in QGroundControl.\n")
    lines.append(f"Generated from {len({p.source_file for p in params})} FactMetaData files.\n")

    # Group parameters
    groups: dict[str, list[Parameter]] = {}
    for param in params:
        key = getattr(param, group_by) or "Ungrouped"
        if key not in groups:
            groups[key] = []
        groups[key].append(param)

    # Generate documentation
    for group_name in sorted(groups.keys()):
        group_params = groups[group_name]
        lines.append(f"\n## {group_name}\n")

        for param in sorted(group_params, key=lambda p: p.name):
            lines.append(f"### {param.name}\n")

            if param.short_desc:
                lines.append(f"**{param.short_desc}**\n")

            if param.long_desc:
                lines.append(f"\n{param.long_desc}\n")

            # Properties table
            lines.append("\n| Property | Value |")
            lines.append("|----------|-------|")

            if param.type:
                lines.append(f"| Type | {param.type} |")
            if param.units:
                lines.append(f"| Units | {param.units} |")
            if param.default:
                lines.append(f"| Default | {param.default} |")
            if param.min_value:
                lines.append(f"| Min | {param.min_value} |")
            if param.max_value:
                lines.append(f"| Max | {param.max_value} |")
            if param.increment:
                lines.append(f"| Increment | {param.increment} |")

            # Enum values
            if param.enum_strings:
                lines.append("\n**Options:**\n")
                for i, enum_str in enumerate(param.enum_strings):
                    enum_val = param.enum_values[i] if i < len(param.enum_values) else str(i)
                    lines.append(f"- `{enum_val}`: {enum_str}")

            lines.append("")

    return "\n".join(lines)


def generate_html(params: list[Parameter]) -> str:
    """Generate HTML documentation."""
    html_parts = []

    html_parts.append("""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>QGroundControl Parameters</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 900px; margin: 0 auto; padding: 20px; }
        h1 { color: #333; border-bottom: 2px solid #4a90d9; padding-bottom: 10px; }
        h2 { color: #4a90d9; margin-top: 30px; }
        h3 { color: #666; margin-top: 20px; }
        .param { background: #f5f5f5; padding: 15px; border-radius: 5px; margin: 10px 0; }
        .param-name { font-weight: bold; color: #333; font-size: 1.1em; }
        .param-desc { color: #666; margin: 5px 0; }
        table { border-collapse: collapse; margin: 10px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background: #4a90d9; color: white; }
        .enum { background: #e8f4fd; padding: 10px; border-radius: 3px; margin: 10px 0; }
        code { background: #eee; padding: 2px 5px; border-radius: 3px; }
    </style>
</head>
<body>
    <h1>QGroundControl Parameters</h1>
""")

    # Group parameters
    groups: dict[str, list[Parameter]] = {}
    for param in params:
        key = param.group or "Ungrouped"
        if key not in groups:
            groups[key] = []
        groups[key].append(param)

    for group_name in sorted(groups.keys()):
        html_parts.append(f"<h2>{html.escape(group_name)}</h2>")

        for param in sorted(groups[group_name], key=lambda p: p.name):
            html_parts.append('<div class="param">')
            html_parts.append(f'<div class="param-name">{html.escape(param.name)}</div>')

            if param.short_desc:
                html_parts.append(
                    f'<div class="param-desc"><strong>{html.escape(param.short_desc)}</strong></div>'
                )

            if param.long_desc:
                html_parts.append(f'<div class="param-desc">{html.escape(param.long_desc)}</div>')

            html_parts.append("<table>")
            html_parts.append("<tr><th>Property</th><th>Value</th></tr>")

            props = [
                ("Type", param.type),
                ("Units", param.units),
                ("Default", param.default),
                ("Min", param.min_value),
                ("Max", param.max_value),
            ]
            for prop_name, prop_val in props:
                if prop_val:
                    html_parts.append(
                        f"<tr><td>{prop_name}</td><td>{html.escape(str(prop_val))}</td></tr>"
                    )

            html_parts.append("</table>")

            if param.enum_strings:
                html_parts.append('<div class="enum"><strong>Options:</strong><ul>')
                for i, enum_str in enumerate(param.enum_strings):
                    enum_val = param.enum_values[i] if i < len(param.enum_values) else str(i)
                    html_parts.append(
                        f"<li><code>{html.escape(enum_val)}</code>: {html.escape(enum_str)}</li>"
                    )
                html_parts.append("</ul></div>")

            html_parts.append("</div>")

    html_parts.append("</body></html>")
    return "\n".join(html_parts)


def generate_json_output(params: list[Parameter]) -> str:
    """Generate JSON output."""
    output = []
    for param in params:
        output.append(
            {
                "name": param.name,
                "shortDesc": param.short_desc,
                "longDesc": param.long_desc,
                "type": param.type,
                "units": param.units,
                "default": param.default,
                "min": param.min_value,
                "max": param.max_value,
                "increment": param.increment,
                "enumStrings": param.enum_strings,
                "enumValues": param.enum_values,
                "group": param.group,
                "sourceFile": param.source_file,
            }
        )
    return json.dumps(output, indent=2)


def main():
    parser = argparse.ArgumentParser(
        description="Generate parameter documentation from FactMetaData JSON files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("path", nargs="?", default="src", help="Directory to search (default: src)")
    parser.add_argument(
        "-f",
        "--format",
        choices=["markdown", "html", "json"],
        default="markdown",
        help="Output format",
    )
    parser.add_argument("-o", "--output", help="Output file (default: stdout)")
    parser.add_argument("-g", "--group", help="Filter by group name")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show verbose output")

    args = parser.parse_args()

    # Find repo root
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent

    search_path = Path(args.path)
    if not search_path.is_absolute():
        search_path = repo_root / search_path

    if not search_path.exists():
        print(f"Error: Path not found: {search_path}", file=sys.stderr)
        sys.exit(1)

    # Find and parse files
    files = find_factmetadata_files(search_path)

    if args.verbose:
        print(f"Found {len(files)} FactMetaData files", file=sys.stderr)

    all_params = []
    for filepath in files:
        if args.verbose:
            print(f"  Parsing: {filepath.name}", file=sys.stderr)
        params = parse_factmetadata(filepath)
        all_params.extend(params)

    if args.verbose:
        print(f"Found {len(all_params)} parameters", file=sys.stderr)

    # Filter by group
    if args.group:
        all_params = [p for p in all_params if args.group.lower() in p.group.lower()]

    if not all_params:
        print("No parameters found", file=sys.stderr)
        sys.exit(1)

    # Generate output
    if args.format == "markdown":
        output = generate_markdown(all_params)
    elif args.format == "html":
        output = generate_html(all_params)
    else:
        output = generate_json_output(all_params)

    # Write output
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(output)
        print(f"Written to {args.output}", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()
