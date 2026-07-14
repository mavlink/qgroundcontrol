"""Plan Docker build matrix entries for the Docker workflow.

The variant set (base images, build args, artifact patterns) is defined once in
deploy/docker/variants.json and shared with run-docker.sh and gen_compose.py, so
this planner stays a thin selector over that source.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import parse_bool, write_github_output

_docker_dir = Path(__file__).resolve().parents[2] / "deploy" / "docker"
if str(_docker_dir) not in sys.path:
    sys.path.insert(0, str(_docker_dir))

from _variants import load_variants  # pyright: ignore[reportMissingImports]


def build_args_str(build_args: dict[str, str]) -> str:
    """Render an ordered build-arg map as the newline-joined KEY=VALUE the action expects."""
    return "\n".join(f"{key}={value}" for key, value in build_args.items())


def plan_builds(event_name: str, linux_changed: bool, android_changed: bool) -> dict[str, Any]:
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
            "security_category": v["security_category"],
            "target": v["target"],
            "variant": v["ci_variant"],
            "build_args": build_args_str(v["build_args"]),
            "fuse": v["fuse"],
            "artifact_pattern": v["artifact_pattern"],
            "package_pattern": v["package_pattern"],
        }
        for v in load_variants()
        if selected.get(v["selector"], False)
    ]

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
