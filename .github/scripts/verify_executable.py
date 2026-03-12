#!/usr/bin/env python3
"""Verify QGroundControl executable with a boot test.

Resolves paths, sets permissions, and delegates to run_tests.py --verify-only.
"""

from __future__ import annotations

import argparse
import os
import stat
import subprocess
import sys
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary-path", required=True)
    parser.add_argument("--working-dir", default="")
    parser.add_argument("--type", default="binary", dest="exe_type")
    parser.add_argument("--timeout", default="60")
    args = parser.parse_args()

    binary_path = Path(args.binary_path)
    work_dir = Path(args.working_dir) if args.working_dir else binary_path.parent

    if not work_dir.is_dir():
        print(f"::error::Working directory not found: {work_dir}", file=sys.stderr)
        sys.exit(1)

    binary_name = binary_path.name
    run_binary = binary_name
    headless = True

    # AppImages typically don't ship the "offscreen" Qt platform plugin.
    if args.exe_type == "appimage" or binary_name.endswith(".AppImage"):
        headless = False

    if os.name != "nt":
        exe = work_dir / binary_name
        try:
            exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
        except OSError:
            pass
        run_binary = str(exe.resolve())

    workspace = os.environ.get("GITHUB_WORKSPACE", ".")
    cmd = [
        sys.executable, os.path.join(workspace, "tools", "run_tests.py"),
        "--binary", run_binary,
        "--timeout", args.timeout,
        "--verify-only",
        "-v",
    ]
    if headless:
        cmd.append("--headless")

    result = subprocess.run(cmd, cwd=str(work_dir), check=False)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
