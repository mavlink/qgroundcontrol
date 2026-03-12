#!/usr/bin/env python3
"""Resolve GStreamer and Qt versions for GitHub Actions."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.build_config import get_build_config_value
from common.gh_actions import write_github_output

VERSION_KEYS = {
    "macos": "gstreamer_macos_version",
    "windows": "gstreamer_windows_version",
    "android": "gstreamer_android_version",
    "ios": "gstreamer_ios_version",
}


def resolve_version(platform_name: str, requested_version: str) -> str:
    """Return explicit version override or platform-specific config version."""
    if requested_version:
        return requested_version
    version_key = VERSION_KEYS.get(platform_name, "gstreamer_default_version")
    return get_build_config_value(version_key)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Resolve GStreamer action config values.")
    parser.add_argument("--platform", required=True, choices=["linux", "macos", "windows", "android", "ios"])
    parser.add_argument("--version", default="", help="Explicit GStreamer version override")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Resolve config values and emit GitHub outputs."""
    args = parse_args(argv)
    outputs = {
        "py_cmd": "python3",
        "version": resolve_version(args.platform, args.version),
        "qt_version": get_build_config_value("qt_version"),
    }
    write_github_output(outputs)
    print(f"Building GStreamer {outputs['version']} for {args.platform}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
