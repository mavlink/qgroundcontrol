"""Generate QML vehicle config pages from VehicleConfig.json definitions.

Usage:
    python -m tools.generators.config_qml.generate_pages \\
        --pages-dir src/AutoPilotPlugins/APM/VehicleConfig \\
        --output-dir build/src/AutoPilotPlugins/APM/generated

Each ``<Name>.VehicleConfig.json`` produces ``<Name>Component.qml``. ``--list-outputs`` prints those
names, one per line, without generating anything.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.io import write_text_if_changed

from generators.config_qml.emit import generate_config_page_qml
from generators.config_qml.model import load_page_def


def _output_name(json_name: str) -> str:
    """Derive the QML output filename from the JSON filename.

    ``APMSafety.VehicleConfig.json`` -> ``APMSafetyComponent.qml``
    """
    stem = json_name.removesuffix(".VehicleConfig.json")
    return f"{stem}Component.qml"


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate config page QML from JSON definitions")
    parser.add_argument(
        "--pages-dir",
        type=Path,
        required=True,
        help="Directory containing *.VehicleConfig.json files",
    )
    parser.add_argument(
        "--output-dir",
        "-o",
        type=Path,
        help="Output directory for generated QML files",
    )
    parser.add_argument(
        "--dry-run", "-n", action="store_true", help="Print to stdout without writing files"
    )
    parser.add_argument(
        "--list-outputs",
        action="store_true",
        help="Print the QML file names that would be generated, one per line, without writing",
    )
    args = parser.parse_args()

    if not args.list_outputs and not args.output_dir:
        parser.error("--output-dir is required unless --list-outputs is given")

    if not args.pages_dir.is_dir():
        print(f"Error: {args.pages_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    json_files = sorted(args.pages_dir.glob("*.VehicleConfig.json"))
    if not json_files:
        print(f"Warning: no *.VehicleConfig.json files found in {args.pages_dir}", file=sys.stderr)
        return

    if args.list_outputs:
        print("\n".join(_output_name(json_path.name) for json_path in json_files))
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
            write_text_if_changed(out_path, qml)
            print(f"  Generated {out_path}")


if __name__ == "__main__":
    main()
