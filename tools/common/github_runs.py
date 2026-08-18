"""Shared helpers for selecting and grouping GitHub workflow runs."""

from __future__ import annotations

import json
import sys
from datetime import datetime
from pathlib import Path
from typing import TYPE_CHECKING, Any

from .io import read_json

if TYPE_CHECKING:
    from collections.abc import Callable


class WorkflowRunsFileError(ValueError):
    """Raised when cached workflow-run JSON cannot be used."""


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
        return datetime.fromisoformat(value)
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
        name = str(run.get("name", ""))
        if name not in names:
            continue
        if event and str(run.get("event", "")) != event:
            continue
        if status and str(run.get("status", "")) != status:
            continue
        if conclusion and str(run.get("conclusion", "")) != conclusion:
            continue
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
        name = str(run.get("name", ""))
        if name not in target:
            continue
        if event and str(run.get("event", "")) != event:
            continue
        if status and str(run.get("status", "")) != status:
            continue
        if conclusion and str(run.get("conclusion", "")) != conclusion:
            continue
        grouped[name].append(run)

    for name in grouped:
        grouped[name].sort(
            key=lambda run: (
                parse_created_at(run.get("created_at")) is not None,
                str(run.get("created_at", "")),
            ),
            reverse=True,
        )
    return grouped
