#!/usr/bin/env python3
"""Synchronize locked QGC tooling groups with uv; retain other groups unless --replace is given."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

_tools_dir = Path(__file__).resolve().parents[1]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from qgc_tools.python_env import (
    check_requirements,
    package_groups,
    requirements_for,
    sync_groups,
)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("group", nargs="?", default="dev", help="Comma-separated dependency groups")
    parser.add_argument(
        "--environment", type=Path, help="Explicit environment path (default: tools/.venv)"
    )
    parser.add_argument("--python", help="Python interpreter for the environment")
    parser.add_argument(
        "--replace", action="store_true", help="Remove packages outside the requested profile"
    )
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument(
        "--check", action="store_true", help="Check requirements in the running interpreter"
    )
    parser.add_argument("--list-groups", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.list_groups:
            for name, packages in package_groups().items():
                print(f"{name}: {', '.join(packages)}")
            return 0
        if args.check:
            return check_requirements(requirements_for(args.group))
        target = sync_groups(
            args.group,
            environment=args.environment,
            replace=args.replace,
            dry_run=args.dry_run,
            python=args.python,
        )
        if not args.dry_run:
            activate = target / (
                "Scripts/activate.bat" if sys.platform == "win32" else "bin/activate"
            )
            print(f"Python environment: {target}\nActivate with: {activate}")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Python setup failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
