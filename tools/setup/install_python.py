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
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGE_GROUPS: dict[str, list[str]] = {
    "ci": [
        "pre-commit",
        "meson",
        "ninja",
    ],
    "qt": [
        "aqtinstall",
    ],
    "coverage": [
        "gcovr",
    ],
    "dev": [
        "jinja2",
        "pyyaml",
        "pymavlink",
    ],
    "lsp": [
        "pygls",
        "lsprotocol",
    ],
    "all": [],  # Populated below
}

# 'all' includes everything from other groups
PACKAGE_GROUPS["all"] = list(
    set(
        pkg
        for group, packages in PACKAGE_GROUPS.items()
        if group != "all"
        for pkg in packages
    )
)


def get_repo_root() -> Path:
    """Find repository root directory."""
    current = Path(__file__).resolve()
    for parent in [current] + list(current.parents):
        if (parent / ".git").exists():
            return parent
    return Path.cwd()


def has_uv() -> bool:
    """Check if uv is available."""
    return shutil.which("uv") is not None


def get_venv_path() -> Path:
    """Get path to virtual environment."""
    return get_repo_root() / ".venv"


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
        subprocess.run(["uv", "venv", str(venv_path)], check=True)
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


def list_packages(venv_path: Path) -> None:
    """List installed packages."""
    python = get_python_executable(venv_path)

    if has_uv():
        subprocess.run(["uv", "pip", "list", "--python", str(python)])
    else:
        subprocess.run([str(python), "-m", "pip", "list"])


def get_packages_for_groups(group_spec: str) -> list[str]:
    """Get packages for comma-separated group specification."""
    groups = [g.strip() for g in group_spec.split(",")]
    packages: set[str] = set()

    for group in groups:
        if group not in PACKAGE_GROUPS:
            raise ValueError(f"Unknown group: {group}. Valid groups: {', '.join(PACKAGE_GROUPS.keys())}")
        packages.update(PACKAGE_GROUPS[group])

    return sorted(packages)


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Setup Python environment for QGroundControl development.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Groups:
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
        print("Available package groups:")
        for group, packages in sorted(PACKAGE_GROUPS.items()):
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
        install_packages(venv_path, packages)
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with exit code {e.returncode}", file=sys.stderr)
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
