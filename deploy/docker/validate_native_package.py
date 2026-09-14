#!/usr/bin/env python3
"""Validate, install, smoke-test, and uninstall a native package inside its build image."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def package_commands(package: Path) -> tuple[list[str], list[str]]:
    if package.name.endswith(".deb"):
        query = ["dpkg-deb", "--field", str(package), "Package"]
        install = ["apt-get", "install", "-y", str(package)]
        remove = ["apt-get", "remove", "-y"]
        subprocess.run(["apt-get", "update"], check=True)
    elif package.name.endswith(".rpm"):
        query = ["rpm", "-qp", "--queryformat", "%{NAME}", str(package)]
        install = ["dnf", "install", "-y", str(package)]
        remove = ["dnf", "remove", "-y"]
    elif package.name.endswith(".pkg.tar.zst"):
        query = ["pacman", "-Qp", str(package)]
        install = ["pacman", "-U", "--noconfirm", str(package)]
        remove = ["pacman", "-R", "--noconfirm"]
        subprocess.run(["pacman", "-Sy", "--noconfirm"], check=True)
    else:
        raise ValueError(f"Unsupported native package: {package}")
    name = subprocess.run(query, capture_output=True, text=True, check=True).stdout.split()
    if not name or name[0].startswith("-"):
        raise ValueError("Package metadata has no valid package name")
    return install, [*remove, name[0]]


def validate(package: Path) -> None:
    source = Path(os.environ.get("QGC_SOURCE_ROOT", "/project/source"))
    subprocess.run(
        [sys.executable, str(source / ".github/scripts/validate_native_package.py"), str(package)],
        check=True,
    )
    install, remove = package_commands(package)
    subprocess.run(install, env={**os.environ, "DEBIAN_FRONTEND": "noninteractive"}, check=True)
    try:
        if not os.access("/opt/QGroundControl/bin/QGroundControl", os.X_OK):
            raise ValueError("Installed application is missing or not executable")
        if not Path("/usr/bin/QGroundControl").is_symlink():
            raise ValueError("Installed launcher is not a symlink")
        subprocess.run(
            ["/usr/bin/QGroundControl", "--help"],
            env={**os.environ, "QT_QPA_PLATFORM": "offscreen"},
            timeout=30,
            stdout=subprocess.DEVNULL,
            check=True,
        )
    finally:
        subprocess.run(remove, check=True)
    if os.path.lexists("/opt/QGroundControl") or os.path.lexists("/usr/bin/QGroundControl"):
        raise ValueError("Uninstall left application files behind")
    print(f"Native package lifecycle validated: {package.name}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    args = parser.parse_args(argv)
    try:
        validate(args.package.resolve())
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"Native package validation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
