#!/usr/bin/env python3
"""Generate QML vehicle config pages from VehicleConfig.json definitions.

Usage:
    python -m tools.generators.config_qml.generate_pages \\
        --pages-dir src/AutoPilotPlugins/PX4/VehicleConfig \\
        --output-dir build/src/AutoPilotPlugins/PX4/generated

Each ``<Name>.VehicleConfig.json`` produces ``<Name>Component.qml``.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow running as a module (-m) from the repo root
_SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(_SCRIPT_DIR.parent.parent))

from generators.config_qml.page_generator import generate_config_page_qml, load_page_def


def _output_name(json_name: str) -> str:
    """Derive the QML output filename from the JSON filename.

    ``Safety.VehicleConfig.json`` -> ``SafetyComponent.qml``
    """
    stem = json_name.removesuffix(".VehicleConfig.json")
    return f"{stem}Component.qml"


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate config page QML from JSON definitions")
    parser.add_argument("--pages-dir", type=Path, required=True,
                        help="Directory containing *.VehicleConfig.json files")
    parser.add_argument("--output-dir", "-o", type=Path, required=True,
                        help="Output directory for generated QML files")
    parser.add_argument("--dry-run", "-n", action="store_true",
                        help="Print to stdout without writing files")
    args = parser.parse_args()

    if not args.pages_dir.is_dir():
        print(f"Error: {args.pages_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    json_files = sorted(args.pages_dir.glob("*.VehicleConfig.json"))
    if not json_files:
        print(f"Warning: no *.VehicleConfig.json files found in {args.pages_dir}", file=sys.stderr)
        return

    args.output_dir.mkdir(parents=True, exist_ok=True)

    for json_path in json_files:
        page_def = load_page_def(json_path)
        qml = generate_config_page_qml(page_def)
        out_name = _output_name(json_path.name)

        if args.dry_run:
            print(f"=== {out_name} ===")
            print(qml)
        else:
            out_path = args.output_dir / out_name
            out_path.write_text(qml, encoding="utf-8")
            print(f"  Generated {out_path}")


if __name__ == "__main__":
    main()
