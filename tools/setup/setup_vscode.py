"""Install tracked VS Code defaults without overwriting local configuration."""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.io import read_json, write_json
from common.paths import REPO_ROOT

TEMPLATES = (
    ("settings.default.json", "settings.json"),
    ("tasks.default.json", "tasks.json"),
    ("launch.default.json", "launch.json"),
)


def install_vscode_templates(
    vscode_dir: Path, excluded: set[str] | None = None, python_interpreter: str | None = None
) -> list[Path]:
    """Copy missing VS Code configuration files from the tracked templates."""
    created: list[Path] = []
    excluded = excluded or set()
    for template_name, destination_name in TEMPLATES:
        if Path(destination_name).stem in excluded:
            continue
        template = vscode_dir / template_name
        destination = vscode_dir / destination_name
        if destination.exists():
            print(f"Keeping existing {destination}")
            continue
        if not template.is_file():
            raise FileNotFoundError(f"VS Code template not found: {template}")
        shutil.copyfile(template, destination)
        if destination_name == "settings.json" and python_interpreter:
            settings = read_json(destination)
            settings["python.defaultInterpreterPath"] = python_interpreter
            write_json(destination, settings)
        created.append(destination)
        print(f"Created {destination}")
    return created


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--vscode-dir",
        type=Path,
        default=REPO_ROOT / ".vscode",
        help="Directory containing the tracked VS Code templates",
    )
    parser.add_argument(
        "--exclude",
        action="append",
        choices=[Path(destination).stem for _template, destination in TEMPLATES],
        default=[],
        help="Configuration name to leave unmanaged (may be repeated)",
    )
    parser.add_argument("--python-interpreter", help="Interpreter for newly created settings only")
    args = parser.parse_args(argv)

    try:
        install_vscode_templates(args.vscode_dir, set(args.exclude), args.python_interpreter)
    except (FileNotFoundError, OSError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
