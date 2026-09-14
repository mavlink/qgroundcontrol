"""Shared helpers for selecting and grouping GitHub workflow runs."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING, Any

from common.io import read_json

if TYPE_CHECKING:
    import argparse
    from collections.abc import Callable

DEFAULT_PLATFORM_WORKFLOWS = "Linux,Windows,MacOS,Android"
WORKFLOW_EVENTS = ("", "push", "pull_request", "workflow_dispatch", "schedule")


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


def evaluate_runs(
    runs: list[dict[str, Any]], platforms: list[str], event: str, *, require_success: bool = True
) -> tuple[bool, list[str], list[str], list[str]]:
    latest = select_latest_runs_by_name(runs, set(platforms), event=event)
    missing = [name for name in platforms if name not in latest]
    incomplete = [
        name for name in platforms if name in latest and latest[name].get("status") != "completed"
    ]
    failed = [
        name
        for name in platforms
        if name in latest
        and latest[name].get("status") == "completed"
        and latest[name].get("conclusion") != "success"
    ]
    return not (missing or incomplete or (require_success and failed)), missing, incomplete, failed


class WorkflowRunsFileError(ValueError):
    """Raised when cached workflow-run JSON cannot be used."""


def add_workflow_run_query_args(
    parser: argparse.ArgumentParser,
    *,
    default_event: str,
    workflows_option: str = "--platform-workflows",
    workflows_dest: str = "platform_workflows",
    runs_option: str = "--runs-input",
    include_runs_cache: bool = False,
    restrict_event: bool = False,
) -> None:
    """Add shared repository, SHA, workflow, event, and cached-run arguments."""
    parser.add_argument("--repo", required=True, help="Repository in owner/repo format")
    parser.add_argument("--head-sha", required=True, help="Commit SHA to inspect")
    parser.add_argument(
        workflows_option,
        dest=workflows_dest,
        default=DEFAULT_PLATFORM_WORKFLOWS,
        help="Comma-separated platform workflow names",
    )
    event_kwargs: dict[str, Any] = {}
    if restrict_event:
        event_kwargs["choices"] = WORKFLOW_EVENTS
    parser.add_argument(
        "--event",
        default=default_event,
        help="Optional workflow event name to consider",
        **event_kwargs,
    )
    parser.add_argument(
        runs_option,
        dest="runs_file",
        default="",
        help="Path to cached workflow runs JSON; skips the API call",
    )
    if include_runs_cache:
        parser.add_argument(
            "--runs-cache",
            default="",
            help="Path to write cached workflow runs JSON for downstream scripts",
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
    runs_file: str,
    fetcher: Callable[[str, str], list[dict[str, Any]]],
) -> list[dict[str, Any]] | None:
    """Load cached runs or call *fetcher*, reporting cached-file errors consistently."""
    if not runs_file:
        return fetcher(repo, head_sha)
    try:
        return load_workflow_runs(Path(runs_file))
    except WorkflowRunsFileError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return None


def parse_created_at(created_at: Any) -> datetime | None:
    """Parse GitHub ISO-8601 timestamps into datetimes."""
    value = str(created_at).strip()
    if not value:
        return None
    if value.endswith("Z"):  # 3.10 fromisoformat rejects the trailing 'Z'
        value = f"{value[:-1]}+00:00"
    try:
        parsed = datetime.fromisoformat(value)
        return (
            parsed.replace(tzinfo=timezone.utc)
            if parsed.tzinfo is None
            else parsed.astimezone(timezone.utc)
        )
    except ValueError:
        return None


def is_newer_run(candidate: dict[str, Any], existing: dict[str, Any]) -> bool:
    """Return True when *candidate* is newer than *existing*."""
    candidate_created_at = str(candidate.get("created_at", ""))
    existing_created_at = str(existing.get("created_at", ""))
    candidate_dt = parse_created_at(candidate_created_at)
    existing_dt = parse_created_at(existing_created_at)
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
                parse_created_at(run.get("created_at")) is not None,
                parse_created_at(run.get("created_at"))
                or datetime.min.replace(tzinfo=timezone.utc),
                str(run.get("created_at", "")),
            ),
            reverse=True,
        )
    return grouped
