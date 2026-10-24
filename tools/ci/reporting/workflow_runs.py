"""Shared helpers for selecting and grouping GitHub workflow runs."""

from __future__ import annotations

import argparse
import json
import os
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING, Any

from common.build_config import load_build_config, lookup_dotted
from common.gh_actions import parse_github_timestamp
from common.io import read_json

if TYPE_CHECKING:
    from collections.abc import Callable, Mapping

WORKFLOW_EVENTS = ("", "push", "pull_request", "workflow_dispatch", "schedule")
PLATFORM_WORKFLOWS_KEY = "build.platform_workflows"
RUNS_CACHE_NAME = "workflow-runs-cache.json"


def triggering_run(env: Mapping[str, str] | None = None) -> dict[str, Any]:
    """Return the ``workflow_run`` of the event payload at GITHUB_EVENT_PATH, or {} without one."""
    path = (os.environ if env is None else env).get("GITHUB_EVENT_PATH", "")
    if not path:
        return {}
    event = read_json(Path(path))
    run = event.get("workflow_run") if isinstance(event, dict) else None
    return run if isinstance(run, dict) else {}


def default_runs_cache() -> str:
    """Return the job's shared workflow-run snapshot path in RUNNER_TEMP, or '' outside a runner."""
    runner_temp = os.environ.get("RUNNER_TEMP", "")
    return str(Path(runner_temp) / RUNS_CACHE_NAME) if runner_temp else ""


def configured_platform_workflows() -> str:
    """Return build-config.json's platform workflow names; raise ValueError when unavailable."""
    try:
        value = lookup_dotted(load_build_config(), PLATFORM_WORKFLOWS_KEY, "")
    except (OSError, ValueError) as exc:
        raise ValueError(
            f"cannot read {PLATFORM_WORKFLOWS_KEY} from build-config.json: {exc}"
        ) from exc
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{PLATFORM_WORKFLOWS_KEY} is missing or empty in build-config.json")
    return value


def _workflow_names(value: str) -> str:
    if value.strip():
        return value
    try:
        return configured_platform_workflows()
    except ValueError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from exc


@dataclass(frozen=True)
class WorkflowRun:
    """Validated fields used to select runs; API-specific payload stays with the caller."""

    name: str = ""
    event: str = ""
    status: str = ""
    conclusion: str = ""
    created_at: str = ""

    @classmethod
    def from_mapping(cls, data: dict[str, Any]) -> WorkflowRun:
        fields = {}
        for key in ("name", "event", "status", "conclusion", "created_at"):
            value = data.get(key, "")
            if key == "conclusion" and value is None:
                value = ""
            if not isinstance(value, str):
                raise WorkflowRunsFileError(f"workflow run '{key}' must be a string")
            fields[key] = value
        return cls(**fields)

    def matches(self, names: set[str], event: str, status: str, conclusion: str) -> bool:
        return (
            self.name in names
            and (not event or self.event == event)
            and (not status or self.status == status)
            and (not conclusion or self.conclusion == conclusion)
        )


class WorkflowRunsFileError(ValueError):
    """Raised when cached workflow-run JSON cannot be used."""


def add_workflow_run_query_args(
    parser: argparse.ArgumentParser,
    *,
    default_event: str,
    workflows_option: str = "--platform-workflows",
    workflows_dest: str = "platform_workflows",
    restrict_event: bool = False,
) -> None:
    """Add repository, SHA, workflow, event, and workflow-run snapshot arguments.

    Their defaults come from the runner (GITHUB_REPOSITORY, RUNNER_TEMP), the triggering
    ``workflow_run`` payload, and build-config.json, so build-results.yml passes none of them.
    """
    run = triggering_run()
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    head_sha = str(run.get("head_sha") or "")
    parser.add_argument(
        "--repo",
        default=repo,
        required=not repo,
        help="Repository in owner/repo format (default: $GITHUB_REPOSITORY)",
    )
    parser.add_argument(
        "--head-sha",
        default=head_sha,
        required=not head_sha,
        help="Commit SHA to inspect (default: the triggering workflow run's)",
    )
    # argparse converts a string default through `type` only when the option is absent, so
    # build-config.json is read, and must be valid, only then.
    parser.add_argument(
        workflows_option,
        dest=workflows_dest,
        type=_workflow_names,
        default="",
        help=f"Comma-separated workflow names (default: {PLATFORM_WORKFLOWS_KEY})",
    )
    event_kwargs: dict[str, Any] = {}
    if restrict_event:
        event_kwargs["choices"] = WORKFLOW_EVENTS
    parser.add_argument(
        "--event",
        default=str(run.get("event") or default_event),
        help="Workflow event name to consider (default: the triggering workflow run's)",
        **event_kwargs,
    )
    parser.add_argument(
        "--runs-cache",
        default=default_runs_cache(),
        help=f"Run snapshot the reporting steps share (default: $RUNNER_TEMP/{RUNS_CACHE_NAME})",
    )


