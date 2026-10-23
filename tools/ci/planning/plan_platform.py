#!/usr/bin/env python3
"""Plan a platform workflow's builds for the current GitHub event.

Runs change detection and the release-tag gate once (detect_changes.py), then, for Android and
Docker, their build-matrix planner. Writes `should_build` and `matrix`, the build legs as a JSON
include-list (empty for platforms whose workflow defines its own matrix). Android and Docker
build only when their matrix has legs; each Docker leg carries the image's push target.

Usage:
    python3 tools/ci/planning/plan_platform.py --platform android
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, write_github_output
from qgc_tools.docker_variants import resolve_push_target
from qgc_tools.release_tag_check import GitQueryError

from ci.planning.android_matrix import build_matrix
from ci.planning.detect_changes import detect_builds
from ci.planning.plan_docker_builds import needs_full_matrix, plan_builds

PLATFORMS = ("linux", "windows", "macos", "ios", "android", "docker")


def plan(platform: str) -> tuple[bool, list[dict[str, Any]]]:
    """Return whether *platform* builds and its matrix include-list.

    Raises GitQueryError when git cannot tell whether a tag is on a Stable* branch.
    """
    event_name = os.environ.get("EVENT_NAME", "")
    is_pr = event_name == "pull_request"
    if platform == "android":
        include = build_matrix(is_pr) if detect_builds([platform]).builds[platform] else []
    elif platform == "docker":
        detection = detect_builds(("docker-linux", "docker-android"))
        include = plan_builds(
            event_name,
            detection.builds["docker-linux"],
            detection.builds["docker-android"],
            full_matrix=not is_pr or needs_full_matrix(detection.changed_files),
            # Pull requests select each platform above; elsewhere every verdict is the release-tag gate.
            should_build=is_pr or all(detection.builds.values()),
            push_target=resolve_push_target(
                event_name,
                os.environ.get("GITHUB_REPOSITORY", ""),
                os.environ.get("GITHUB_REF", ""),
            ),
        )
    else:
        return detect_builds([platform]).builds[platform], []
    return bool(include), include


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", required=True, choices=PLATFORMS)
    args = parser.parse_args(argv)
    try:
        should_build, include = plan(args.platform)
    except GitQueryError as exc:
        gh_error(f"Cannot determine whether tag is on a Stable* branch: {exc}")
        return 1
    matrix = json.dumps(include, separators=(",", ":"))
    print(f"{args.platform}: should_build={should_build}\nmatrix={matrix}")
    write_github_output({"should_build": "true" if should_build else "false", "matrix": matrix})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
