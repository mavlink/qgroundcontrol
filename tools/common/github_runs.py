"""Shared helpers for selecting and grouping GitHub workflow runs."""

from __future__ import annotations

from datetime import datetime
from typing import Any


def parse_created_at(created_at: Any) -> datetime | None:
    """Parse GitHub ISO-8601 timestamps into datetimes."""
    value = str(created_at).strip()
    if not value:
        return None
    if value.endswith("Z"):
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