def load_workflow_runs(path: Path) -> list[dict[str, Any]]:
    """Load and validate a cached JSON list of GitHub workflow-run objects."""
    try:
        data = read_json(path)
    except (json.JSONDecodeError, OSError) as exc:
        raise WorkflowRunsFileError(f"failed to read runs file {path}: {exc}") from exc
    if not isinstance(data, list):
        raise WorkflowRunsFileError(f"runs file {path} must contain a JSON list of workflow runs")
    if not all(isinstance(run, dict) for run in data):
        raise WorkflowRunsFileError(f"runs file {path} must contain only workflow-run objects")
    for run in data:
        WorkflowRun.from_mapping(run)
    return data


def resolve_workflow_runs(
    repo: str,
    head_sha: str,
    runs_cache: str,
    fetcher: Callable[[str, str], list[dict[str, Any]]],
) -> list[dict[str, Any]] | None:
    """Load the *runs_cache* snapshot if it exists, else call *fetcher*; report snapshot errors."""
    if not runs_cache or not Path(runs_cache).is_file():
        return fetcher(repo, head_sha)
    try:
        return load_workflow_runs(Path(runs_cache))
    except WorkflowRunsFileError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return None


def is_newer_run(candidate: dict[str, Any], existing: dict[str, Any]) -> bool:
    """Return True when *candidate* is newer than *existing*."""
    candidate_created_at = str(candidate.get("created_at", ""))
    existing_created_at = str(existing.get("created_at", ""))
    candidate_dt = parse_github_timestamp(candidate_created_at)
    existing_dt = parse_github_timestamp(existing_created_at)
    if candidate_dt is not None and existing_dt is not None:
        return candidate_dt > existing_dt
    return candidate_created_at > existing_created_at


def select_latest_runs_by_name(
    runs: list[dict[str, Any]],
    names: set[str],
    *,
    event: str = "",
    status: str = "",
    conclusion: str = "",
) -> dict[str, dict[str, Any]]:
    """Return the latest workflow run per name after optional filtering."""
    latest: dict[str, dict[str, Any]] = {}
    for run in runs:
        record = WorkflowRun.from_mapping(run)
        if not record.matches(names, event, status, conclusion):
            continue
        name = record.name
        existing = latest.get(name)
        if existing is None or is_newer_run(run, existing):
            latest[name] = run
    return latest


def group_runs_by_name(
    runs: list[dict[str, Any]],
    names: list[str],
    *,
    event: str = "",
    status: str = "",
    conclusion: str = "",
) -> dict[str, list[dict[str, Any]]]:
    """Group workflow runs by name after optional filtering, newest first."""
    target = set(names)
    grouped: dict[str, list[dict[str, Any]]] = {name: [] for name in names}
    for run in runs:
        record = WorkflowRun.from_mapping(run)
        if record.matches(target, event, status, conclusion):
            grouped[record.name].append(run)

    for name in grouped:
        grouped[name].sort(
            key=lambda run: (
                parse_github_timestamp(run.get("created_at")) is not None,
                parse_github_timestamp(run.get("created_at"))
                or datetime.min.replace(tzinfo=timezone.utc),
                str(run.get("created_at", "")),
            ),
            reverse=True,
        )
    return grouped
