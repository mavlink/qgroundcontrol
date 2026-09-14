#!/usr/bin/env python3
"""Check whether baseline caches should be updated for a commit SHA."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import list_workflow_runs_for_sha, parse_csv_list, write_github_output
from common.io import read_json, write_json
from qgc_tools.workflow_runs import evaluate_runs, select_latest_runs_by_name


def evaluate_readiness(
    runs: list[dict[str, Any]],
    platforms: list[str],
    event: str = "push",
) -> tuple[bool, list[str], list[str], list[str]]:
    """Return readiness and missing/incomplete/failed platform workflow lists."""
    return evaluate_runs(runs, platforms, event)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check baseline readiness for build-results workflow."
    )
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
        "--runs-input",
        default="",
        help="Path to read pre-fetched workflow runs JSON; skips the API call",
    )
    parser.add_argument(
        "--runs-cache",
        default="",
        help="Path to write cached workflow runs JSON for downstream scripts",
    )
    parser.add_argument(
        "--require-complete", action="store_true", help="Accept any completed conclusion"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    platforms = parse_csv_list(args.platform_workflows)
    if args.runs_input:
        runs = read_json(Path(args.runs_input))
    else:
        runs = list_workflow_runs_for_sha(args.repo, args.head_sha)

    if args.runs_cache:
        write_json(
            Path(args.runs_cache),
            list(
                select_latest_runs_by_name(
                    runs, set(platforms) | {"pre-commit"}, event=args.event
                ).values()
            ),
        )

    ready, missing, incomplete, failed = evaluate_runs(
        runs, platforms, args.event, require_success=not args.require_complete
    )

    write_github_output(
        {
            "ready": "true" if ready else "false",
            "missing": ",".join(missing),
            "incomplete": ",".join(incomplete),
            "failed": ",".join(failed),
        }
    )

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
