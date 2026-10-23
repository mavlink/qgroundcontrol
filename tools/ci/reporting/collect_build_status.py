#!/usr/bin/env python3
"""Collect the latest platform, pre-commit, Code Analysis, and other workflow status for a head SHA.

With --status-out it writes each workflow's state, duration, attempt, and jobs (failed steps
with log links, and runners lost mid-job) for the PR comment, plus the state of every other
workflow that ran for the commit. It saves the latest platform, pre-commit, and Code Analysis
run to --runs-cache for download_artifacts.py. The repository, head SHA, and event default to
the triggering workflow_run, the platform workflows to build-config.json, and the snapshot path
to RUNNER_TEMP (see workflow_runs.add_workflow_run_query_args). Uses only the standard library,
so it runs before the job installs Python dependencies.
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
    gh_api_json,
    gh_warning,
    list_run_jobs,
    list_workflow_runs_for_sha,
    parse_csv_list,
    seconds_between,
)
from common.io import write_json

from ci.reporting.workflow_runs import add_workflow_run_query_args, select_latest_runs_by_name

PRECOMMIT_WORKFLOW = "pre-commit"
ANALYSIS_WORKFLOW = "Code Analysis"

_FAILED_CONCLUSIONS = {"failure", "timed_out", "startup_failure"}
# Jobs that run even when change detection skips every build: the planners and the
# `<workflow>-ok` gate, which is named after its workflow.
_PLANNER_JOB = re.compile(r"(?i)detect changes|^plan\b")
# GitHub's annotation when a hosted ("The hosted runner: ...") or self-hosted ("The self-hosted
# runner: ...") runner stops responding; the job fails without logs for its last step.
_LOST_RUNNER = "lost communication with the server"


def _lost_runner_annotation(repo: str, job: dict[str, Any]) -> bool:
    try:
        annotations = gh_api_json(f"repos/{repo}/check-runs/{int(job['id'])}/annotations")
    except Exception as exc:
        gh_warning(f"Cannot read the annotations of job {job.get('name')}: {exc}")
        return False
    return isinstance(annotations, list) and any(
        isinstance(annotation, dict) and _LOST_RUNNER in str(annotation.get("message", ""))
        for annotation in annotations
    )


def runner_lost(repo: str, job: dict[str, Any]) -> dict[str, str] | None:
    """Return the step a failed job's runner was lost during, with the job's start and end.

    A lost runner leaves its step in progress and the rest pending. A job whose steps all
    finished without one failing has its annotations read for GitHub's lost-runner message;
    other jobs need no API call.
    """
    if job.get("conclusion") != "failure":
        return None
    steps = [step for step in job.get("steps") or [] if isinstance(step, dict)]
    unfinished = [step for step in steps if step.get("status") != "completed"]
    if unfinished:
        lost = next(
            (step for step in unfinished if step.get("status") == "in_progress"), unfinished[0]
        )
    elif any(step.get("conclusion") in _FAILED_CONCLUSIONS for step in steps):
        return None
    elif _lost_runner_annotation(repo, job):
        started = [step for step in steps if step.get("started_at")]
        lost = started[-1] if started else {}
    else:
        return None
    return {
        "step": str(lost.get("name", "")),
        "started_at": str(job.get("started_at") or ""),
        "completed_at": str(job.get("completed_at") or ""),
    }


def summarize_job(job: dict[str, Any], *, lost: dict[str, str] | None = None) -> dict[str, Any]:
    """Return a job's state, duration, failed steps with direct log links, and lost runner."""
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
        "duration_seconds": seconds_between(job.get("started_at"), job.get("completed_at")),
        "failed_steps": failed_steps,
        "runner_lost": lost,
    }


def skipped_by_change_detection(workflow: str, jobs: list[dict[str, Any]]) -> bool:
    """True when every build job was skipped and only planners and the gate ran."""
    ran = [job for job in jobs if job["conclusion"] != "skipped"]
    return len(ran) < len(jobs) and all(
        job["name"] == workflow or _PLANNER_JOB.search(job["name"]) for job in ran
    )


def workflow_state(run: dict[str, Any] | None, jobs: list[dict[str, Any]], name: str) -> str:
    """Classify a run as success, failure, cancelled, skipped, running, pending, or
    action_required."""
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
    """The latest attempt's duration."""
    if run.get("status") != "completed":
        return None
    return seconds_between(
        run.get("run_started_at") or run.get("created_at"), run.get("updated_at")
    )


def _run_elapsed(run: dict[str, Any]) -> float | None:
    """Wall time from the run's creation to its latest attempt's end, re-runs included."""
    if run.get("status") != "completed":
        return None
    return seconds_between(run.get("created_at"), run.get("updated_at"))


def describe_workflow(
    repo: str, name: str, run: dict[str, Any] | None, *, with_jobs: bool = True
) -> dict[str, Any]:
    """Return the status-file entry for one workflow's latest run."""
    jobs: list[dict[str, Any]] = []
    if with_jobs and run and int(run.get("id", 0)) > 0:
        try:
            jobs = [
                summarize_job(job, lost=runner_lost(repo, job))
                for job in list_run_jobs(repo, int(run["id"]))
            ]
        except Exception as exc:
            gh_warning(f"Cannot list the jobs of {name} run {run.get('id')}: {exc}")
    return {
        "name": name,
        "state": workflow_state(run, jobs, name),
        "run_id": str(run.get("id", "")) if run else "",
        "url": str(run.get("html_url", "")) if run else "",
        "run_number": run.get("run_number") if run else None,
        "attempt": int(run.get("run_attempt") or 1) if run else None,
        "duration_seconds": _run_duration(run) if run else None,
        "elapsed_seconds": _run_elapsed(run) if run else None,
        "jobs": jobs,
    }


def describe_others(repo: str, latest: dict[str, dict[str, Any]]) -> list[dict[str, Any]]:
    """Describe every other workflow's latest run; only failed runs list their jobs."""
    return [
        describe_workflow(
            repo,
            name,
            run,
            with_jobs=run.get("status") == "completed"
            and run.get("conclusion") in _FAILED_CONCLUSIONS,
        )
        for name, run in sorted(latest.items(), key=lambda item: item[0].casefold())
    ]


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect build status for PR build-results comment."
    )
    add_workflow_run_query_args(parser, default_event="pull_request")
    parser.add_argument(
        "--status-out",
        type=Path,
        default=None,
        help="Write each workflow's state, duration, and jobs to this JSON file (lists jobs)",
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

    if args.status_out is not None:
        other_names = {str(run.get("name", "")) for run in runs} - target_names - {""}
        status = {
            "repo": args.repo,
            "head_sha": args.head_sha,
            "collected_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "platforms": [
                describe_workflow(args.repo, name, latest.get(name)) for name in platforms
            ],
            "precommit": describe_workflow(args.repo, PRECOMMIT_WORKFLOW, precommit)
            if (precommit := latest.get(PRECOMMIT_WORKFLOW))
            else None,
            "analysis": describe_workflow(args.repo, ANALYSIS_WORKFLOW, analysis)
            if (analysis := latest.get(ANALYSIS_WORKFLOW))
            else None,
            "others": describe_others(
                args.repo, select_latest_runs_by_name(runs, other_names, event=args.event)
            ),
        }
        write_json(args.status_out, status)

    for name in platforms:
        run = latest.get(name)
        state = f"{run.get('status')}/{run.get('conclusion') or '-'}" if run else "not started"
        print(f"{name}: {state}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
