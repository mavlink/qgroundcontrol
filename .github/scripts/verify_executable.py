#!/usr/bin/env python3
"""Verify QGroundControl executable with a boot test.

Resolves paths, sets permissions, and delegates to run_tests.py --verify-only.
"""

from __future__ import annotations

import argparse
import os
import platform
import re
import stat
import subprocess
import sys
from pathlib import Path


def _setup_gstreamer_env(build_dir: Path) -> None:
    """Extract GStreamer_ROOT_DIR from CMakeCache and set plugin env vars.

    For dev builds (before cmake --install), GStreamer plugins haven't been
    copied into the app bundle yet. This lets the runtime find the
    auto-downloaded SDK's plugins.
    """
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return

    match = re.search(
        r"^GStreamer_ROOT_DIR:\w*=(.+)$", cache.read_text(), re.MULTILINE
    )
    if not match:
        return

    gst_root = Path(match.group(1))
    plugin_dir = gst_root / "lib" / "gstreamer-1.0"
    if not plugin_dir.is_dir():
        return

    print(f"Setting GStreamer env from build cache: {gst_root}")
    plugin_str = str(plugin_dir)
    for var in (
        "GST_PLUGIN_PATH", "GST_PLUGIN_PATH_1_0",
        "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0",
    ):
        os.environ[var] = plugin_str

    scanner = gst_root / "libexec" / "gstreamer-1.0" / "gst-plugin-scanner"
    if platform.system() == "Windows":
        scanner = scanner.with_suffix(".exe")
    if scanner.is_file() and os.access(scanner, os.X_OK):
        scanner_str = str(scanner)
        os.environ["GST_PLUGIN_SCANNER"] = scanner_str
        os.environ["GST_PLUGIN_SCANNER_1_0"] = scanner_str

    lib_dir = str(gst_root / "lib")
    if platform.system() == "Darwin":
        existing = os.environ.get("DYLD_LIBRARY_PATH", "")
        os.environ["DYLD_LIBRARY_PATH"] = (
            f"{lib_dir}:{existing}" if existing else lib_dir
        )
    elif platform.system() == "Windows":
        bin_dir = str(gst_root / "bin")
        existing = os.environ.get("PATH", "")
        os.environ["PATH"] = f"{bin_dir};{existing}" if existing else bin_dir


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary-path", required=True)
    parser.add_argument("--working-dir", default="")
    parser.add_argument("--build-dir", default="")
    parser.add_argument("--type", default="binary", dest="exe_type")
    parser.add_argument("--timeout", default="60")
    args = parser.parse_args()

    binary_path = Path(args.binary_path)
    work_dir = Path(args.working_dir) if args.working_dir else binary_path.parent

    if not work_dir.is_dir():
        print(f"::error::Working directory not found: {work_dir}", file=sys.stderr)
        sys.exit(1)

    if args.build_dir:
        _setup_gstreamer_env(Path(args.build_dir))

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
