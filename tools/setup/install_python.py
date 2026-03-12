#!/usr/bin/env python3
"""
Setup Python development environment for QGroundControl.

Usage:
    python tools/setup/install_python.py           # Install CI tools
    python tools/setup/install_python.py dev       # Install development tools
    python tools/setup/install_python.py all       # Install everything
    python tools/setup/install_python.py --help    # Show help

This script uses uv (fast) if available, otherwise falls back to pip.
"""

from __future__ import annotations

import argparse
import functools
import os
import shutil
import subprocess
import sys
import tomllib
from pathlib import Path

_tools_dir = Path(__file__).resolve().parents[1]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from common.file_traversal import find_repo_root


@functools.lru_cache(maxsize=1)
def load_package_groups() -> dict[str, list[str]]:
    """Load dependency groups from tools/pyproject.toml."""
    pyproject_path = find_repo_root() / "tools" / "pyproject.toml"
    data = tomllib.loads(pyproject_path.read_text(encoding="utf-8"))
    optional = data.get("project", {}).get("optional-dependencies", {})
    if not isinstance(optional, dict):
        raise ValueError("project.optional-dependencies is missing from tools/pyproject.toml")

    groups: dict[str, list[str]] = {}
    for group, packages in optional.items():
        if isinstance(packages, list):
            groups[group] = [str(pkg) for pkg in packages]

    groups["all"] = sorted(
        {
            package
            for group, packages in groups.items()
            if group != "all"
            for package in packages
        }
    )
    return groups


def has_uv() -> bool:
    """Check if uv is available."""
    return shutil.which("uv") is not None


def get_lockfile_path() -> Path:
    """Return the uv lockfile path for the tools project."""
    return find_repo_root() / "tools" / "uv.lock"


def get_project_path() -> Path:
    """Return the tools project path."""
    return find_repo_root() / "tools"


def get_venv_path() -> Path:
    """Get path to virtual environment."""
    return find_repo_root() / ".venv"


def get_activate_script(venv_path: Path) -> Path:
    """Get path to activate script based on platform."""
    if sys.platform == "win32":
        return venv_path / "Scripts" / "activate.bat"
    return venv_path / "bin" / "activate"


def get_python_executable(venv_path: Path) -> Path:
    """Get path to Python executable in venv."""
    if sys.platform == "win32":
        return venv_path / "Scripts" / "python.exe"
    return venv_path / "bin" / "python"


def create_venv(venv_path: Path) -> None:
    """Create virtual environment."""
    if venv_path.exists():
        print(f"Using existing venv: {venv_path}")
        return

    print(f"Creating virtual environment: {venv_path}")

    if has_uv():
        subprocess.run(["uv", "venv", "--seed", str(venv_path)], check=True)
    else:
        subprocess.run([sys.executable, "-m", "venv", str(venv_path)], check=True)


def install_packages(venv_path: Path, packages: list[str]) -> None:
    """Install packages into virtual environment."""
    if not packages:
        print("No packages to install")
        return

    python = get_python_executable(venv_path)

    if has_uv():
        print("Using uv (fast mode)")
        cmd = ["uv", "pip", "install", "--python", str(python)] + packages
    else:
        print("Using pip (install uv for faster installs: curl -LsSf https://astral.sh/uv/install.sh | sh)")
        # Upgrade pip first
        subprocess.run(
            [str(python), "-m", "pip", "install", "--quiet", "--upgrade", "pip"],
            check=True,
        )
        cmd = [str(python), "-m", "pip", "install"] + packages

    subprocess.run(cmd, check=True)


def sync_groups_with_uv(venv_path: Path, group_spec: str) -> None:
    """Sync dependency groups from the locked tools project into the repo venv."""
    lockfile_path = get_lockfile_path()
    if not lockfile_path.exists():
        raise FileNotFoundError(f"uv lockfile not found at {lockfile_path}")

    groups = [group.strip() for group in group_spec.split(",") if group.strip()]
    cmd = [
        "uv",
        "sync",
        "--project",
        str(get_project_path()),
        "--active",
        "--no-install-project",
        "--frozen",
    ]

    if "all" in groups:
        cmd.append("--all-extras")
    else:
        for group in groups:
            cmd.extend(["--extra", group])

    env = os.environ.copy()
    env["VIRTUAL_ENV"] = str(venv_path)
    scripts_dir = venv_path / ("Scripts" if sys.platform == "win32" else "bin")
    env["PATH"] = f"{scripts_dir}{os.pathsep}{env.get('PATH', '')}"
    subprocess.run(cmd, check=True, env=env)


def list_packages(venv_path: Path) -> None:
    """List installed packages."""
    python = get_python_executable(venv_path)
    subprocess.run([str(python), "-m", "pip", "list"])


def get_packages_for_groups(group_spec: str) -> list[str]:
    """Get packages for comma-separated group specification."""
    package_groups = load_package_groups()
    groups = [g.strip() for g in group_spec.split(",")]
    packages: set[str] = set()

    for group in groups:
        if group not in package_groups:
            raise ValueError(f"Unknown group: {group}. Valid groups: {', '.join(package_groups.keys())}")
        packages.update(package_groups[group])

    return sorted(packages)


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Setup Python environment for QGroundControl development.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Groups:
  precommit Pre-commit hooks only
  test      Python test tools (pytest, jinja2, pyyaml)
  ci        Pre-commit hooks, meson, ninja (default)
  qt        Qt installation tools (aqtinstall)
  coverage  Code coverage tools (gcovr)
  dev       Development tools (jinja2, pyyaml, pymavlink)
  lsp       LSP server (pygls, lsprotocol)
  all       Everything

Examples:
  python install_python.py              # CI tools only
  python install_python.py dev          # Development tools
  python install_python.py ci,coverage  # Multiple groups

To install uv (recommended, 10-100x faster than pip):
  curl -LsSf https://astral.sh/uv/install.sh | sh
""",
    )

    parser.add_argument(
        "group",
        nargs="?",
        default="ci",
        help="Package group(s) to install, comma-separated (default: ci)",
    )
    parser.add_argument(
        "--list-groups",
        action="store_true",
        help="List available package groups and exit",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be installed without installing",
    )

    return parser.parse_args(args)


def main() -> int:
    """Main entry point."""
    args = parse_args()

    if args.list_groups:
        package_groups = load_package_groups()
        print("Available package groups:")
        for group, packages in sorted(package_groups.items()):
            print(f"  {group}:")
            for pkg in sorted(packages):
                print(f"    - {pkg}")
        return 0

    try:
        packages = get_packages_for_groups(args.group)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"Setting up Python environment with group: {args.group}")
    print(f"Packages: {', '.join(packages)}")

    if args.dry_run:
        print("\nDry run - no changes made")
        return 0

    venv_path = get_venv_path()

    try:
        create_venv(venv_path)
        if has_uv():
            print("Using uv sync (locked mode)")
            sync_groups_with_uv(venv_path, args.group)
        else:
            install_packages(venv_path, packages)
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with exit code {e.returncode}", file=sys.stderr)
        return 1
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print()
    print("Done! Activate the environment with:")
    activate_script = get_activate_script(venv_path)
    if sys.platform == "win32":
        print(f"  {activate_script}")
    else:
        print(f"  source {activate_script}")

    print()
    print("Installed packages:")
    list_packages(venv_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
