#!/usr/bin/env python3
"""Collect the latest platform, pre-commit, and Code Analysis workflow status for a head SHA.

Also the build-results gate: `ready` is true once every platform workflow has
completed (with --require-success, successfully). With --status-out it writes each
workflow's state, duration, and jobs (failed steps with log links) for the PR
comment. Uses only the standard library, so it runs before the job installs Python
dependencies.
"""

from __future__ import annotations

import argparse
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    gh_warning,
    list_run_jobs,
    list_workflow_runs_for_sha,
    parse_csv_list,
    write_github_output,
)
from common.io import write_json

from ci.reporting.workflow_runs import (
    DEFAULT_PLATFORM_WORKFLOWS,
    evaluate_runs,
    parse_created_at,
    select_latest_runs_by_name,
)

PRECOMMIT_WORKFLOW = "pre-commit"
ANALYSIS_WORKFLOW = "Code Analysis"

_FAILED_CONCLUSIONS = {"failure", "timed_out", "startup_failure"}
# Jobs that run even when change detection skips every build: the planners and the
# `<workflow>-ok` gate, which is named after its workflow.
_PLANNER_JOB = re.compile(r"(?i)detect changes|build-matrix|^plan\b")


def _seconds_between(start: Any, end: Any) -> float | None:
    started = parse_created_at(start)
    finished = parse_created_at(end)
    if started is None or finished is None or finished < started:
        return None
    return (finished - started).total_seconds()


def summarize_job(job: dict[str, Any]) -> dict[str, Any]:
    """Return a job's state, duration, and failed steps with direct log links."""
    url = str(job.get("html_url", ""))
    failed_steps = [
        {
            "name": str(step.get("name", "")),
            "number": int(step.get("number", 0)),
            "url": f"{url}#step:{int(step.get('number', 0))}:1" if url else "",
        }
        for step in job.get("steps") or []
        if isinstance(step, dict) and step.get("conclusion") in _FAILED_CONCLUSIONS
    ]
    return {
        "name": str(job.get("name", "")),
        "status": str(job.get("status", "")),
        "conclusion": str(job.get("conclusion") or ""),
        "url": url,
        "duration_seconds": _seconds_between(job.get("started_at"), job.get("completed_at")),
        "failed_steps": failed_steps,
    }


def skipped_by_change_detection(workflow: str, jobs: list[dict[str, Any]]) -> bool:
    """True when every build job was skipped and only planners and the gate ran."""
    ran = [job for job in jobs if job["conclusion"] != "skipped"]
    return len(ran) < len(jobs) and all(
        job["name"] == workflow or _PLANNER_JOB.search(job["name"]) for job in ran
    )


def workflow_state(run: dict[str, Any] | None, jobs: list[dict[str, Any]], name: str) -> str:
    """Classify a run as success, failure, cancelled, skipped, running, pending, or action_required."""
    if not run:
        return "pending"
    status = str(run.get("status", ""))
    if status == "in_progress":
        return "running"
    if status != "completed":
        return "pending"
    conclusion = str(run.get("conclusion") or "")
    if conclusion == "success":
        return "skipped" if skipped_by_change_detection(name, jobs) else "success"
    if conclusion in {"cancelled", "action_required"}:
        return conclusion
    if conclusion in {"skipped", "neutral"}:
        return "skipped"
    return "failure"


def _run_duration(run: dict[str, Any]) -> float | None:
    if run.get("status") != "completed":
        return None
    return _seconds_between(
        run.get("run_started_at") or run.get("created_at"), run.get("updated_at")
    )


def describe_workflow(repo: str, name: str, run: dict[str, Any] | None) -> dict[str, Any]:
    """Return the status-file entry for one workflow's latest run."""
    jobs: list[dict[str, Any]] = []
    if run and int(run.get("id", 0)) > 0:
        try:
            jobs = [summarize_job(job) for job in list_run_jobs(repo, int(run["id"]))]
        except Exception as exc:
            gh_warning(f"Cannot list the jobs of {name} run {run.get('id')}: {exc}")
    return {
        "name": name,
        "state": workflow_state(run, jobs, name),
        "run_id": str(run.get("id", "")) if run else "",
        "url": str(run.get("html_url", "")) if run else "",
        "run_number": run.get("run_number") if run else None,
        "duration_seconds": _run_duration(run) if run else None,
        "jobs": jobs,
    }


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
        "--status-out",
        type=Path,
        default=None,
        help="Write each workflow's state, duration, and jobs to this JSON file (lists jobs)",
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
    target_names = {*platforms, PRECOMMIT_WORKFLOW, ANALYSIS_WORKFLOW}

    runs = list_workflow_runs_for_sha(args.repo, args.head_sha)
    latest = select_latest_runs_by_name(runs, target_names, event=args.event)
    # Later artifact downloads read this snapshot of the latest run per workflow instead of
    # querying the API again.
    if args.runs_cache:
        write_json(Path(args.runs_cache), list(latest.values()))

    all_complete, missing, incomplete, failed = evaluate_runs(
        latest, platforms, require_success=False
    )
    ready = all_complete and not (args.require_success and failed)
    precommit = latest.get(PRECOMMIT_WORKFLOW)
    analysis = latest.get(ANALYSIS_WORKFLOW)

    if args.status_out is not None:
        status = {
            "repo": args.repo,
            "head_sha": args.head_sha,
            "collected_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "platforms": [
                describe_workflow(args.repo, name, latest.get(name)) for name in platforms
            ],
            "precommit": describe_workflow(args.repo, PRECOMMIT_WORKFLOW, precommit)
            if precommit
            else None,
            "analysis": describe_workflow(args.repo, ANALYSIS_WORKFLOW, analysis)
            if analysis
            else None,
        }
        write_json(args.status_out, status)

    write_github_output(
        {
            "ready": "true" if ready else "false",
            "precommit_run_id": str(precommit.get("id", "")) if precommit else "",
            "analysis_complete": "true"
            if analysis and analysis.get("status") == "completed"
            else "false",
        }
    )

    for name in platforms:
        run = latest.get(name)
        state = f"{run.get('status')}/{run.get('conclusion') or '-'}" if run else "not started"
        print(f"{name}: {state}")
    if not ready:
        print(
            f"Not ready for {args.head_sha}: Missing=[{', '.join(missing)}] "
            f"Incomplete=[{', '.join(incomplete)}] Failed=[{', '.join(failed)}]"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
