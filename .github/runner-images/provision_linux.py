#!/usr/bin/env python3
"""Provision the Qt SDK after the shell bootstrap checks out the requested source."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def provision() -> None:
    source = Path(__file__).resolve().parents[2]
    prefix = Path("/opt/qgc-sdk")
    subprocess.run([sys.executable, str(source / "tools/setup/install_uv.py")], check=True)
    os.environ["PATH"] = f"{Path.home() / '.local/bin'}:{os.environ['PATH']}"
    subprocess.run(
        [sys.executable, str(source / "tools/setup/install_python.py"), "build,qt"], check=True
    )
    python = str(source / "tools/.venv/bin/python")
    os.environ["PATH"] = f"{source / 'tools/.venv/bin'}:{os.environ['PATH']}"
    subprocess.run(
        [python, str(source / "tools/setup/install_dependencies"), "--platform", "debian"],
        check=True,
    )
    subprocess.run(["sudo", "mkdir", "-p", str(prefix / "Qt")], check=True)
    subprocess.run(["sudo", "chown", "-R", f"{os.getuid()}:{os.getgid()}", str(prefix)], check=True)
    subprocess.run(
        [
            python,
            str(source / "tools/setup/install_qt.py"),
            "install",
            "--version",
            os.environ["QGC_QT_VERSION"],
            "--host",
            "linux",
            "--target",
            "desktop",
            "--arch",
            "linux_gcc_64",
            "--modules",
            os.environ["QGC_QT_MODULES"],
            "--outdir",
            str(prefix / "Qt"),
        ],
        check=True,
    )
    root = prefix / "Qt" / os.environ["QGC_QT_VERSION"] / "gcc_64"
    (root / ".qgc-modules").write_text(f"{os.environ['QGC_QT_MODULES']}\n")
    subprocess.run(
        [
            python,
            str(source / ".github/scripts/cpm_helper.py"),
            "create-seed",
            "--root",
            str(source),
            "--qt-root",
            str(root),
            "--seed",
            str(prefix / "cpm"),
        ],
        check=True,
    )
    subprocess.run(["sudo", "chown", "-R", "root:root", str(prefix)], check=True)
    subprocess.run(["sudo", "chmod", "-R", "a+rX", str(prefix)], check=True)
    subprocess.run(
        ["sudo", "tee", "-a", "/etc/environment"],
        input=f"QGC_PREINSTALLED_QT_DIR={prefix}\nQGC_PREINSTALLED_CPM_DIR={prefix / 'cpm'}\n",
        text=True,
        stdout=subprocess.DEVNULL,
        check=True,
    )
    subprocess.run(
        ["sudo", "tee", "/etc/profile.d/qgc-sdk.sh"],
        input=f"export QGC_PREINSTALLED_QT_DIR={prefix}\nexport QGC_PREINSTALLED_CPM_DIR={prefix / 'cpm'}\n",
        text=True,
        stdout=subprocess.DEVNULL,
        check=True,
    )
    subprocess.run(["sudo", "apt-get", "clean"], check=True)
    for entry in Path("/var/lib/apt/lists").iterdir():
        subprocess.run(["sudo", "rm", "-rf", str(entry)], check=True)


if __name__ == "__main__":
    try:
        provision()
    except (OSError, KeyError, subprocess.CalledProcessError) as error:
        print(f"Runner provisioning failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
