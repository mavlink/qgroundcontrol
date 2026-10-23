"""Select the Docker workflow's build legs for plan_platform.py.

The variant set (base images, build args, artifact patterns) is defined once in
deploy/docker/variants.json and shared with run_docker.py and gen_compose.py, so
this planner stays a thin selector over that source.
"""

from __future__ import annotations

import fnmatch
from typing import Any

from qgc_tools.docker_variants import load_variants


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
    push_target: str = "",
) -> list[dict[str, Any]]:
    """Return the matrix include-list; empty when nothing builds.

    ``should_build`` False (a tag that is not a Stable* release tag) yields no legs regardless
    of event. Every leg carries the same ``push_target`` registry ('' for no push).
    """
    if not should_build:
        return []

    selected = {
        "linux": event_name != "pull_request" or linux_changed,
        "android": event_name != "pull_request" or android_changed,
    }

    return [
        {
            "platform": v["platform"],
            "target": v["target"],
            "variant": v["ci_variant"],
            "build_args": build_args_str(v["build_args"]),
            "fuse": v["fuse"],
            "artifact_pattern": v["artifact_pattern"],
            "package_pattern": v["package_pattern"],
            "push_target": push_target,
        }
        for v in load_variants()
        if selected.get(v["selector"], False)
        and (event_name != "pull_request" or full_matrix or v["id"] in {"ubuntu", "android"})
    ]


def needs_full_matrix(files: list[str] | None) -> bool:
    """Unknown diffs and toolchain/package changes require every variant."""
    patterns = (
        ".github/build-config*.json",
        ".github/workflows/docker.yml",
        ".github/actions/docker/**",
        ".github/actions/free-disk-space/**",
        "tools/ci/compile/docker_helper.py",
        "tools/ci/planning/plan_docker_builds.py",
        "tools/ci/planning/plan_platform.py",
        "tools/ci/planning/detect_changes.py",
        ".github/workflows/_detect-changes.yml",
        "tools/ci/publish/attest_helper.py",
        "tools/ci/packaging/validate_native_package.py",
        "tools/ci/security/generate_cpm_sbom.py",
        "tools/ci/packaging/find_artifact.py",
        ".dockerignore",
        ".gitmodules",
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/**",
        "src/CMakeLists.txt",
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
        "tools/moccache/**",
        "tools/moccache_launcher.py",
        "libs/**",
        "android/**",
    )
    return files is None or any(
        fnmatch.fnmatchcase(path, pattern) for path in files for pattern in patterns
    )
