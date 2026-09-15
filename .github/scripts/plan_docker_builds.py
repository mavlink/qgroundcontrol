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

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import parse_bool, write_github_output

VARIANTS_JSON = Path(__file__).resolve().parents[2] / "deploy" / "docker" / "variants.json"
from qgc_tools.docker_variants import load_variants


def build_args_str(build_args: dict[str, str]) -> str:
    """Render an ordered build-arg map as the newline-joined KEY=VALUE the action expects."""
    return "\n".join(f"{key}={value}" for key, value in build_args.items())


def plan_builds(
    event_name: str, linux_changed: bool, android_changed: bool, *, full_matrix: bool = False
) -> dict[str, Any]:
    """Return workflow matrix and a has_jobs flag.

    Returns {"matrix": {"include": [...]}, "has_jobs": bool}. Typed as
    dict[str, Any] so callers can subscript matrix["include"] without
    pyright complaining about object indexing.
    """
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
        ".github/workflows/_detect-changes.yml",
        ".github/actions/docker/**",
        ".github/actions/free-disk-space/**",
        ".github/scripts/docker_helper.py",
        ".github/scripts/plan_docker_builds.py",
        ".github/scripts/detect_changes.py",
        ".github/scripts/ci_bootstrap.py",
        ".github/scripts/validate_native_package.py",
        ".github/scripts/generate_cpm_sbom.py",
        ".github/scripts/find_artifact.py",
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
    from detect_changes import get_changed_files

    plan = plan_builds(
        args.event_name,
        parse_bool(args.linux),
        parse_bool(args.android),
        full_matrix=needs_full_matrix(get_changed_files())
        if args.event_name == "pull_request"
        else True,
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
