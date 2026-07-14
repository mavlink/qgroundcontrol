"""Install tracked VS Code defaults without overwriting local configuration."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

TEMPLATES = (
    ("settings.default.json", "settings.json"),
    ("tasks.default.json", "tasks.json"),
    ("launch.default.json", "launch.json"),
)


def install_vscode_templates(vscode_dir: Path, excluded: set[str] | None = None) -> list[Path]:
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
        created.append(destination)
        print(f"Created {destination}")
    return created


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--vscode-dir",
        type=Path,
        default=Path(__file__).resolve().parents[2] / ".vscode",
        help="Directory containing the tracked VS Code templates",
    )
    parser.add_argument(
        "--exclude",
        action="append",
        choices=[Path(destination).stem for _template, destination in TEMPLATES],
        default=[],
        help="Configuration name to leave unmanaged (may be repeated)",
    )
    args = parser.parse_args(argv)

    try:
        install_vscode_templates(args.vscode_dir, set(args.exclude))
    except (FileNotFoundError, OSError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
