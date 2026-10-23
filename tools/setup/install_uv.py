#!/usr/bin/env python3
"""Bootstrap uv for Linux/macOS provisioning without project dependencies."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PYPROJECT = Path(__file__).resolve().parents[1] / "pyproject.toml"


def required_version(pyproject: Path = PYPROJECT) -> str:
    """Return the version named by the `[tool.uv] required-version` floor in tools/pyproject.toml."""
    # Regex rather than TOML: this runs before any package, including tomli on Python 3.10, exists.
    match = re.search(
        r'^required-version\s*=\s*"(?:>=|==)(\d+\.\d+\.\d+)"$',
        pyproject.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    if not match:
        raise ValueError(f"{pyproject} must set [tool.uv] required-version to '>=X.Y.Z'")
    return match[1]


def main() -> int:
    if shutil.which("uv"):
        return 0
    try:
        version = required_version(PYPROJECT)
        with tempfile.TemporaryDirectory(prefix="qgc-uv-") as directory:
            installer = Path(directory) / "install.sh"
            subprocess.run(
                [
                    "curl",
                    "--fail",
                    "--location",
                    "--proto",
                    "=https",
                    "--proto-redir",
                    "=https",
                    "--tlsv1.2",
                    "--retry",
                    "3",
                    "--connect-timeout",
                    "30",
                    "--max-time",
                    "120",
                    f"https://astral.sh/uv/{version}/install.sh",
                    "-o",
                    str(installer),
                ],
                check=True,
            )
            subprocess.run(
                ["sh", str(installer)],
                env={**os.environ, "UV_NO_MODIFY_PATH": "1"},
                check=True,
            )
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"uv installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
