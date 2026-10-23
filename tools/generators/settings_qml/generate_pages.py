"""
Generate QML settings pages from UI definition JSON files.

Run from the repo root:
    python3 -m tools.generators.settings_qml.generate_pages --output-dir build/generated-settings-qml

Reads:
  - src/AppSettings/pages/SettingsPages.json  (page list + metadata)
  - src/AppSettings/pages/<page>.json          (per-page UI definitions)
  - src/Settings/*.SettingsGroup.json              (fact type metadata)

Generates:
  - <output-dir>/<page>.qml                     (one per page definition)
  - <output-dir>/SettingsPagesModel.qml         (page list model)

CMake runs this at build time with an output directory under the build tree.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.io import write_text_if_changed

from .emit import generate_page_qml, generate_pages_model_qml
from .metadata import load_settings_metadata
from .model import load_page_def, load_pages_data, resolve_page_def_path

PAGES_DIR = Path("src/AppSettings/pages")
SETTINGS_DIR = Path("src/Settings")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate QML settings pages from UI definitions")
    parser.add_argument("--output-dir", "-o", help="Output directory for generated QML files")
    parser.add_argument(
        "--dry-run", "-n", action="store_true", help="Print what would be generated without writing"
    )
    parser.add_argument(
        "--custom-pages-dir",
        type=Path,
        help="Custom-build pages dir with an optional SettingsPages.json overlay "
        "and shadowing/extra page definition files",
    )
    parser.add_argument(
        "--custom-settings-dir",
        type=Path,
        help="Custom-build dir with additional *.SettingsGroup.json fact metadata",
    )
    parser.add_argument(
        "--list-outputs",
        action="store_true",
        help="Print the QML file names that would be generated, one per line, without writing",
    )
    args = parser.parse_args()

    if not args.list_outputs and not args.output_dir:
        parser.error("--output-dir is required unless --list-outputs is given")

    pages_json = PAGES_DIR / "SettingsPages.json"
    if not pages_json.exists():
        print(f"ERROR: {pages_json} not found", file=sys.stderr)
        return 1

    settings_dirs: tuple[Path, ...] = (SETTINGS_DIR,)
    if args.custom_settings_dir:
        settings_dirs += (args.custom_settings_dir,)
        # Metadata is otherwise loaded lazily per control; validate custom accessor
        # collisions/grammar unconditionally, even when no generated page needs metadata
        load_settings_metadata(settings_dirs)

    page_entries = load_pages_data(pages_json, args.custom_pages_dir)
    output_names: list[str] = []
    generated = 0

    # Generate per-page QML files
    for entry in page_entries:
        if entry.get("divider"):
            continue

        page_def_name = entry.get("pageDefinition")
        if not page_def_name:
            continue

        qml_name = entry.get("qml")
        if not qml_name:
            print(f"SKIP {page_def_name}: no 'qml' field", file=sys.stderr)
            continue

        page_def_path = resolve_page_def_path(page_def_name, PAGES_DIR, args.custom_pages_dir)
        if not page_def_path.exists():
            print(f"ERROR: {qml_name}: {page_def_path} not found", file=sys.stderr)
            return 1

        output_names.append(qml_name)
        if args.list_outputs:
            continue

        page = load_page_def(page_def_path)
        qml = generate_page_qml(
            page, settings_dirs, json_context=page_def_name, page_name=entry.get("name", "")
        )

        if args.dry_run:
            print(f"=== {qml_name} ===")
            print("\n".join(qml.split("\n")[:20]))
            print("...\n")
        else:
            output_path = Path(args.output_dir) / qml_name
            output_path.parent.mkdir(parents=True, exist_ok=True)
            write_text_if_changed(output_path, qml)
            print(f"Generated: {output_path}")

        generated += 1

    output_names.append("SettingsPagesModel.qml")
    if args.list_outputs:
        print("\n".join(output_names))
        return 0

    # Generate SettingsPagesModel.qml
    model_qml = generate_pages_model_qml(pages_json, args.custom_pages_dir)
    if args.dry_run:
        print("=== SettingsPagesModel.qml ===")
        print(model_qml)
    else:
        model_path = Path(args.output_dir) / "SettingsPagesModel.qml"
        write_text_if_changed(model_path, model_qml)
        print(f"Generated: {model_path}")

    generated += 1
    print(f"\n{generated} files generated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
