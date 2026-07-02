#!/usr/bin/env python3
"""Pick the platform-specific GStreamer version from build-config outputs.

Qt version and platform metadata come from the build-config composite action;
this script's only job is the GStreamer version selection (which build-config
exposes as four separate outputs rather than picking one).
"""

from __future__ import annotations

import argparse
import sys

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, write_github_output


def resolve_version(requested: str, platform_default: str, fallback: str) -> str:
    """Return the explicit override if set, else the platform default, else the generic version."""
    if requested:
        return requested
    return platform_default or fallback


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--platform", required=True, choices=["linux", "macos", "windows", "android", "ios"]
    )
    parser.add_argument("--version", default="", help="Explicit GStreamer version override")
    parser.add_argument(
        "--platform-version",
        default="",
        help="Platform default version (from build-config action output)",
    )
    parser.add_argument(
        "--fallback-version",
        default="",
        help="Generic fallback (build-config gstreamer_version output)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    version = resolve_version(args.version, args.platform_version, args.fallback_version)
    if not version:
        gh_error(f"No GStreamer version resolved for platform={args.platform}")
        return 1
    write_github_output({"version": version})
    print(f"Building GStreamer {version} for {args.platform}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
