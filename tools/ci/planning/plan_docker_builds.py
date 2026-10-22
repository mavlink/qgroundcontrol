#!/usr/bin/env python3
"""Plan Docker build matrix entries for the Docker workflow.

The variant set (base images, build args, artifact patterns) is defined once in
deploy/docker/variants.json and shared with run_docker.py and gen_compose.py, so
this planner stays a thin selector over that source.
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import write_github_output
from qgc_tools.docker_variants import load_variants
from qgc_tools.release_tag_check import GitQueryError

from ci.planning.detect_changes import detect_builds, report_tag_query_error


def build_args_str(build_args: dict[str, str]) -> str:
    """Render an ordered build-arg map as the newline-joined KEY=VALUE the action expects."""
    return "\n".join(f"{key}={value}" for key, value in build_args.items())


def plan_builds(
    event_name: str,
    linux_changed: bool,
    android_changed: bool,
    *,
    full_matrix: bool = False,
    should_build: bool = True,
) -> dict[str, Any]:
    """Return workflow matrix and a has_jobs flag.

    Returns {"matrix": {"include": [...]}, "has_jobs": bool}. Typed as
    dict[str, Any] so callers can subscript matrix["include"] without
    pyright complaining about object indexing.

    ``should_build`` False (a tag that is not a Stable* release tag) yields an
    empty matrix regardless of event.
    """
    if not should_build:
        return {"matrix": {"include": []}, "has_jobs": False}

    selected = {
        "linux": event_name != "pull_request" or linux_changed,
        "android": event_name != "pull_request" or android_changed,
    }

    include: list[dict[str, Any]] = [
        {
            "platform": v["platform"],
            "target": v["target"],
            "variant": v["ci_variant"],
            "build_args": build_args_str(v["build_args"]),
            "fuse": v["fuse"],
            "artifact_pattern": v["artifact_pattern"],
            "package_pattern": v["package_pattern"],
        }
        for v in load_variants()
        if selected.get(v["selector"], False)
        and (event_name != "pull_request" or full_matrix or v["id"] in {"ubuntu", "android"})
    ]

    return {"matrix": {"include": include}, "has_jobs": bool(include)}


def needs_full_matrix(files: list[str] | None) -> bool:
    """Unknown diffs and toolchain/package changes require every variant."""
    patterns = (
        ".github/build-config*.json",
        ".github/workflows/docker.yml",
        ".github/actions/docker/**",
        ".github/actions/free-disk-space/**",
        "tools/ci/compile/docker_helper.py",
        "tools/ci/planning/plan_docker_builds.py",
        "tools/ci/planning/detect_changes.py",
        "tools/ci/packaging/validate_native_package.py",
        "tools/ci/security/generate_cpm_sbom.py",
        "tools/ci/packaging/find_artifact.py",
        ".dockerignore",
        ".gitmodules",
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/**",
        "src/**/CMakeLists.txt",
        "deploy/docker/**",
        "deploy/linux/**",
        "tools/setup/**",
        "tools/common/**",
        "tools/qgc_tools/**",
        "tools/_bootstrap.py",
        "tools/pyproject.toml",
        "tools/uv.lock",
        "tools/configs/ccache.conf",
        "tools/moccache.py",
        "libs/**",
        "android/**",
    )
    return files is None or any(
        fnmatch.fnmatchcase(path, pattern) for path in files for pattern in patterns
    )


def main(argv: list[str] | None = None) -> int:
    """Compute the Docker build matrix for the current event and emit outputs."""
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    try:
        detection = detect_builds(("docker-linux", "docker-android"))
    except GitQueryError as exc:
        return report_tag_query_error(exc)

    event_name = os.environ.get("EVENT_NAME", "")
    is_pr = event_name == "pull_request"
    plan = plan_builds(
        event_name,
        detection.builds["docker-linux"],
        detection.builds["docker-android"],
        full_matrix=not is_pr or needs_full_matrix(detection.changed_files),
        # Pull requests select each platform above; elsewhere every verdict is the release-tag gate.
        should_build=is_pr or all(detection.builds.values()),
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
