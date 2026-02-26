#!/usr/bin/env python3
"""Collect latest platform/pre-commit workflow status for a PR head SHA."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from typing import Any

from workflow_runs import list_workflow_runs, parse_csv_list


def latest_runs_by_name(
    runs: list[dict[str, Any]],
    names: set[str],
    event: str,
) -> dict[str, dict[str, Any]]:
    latest: dict[str, dict[str, Any]] = {}
    for run in runs:
        name = str(run.get("name", ""))
        if name not in names:
            continue
        if str(run.get("event", "")) != event:
            continue
        existing = latest.get(name)
        if existing is None or str(run.get("created_at", "")) > str(existing.get("created_at", "")):
            latest[name] = run
    return latest


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
    lines = ["| Platform | Status | Details |", "|----------|--------|--------|"]
    for name in platforms:
        info = states[name]
        link = f"[View]({info['url']})" if info["url"] else "-"
        lines.append(f"| {name} | {info['status']} | {link} |")
    return "\n".join(lines)


def write_output(key: str, value: str) -> None:
    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        return

    if "\n" in value:
        value_hash = hashlib.sha256(value.encode("utf-8")).hexdigest()[:12]
        delim = f"EOF_{key}_{value_hash}"
        while delim in value:
            delim = f"{delim}_X"
        with open(github_output, "a", encoding="utf-8") as f:
            f.write(f"{key}<<{delim}\n{value}\n{delim}\n")
    else:
        with open(github_output, "a", encoding="utf-8") as f:
            f.write(f"{key}={value}\n")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Collect build status for PR build-results comment.")
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

    runs = list_workflow_runs(args.repo, args.head_sha)

    if args.runs_cache:
        with open(args.runs_cache, "w", encoding="utf-8") as f:
            json.dump(runs, f)

    latest = latest_runs_by_name(runs, target_names, args.event)

    states = {name: platform_status(latest.get(name)) for name in platforms}
    precommit = precommit_status(latest.get("pre-commit"))

    table = render_table(platforms, states)
    platform_conclusions = [states[name]["conclusion"] for name in platforms]
    all_complete = all(c in {"success", "failure", "cancelled"} for c in platform_conclusions)
    all_success = all(c == "success" for c in platform_conclusions)
    summary = "All builds passed." if all_complete and all_success else (
        "Some builds failed." if all_complete else "Some builds still in progress."
    )

    write_output("table", table)
    write_output("summary", summary)
    write_output("all_complete", "true" if all_complete else "false")
    write_output("precommit_status", precommit["status"])
    write_output("precommit_url", precommit["url"])
    write_output("precommit_conclusion", precommit["conclusion"])
    write_output("precommit_run_id", precommit["run_id"])

    print(table)
    print(summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
