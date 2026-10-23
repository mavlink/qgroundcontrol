#!/usr/bin/env python3
"""Synchronize locked QGC tooling groups with uv; retain other groups unless --replace is given."""

from __future__ import annotations

import argparse
import filecmp
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from common.gh_actions import append_github_path, is_ci
from qgc_tools.python_env import (
    check_requirements,
    executable,
    package_groups,
    requirements_for,
    sync_groups,
)


def add_python3_alias(environment: Path) -> Path:
    """Copy a Windows environment's Scripts/python.exe to python3.exe, like POSIX bin/python3.

    Bash steps run `python3`, which Git Bash resolves to python3.exe; without one in the venv they
    fall through to the runner's interpreter. The venv's python.exe (CPython's venv launcher, or
    the interpreter itself beside its DLLs) finds pyvenv.cfg relative to its own path, so a copy in
    the same directory starts the same environment.
    """
    scripts = environment / "Scripts"
    python, alias = scripts / "python.exe", scripts / "python3.exe"
    # A rebuilt environment can replace python.exe; an identical copy may be the running process.
    if not alias.is_file() or not filecmp.cmp(python, alias, shallow=False):
        shutil.copy2(python, alias)
    return alias


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
            if sys.platform == "win32":
                add_python3_alias(target)
            activate = target / (
                "Scripts/activate.bat" if sys.platform == "win32" else "bin/activate"
            )
            print(f"Python environment: {target}\nActivate with: {activate}")
            if is_ci():
                append_github_path(str(executable("python", target).parent))
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Python setup failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
