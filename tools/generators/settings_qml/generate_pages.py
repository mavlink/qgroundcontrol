#!/usr/bin/env python3
"""
Generate QML settings pages from UI definition JSON files.

Run from the repo root:
    python3 -m tools.generators.settings_qml.generate_pages --output-dir src/UI/AppSettings

Reads:
  - src/UI/AppSettings/pages/SettingsPages.json  (page list + metadata)
  - src/UI/AppSettings/pages/<page>.json          (per-page UI definitions)
  - src/Settings/*.SettingsGroup.json              (fact type metadata)

Generates:
  - src/UI/AppSettings/<page>.qml                  (one per page definition)
  - src/UI/AppSettings/SettingsPagesModel.qml      (page list model)
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .page_generator import generate_page_qml, generate_pages_model_qml, load_page_def

PAGES_DIR = Path("src/UI/AppSettings/pages")
SETTINGS_DIR = Path("src/Settings")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate QML settings pages from UI definitions")
    parser.add_argument("--output-dir", "-o", required=True,
                        help="Output directory for generated QML files")
    parser.add_argument("--dry-run", "-n", action="store_true",
                        help="Print what would be generated without writing")
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    pages_json = PAGES_DIR / "SettingsPages.json"

    if not pages_json.exists():
        print(f"ERROR: {pages_json} not found", file=sys.stderr)
        return 1

    import json
    with open(pages_json, encoding="utf-8") as f:
        pages_data = json.load(f)

    generated = 0

    # Generate per-page QML files
    for entry in pages_data.get("pages", []):
        if entry.get("divider"):
            continue

        page_def_name = entry.get("pageDefinition")

        if not page_def_name:
            continue

        # Determine output file name from "qml" or "outputFile" field
        qml_name = entry.get("qml") or entry.get("outputFile")
        if not qml_name:
            print(f"SKIP {page_def_name}: no 'qml' or 'outputFile' field", file=sys.stderr)
            continue

        page_def_path = PAGES_DIR / page_def_name
        if not page_def_path.exists():
            print(f"SKIP {qml_name}: {page_def_path} not found", file=sys.stderr)
            continue

        page = load_page_def(page_def_path)
        qml = generate_page_qml(page, SETTINGS_DIR)

        if args.dry_run:
            print(f"=== {qml_name} ===")
            print("\n".join(qml.split("\n")[:20]))
            print("...\n")
        else:
            page_output_dir = Path(entry["outputDir"]) if "outputDir" in entry else output_dir
            output_path = page_output_dir / qml_name
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(qml, encoding="utf-8")
            print(f"Generated: {output_path}")

        generated += 1

    # Generate SettingsPagesModel.qml
    model_qml = generate_pages_model_qml(pages_json)
    if args.dry_run:
        print("=== SettingsPagesModel.qml ===")
        print(model_qml)
    else:
        model_path = output_dir / "SettingsPagesModel.qml"
        model_path.write_text(model_qml, encoding="utf-8")
        print(f"Generated: {model_path}")

    generated += 1
    print(f"\n{generated} files generated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
