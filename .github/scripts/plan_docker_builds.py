#!/usr/bin/env python3
"""Plan Docker build matrix entries for the Docker workflow."""

from __future__ import annotations

import argparse
import json
import os
import sys

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output


def plan_builds(event_name: str, linux_changed: bool, android_changed: bool) -> dict[str, object]:
    """Return workflow matrix and a has_jobs flag."""
    include: list[dict[str, object]] = []

    linux_selected = event_name != "pull_request" or linux_changed
    android_selected = event_name != "pull_request" or android_changed

    if linux_selected:
        include.append(
            {
                "platform": "Linux",
                "dockerfile": "Dockerfile-build-ubuntu",
                "fuse": True,
                "artifact_pattern": "*.AppImage",
            }
        )
    if android_selected:
        include.append(
            {
                "platform": "Android",
                "dockerfile": "Dockerfile-build-android",
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
        args.linux == "true",
        args.android == "true",
    )
    matrix_json = json.dumps(plan["matrix"], separators=(",", ":"))
    print(matrix_json)

    write_github_output({
        "matrix": matrix_json,
        "has_jobs": "true" if plan["has_jobs"] else "false",
    })
    return 0


if __name__ == "__main__":
    sys.exit(main())
