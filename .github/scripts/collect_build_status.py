#!/usr/bin/env python3
"""Collect latest platform/pre-commit workflow status for a PR head SHA."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Any

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import list_workflow_runs_for_sha, parse_csv_list, write_github_output
from common.github_runs import select_latest_runs_by_name
from common.io import read_json, write_json
from common.markdown import md_table


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
        default="Linux,Windows,MacOS,Android",
        help="Comma-separated platform workflow names",
    )
    parser.add_argument(
        "--event",
        default="pull_request",
        help="Workflow event name to consider (default: pull_request)",
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
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    platforms = parse_csv_list(args.platform_workflows)
    target_names = set(platforms)
    target_names.add("pre-commit")

    if args.runs_input:
        runs = read_json(Path(args.runs_input))
    else:
        runs = list_workflow_runs_for_sha(args.repo, args.head_sha)

    if args.runs_cache:
        write_json(Path(args.runs_cache), runs)

    latest = select_latest_runs_by_name(runs, target_names, event=args.event)

    states = {name: platform_status(latest.get(name)) for name in platforms}
    precommit = precommit_status(latest.get("pre-commit"))

    table = render_table(platforms, states)
    platform_conclusions = [states[name]["conclusion"] for name in platforms]
    all_complete = all(c in {"success", "failure", "cancelled"} for c in platform_conclusions)
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
            "all_complete": "true" if all_complete else "false",
            "precommit_status": precommit["status"],
            "precommit_url": precommit["url"],
            "precommit_conclusion": precommit["conclusion"],
            "precommit_run_id": precommit["run_id"],
        }
    )

    print(table)
    print(summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
