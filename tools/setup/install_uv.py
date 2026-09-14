#!/usr/bin/env python3
"""Bootstrap uv for Linux/macOS provisioning without project dependencies."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if shutil.which("uv"):
        return 0
    try:
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
                    "https://astral.sh/uv/0.11.12/install.sh",
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
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"uv installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
