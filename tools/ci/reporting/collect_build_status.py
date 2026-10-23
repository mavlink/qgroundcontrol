#!/usr/bin/env python3
"""Collect the latest platform, pre-commit, and Code Analysis workflow status for a head SHA.

Also the build-results gate: `ready` is true once every platform workflow has
completed (with --require-success, successfully). With --status-out it writes each
workflow's state, duration, and jobs (failed steps with log links) for the PR
comment. It saves the latest run per workflow to --runs-cache for download_artifacts.py.
The repository, head SHA, and event default to the triggering workflow_run, the platform
workflows to build-config.json, and the snapshot path to RUNNER_TEMP (see
workflow_runs.add_workflow_run_query_args). Uses only the standard library, so it runs
before the job installs Python dependencies.
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
    write_github_output,
)
from common.io import write_json

from ci.reporting.workflow_runs import (
    add_workflow_run_query_args,
    evaluate_runs,
    parse_created_at,
    select_latest_runs_by_name,
)

PRECOMMIT_WORKFLOW = "pre-commit"
ANALYSIS_WORKFLOW = "Code Analysis"

_FAILED_CONCLUSIONS = {"failure", "timed_out", "startup_failure"}
# Jobs that run even when change detection skips every build: the planners and the
# `<workflow>-ok` gate, which is named after its workflow.
_PLANNER_JOB = re.compile(r"(?i)detect changes|^plan\b")
# RunsOn fails a job whose EC2 Spot instance AWS reclaimed, annotating it with this message, and re-runs
# the run's failed jobs once it completes after attempts 1 and 2; attempt 3 is final.
_SPOT_INTERRUPTION = "AWS interrupted the EC2 Spot instance"
_RUNS_ON_FINAL_ATTEMPT = 3


def _seconds_between(start: Any, end: Any) -> float | None:
    started = parse_created_at(start)
    finished = parse_created_at(end)
    if started is None or finished is None or finished < started:
        return None
    return (finished - started).total_seconds()


def spot_interrupted(repo: str, job: dict[str, Any]) -> bool:
    """Whether RunsOn failed this job because AWS reclaimed its Spot instance."""
    if job.get("conclusion") not in _FAILED_CONCLUSIONS:
        return False
    if not str(job.get("runner_name") or "").startswith("runs-on--"):
        return False
    try:
        annotations = gh_api_json(f"repos/{repo}/check-runs/{int(job['id'])}/annotations")
    except Exception as exc:
        gh_warning(f"Cannot read the annotations of job {job.get('name')}: {exc}")
        return False
    return isinstance(annotations, list) and any(
        isinstance(annotation, dict) and _SPOT_INTERRUPTION in str(annotation.get("message", ""))
        for annotation in annotations
    )


def summarize_job(job: dict[str, Any], *, interrupted: bool = False) -> dict[str, Any]:
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
        "interrupted": interrupted,
    }


def skipped_by_change_detection(workflow: str, jobs: list[dict[str, Any]]) -> bool:
    """True when every build job was skipped and only planners and the gate ran."""
    ran = [job for job in jobs if job["conclusion"] != "skipped"]
    return len(ran) < len(jobs) and all(
        job["name"] == workflow or _PLANNER_JOB.search(job["name"]) for job in ran
    )


def workflow_state(run: dict[str, Any] | None, jobs: list[dict[str, Any]], name: str) -> str:
    """Classify a run as success, failure, interrupted, cancelled, skipped, running, pending, or
    action_required.

    `interrupted`: every failed leg lost its Spot instance and RunsOn will re-run them.
    """
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
    # The <workflow>-ok gate fails with its legs.
    failed = [
        job for job in jobs if job["conclusion"] in _FAILED_CONCLUSIONS and job["name"] != name
    ]
    if (
        failed
        and all(job.get("interrupted") for job in failed)
        and int(run.get("run_attempt") or 1) < _RUNS_ON_FINAL_ATTEMPT
    ):
        return "interrupted"
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
            jobs = [
                summarize_job(job, interrupted=spot_interrupted(repo, job))
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
        "duration_seconds": _run_duration(run) if run else None,
        "jobs": jobs,
    }


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

    write_github_output({"ready": "true" if ready else "false"})

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
