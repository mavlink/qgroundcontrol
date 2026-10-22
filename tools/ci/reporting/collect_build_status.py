#!/usr/bin/env python3
"""Collect the latest platform/pre-commit workflow status for a head SHA.

Also the build-results gate: `ready` is true once every platform workflow has
completed (with --require-success, successfully). Uses only the standard library,
so it runs before the job installs Python dependencies.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import list_workflow_runs_for_sha, parse_csv_list, write_github_output
from common.io import write_json
from common.markdown import md_table

from ci.reporting.workflow_runs import (
    DEFAULT_PLATFORM_WORKFLOWS,
    evaluate_runs,
    select_latest_runs_by_name,
)


def platform_status(
    run: dict[str, Any] | None,
) -> dict[str, str]:
    if not run:
        return {"status": "Pending", "conclusion": "pending", "url": ""}

    status = str(run.get("status", "pending"))
    conclusion = str(run.get("conclusion") or "pending")
    url = str(run.get("html_url", ""))

    if status == "completed":
        human = "Passed" if conclusion == "success" else "Failed"
        return {"status": human, "conclusion": conclusion, "url": url}
    if status == "in_progress":
        return {"status": "Running", "conclusion": "in_progress", "url": url}
    return {"status": "Pending", "conclusion": "pending", "url": url}


def precommit_status(run: dict[str, Any] | None) -> dict[str, str]:
    if not run:
        return {"status": "Not Triggered", "conclusion": "not_triggered", "url": "", "run_id": ""}

    status = str(run.get("status", "pending"))
    conclusion = str(run.get("conclusion") or "pending")
    url = str(run.get("html_url", ""))
    run_id = str(run.get("id", ""))

    if status == "completed":
        human = "Passed" if conclusion == "success" else "Failed"
        return {"status": human, "conclusion": conclusion, "url": url, "run_id": run_id}
    if status == "in_progress":
        return {"status": "Running", "conclusion": "in_progress", "url": url, "run_id": run_id}
    return {"status": "Pending", "conclusion": "pending", "url": url, "run_id": run_id}


def render_table(platforms: list[str], states: dict[str, dict[str, str]]) -> str:
    rows = []
    for name in platforms:
        info = states[name]
        link = f"[View]({info['url']})" if info["url"] else "-"
        rows.append([name, info["status"], link])
    return md_table(["Platform", "Status", "Details"], rows)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect build status for PR build-results comment."
    )
    parser.add_argument("--repo", required=True, help="Repository in owner/repo format")
    parser.add_argument("--head-sha", required=True, help="Commit SHA to inspect")
    parser.add_argument(
        "--platform-workflows",
        default=DEFAULT_PLATFORM_WORKFLOWS,
        help="Comma-separated platform workflow names",
    )
    parser.add_argument(
        "--event", default="pull_request", help="Optional workflow event name to consider"
    )
    parser.add_argument(
        "--runs-cache",
        default="",
        help="Path to write cached workflow runs JSON for downstream scripts",
    )
    parser.add_argument(
        "--require-success",
        action="store_true",
        help="Report ready only when every platform workflow succeeded (default: completed)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    platforms = parse_csv_list(args.platform_workflows)
    target_names = set(platforms)
    target_names.add("pre-commit")

    runs = list_workflow_runs_for_sha(args.repo, args.head_sha)
    latest = select_latest_runs_by_name(runs, target_names, event=args.event)
    # Later artifact downloads read this snapshot of the latest run per workflow instead of
    # querying the API again.
    if args.runs_cache:
        write_json(Path(args.runs_cache), list(latest.values()))

    states = {name: platform_status(latest.get(name)) for name in platforms}
    precommit = precommit_status(latest.get("pre-commit"))

    table = render_table(platforms, states)
    platform_conclusions = [states[name]["conclusion"] for name in platforms]
    all_complete, missing, incomplete, failed = evaluate_runs(
        runs, platforms, args.event, require_success=False
    )
    ready = all_complete and not (args.require_success and failed)
    all_success = all(c == "success" for c in platform_conclusions)
    summary = (
        "All builds passed."
        if all_complete and all_success
        else ("Some builds failed." if all_complete else "Some builds still in progress.")
    )

    write_github_output(
        {
            "table": table,
            "summary": summary,
            "ready": "true" if ready else "false",
            "precommit_status": precommit["status"],
            "precommit_url": precommit["url"],
            "precommit_conclusion": precommit["conclusion"],
            "precommit_run_id": precommit["run_id"],
        }
    )

    print(table)
    print(summary)
    if not ready:
        print(
            f"Not ready for {args.head_sha}: Missing=[{', '.join(missing)}] "
            f"Incomplete=[{', '.join(incomplete)}] Failed=[{', '.join(failed)}]"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
