#!/usr/bin/env python3
"""Check whether baseline caches should be updated for a commit SHA."""

from __future__ import annotations

import argparse
import json
import os
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import list_workflow_runs_for_sha, parse_csv_list, write_github_output


def evaluate_readiness(
    runs: list[dict[str, Any]],
    platforms: list[str],
    event: str = "push",
) -> tuple[bool, list[str], list[str], list[str]]:
    """Return readiness and missing/incomplete/failed platform workflow lists."""
    latest_by_name: dict[str, dict[str, Any]] = {}
    target = set(platforms)

    for run in runs:
        name = str(run.get("name", ""))
        if name not in target:
            continue
        if str(run.get("event", "")) != event:
            continue
        existing = latest_by_name.get(name)
        if existing is None or str(run.get("created_at", "")) > str(existing.get("created_at", "")):
            latest_by_name[name] = run

    missing = [name for name in platforms if name not in latest_by_name]
    incomplete = [
        name for name in platforms
        if name in latest_by_name and str(latest_by_name[name].get("status", "")) != "completed"
    ]
    failed = [
        name for name in platforms
        if name in latest_by_name
        and str(latest_by_name[name].get("status", "")) == "completed"
        and str(latest_by_name[name].get("conclusion", "")) != "success"
    ]

    ready = not missing and not incomplete and not failed
    return ready, missing, incomplete, failed



def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check baseline readiness for build-results workflow.")
    parser.add_argument("--repo", required=True, help="Repository in owner/repo format")
    parser.add_argument("--head-sha", required=True, help="Commit SHA to inspect")
    parser.add_argument(
        "--platform-workflows",
        default="Linux,Windows,MacOS,Android",
        help="Comma-separated platform workflow names",
    )
    parser.add_argument(
        "--event",
        default="push",
        help="Workflow event name to consider (default: push)",
    )
    parser.add_argument(
        "--runs-cache",
        default="",
        help="Path to write cached workflow runs JSON for downstream scripts",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    platforms = parse_csv_list(args.platform_workflows)
    runs = list_workflow_runs_for_sha(args.repo, args.head_sha)

    if args.runs_cache:
        with open(args.runs_cache, "w", encoding="utf-8") as f:
            json.dump(runs, f)

    ready, missing, incomplete, failed = evaluate_readiness(runs, platforms, args.event)

    write_github_output({
        "ready": "true" if ready else "false",
        "missing": ",".join(missing),
        "incomplete": ",".join(incomplete),
        "failed": ",".join(failed),
    })

    if ready:
        print(f"Baseline ready for {args.head_sha}.")
    else:
        print(
            f"Skipping baseline update for {args.head_sha}. "
            f"Missing=[{', '.join(missing)}] "
            f"Incomplete=[{', '.join(incomplete)}] "
            f"Failed=[{', '.join(failed)}]"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
