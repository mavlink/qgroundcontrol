#!/usr/bin/env python3
"""Plan Docker build matrix entries for the Docker workflow."""

from __future__ import annotations

import argparse
import json
import os
import sys
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import parse_bool, write_github_output

# Build args that turn the `linux` target (24.04 default) into the 22.04 image:
# older glibc base, gcc-12 for full C++20, and a modern cmake via pip (jammy's
# apt cmake 3.22 < required 3.25). Sync with run-docker.sh is enforced by
# test_2204_build_args_match_run_docker_sh.
LINUX_2204_BUILD_ARGS = "\n".join(
    [
        "UBUNTU_REF=ubuntu:22.04@sha256:4f838adc7181d9039ac795a7d0aba05a9bd9ecd480d294483169c5def983b64d",
        "APT_EXTRA=gcc-12 g++-12",
        "PIP_CMAKE=cmake>=3.25,<4",
        "CC_PIN=gcc-12",
        "CXX_PIN=g++-12",
    ]
)


def plan_builds(event_name: str, linux_changed: bool, android_changed: bool) -> dict[str, Any]:
    """Return workflow matrix and a has_jobs flag.

    Returns {"matrix": {"include": [...]}, "has_jobs": bool}. Typed as
    dict[str, Any] so callers can subscript matrix["include"] without
    pyright complaining about object indexing.
    """
    include: list[dict[str, Any]] = []

    linux_selected = event_name != "pull_request" or linux_changed
    android_selected = event_name != "pull_request" or android_changed

    if linux_selected:
        include.append(
            {
                "platform": "Linux",
                "target": "linux",
                "variant": "linux",
                "build_args": "",
                "fuse": True,
                "artifact_pattern": "*.AppImage",
            }
        )
        include.append(
            {
                "platform": "Linux-22.04",
                # 22.04 reuses the `linux` target, overriding base + toolchain
                # via build args; `variant` keys the cache/tags so it stays distinct.
                "target": "linux",
                "variant": "linux-2204",
                "build_args": LINUX_2204_BUILD_ARGS,
                "fuse": True,
                "artifact_pattern": "*.AppImage",
            }
        )
        include.append(
            {
                "platform": "Linux-aarch64",
                "target": "linux-cross",
                "variant": "linux-aarch64",
                "build_args": "",
                "fuse": False,
                "artifact_pattern": "QGroundControl",
            }
        )
    if android_selected:
        include.append(
            {
                "platform": "Android",
                "target": "android",
                "variant": "android",
                "build_args": "",
                "fuse": False,
                "artifact_pattern": "*.apk",
            }
        )

    return {"matrix": {"include": include}, "has_jobs": bool(include)}


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Plan Docker workflow builds.")
    parser.add_argument("--event-name", default=os.environ.get("EVENT_NAME", ""))
    parser.add_argument("--linux", default=os.environ.get("LINUX", "false"))
    parser.add_argument("--android", default=os.environ.get("ANDROID", "false"))
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Compute the Docker build matrix and emit outputs."""
    args = parse_args(argv)
    plan = plan_builds(
        args.event_name,
        parse_bool(args.linux),
        parse_bool(args.android),
    )
    matrix_json = json.dumps(plan["matrix"], separators=(",", ":"))
    print(matrix_json)

    write_github_output(
        {
            "matrix": matrix_json,
            "has_jobs": "true" if plan["has_jobs"] else "false",
        }
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
