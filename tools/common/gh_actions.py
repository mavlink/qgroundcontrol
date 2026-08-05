#!/usr/bin/env python3
"""Shared helpers for GitHub Actions API access via gh CLI."""

from __future__ import annotations

import json
import os
import subprocess
from typing import Any


def gh(*args: str, check: bool = True) -> subprocess.CompletedProcess:
    """Run a gh CLI command and return the process result."""
    return subprocess.run(
        ["gh", *args],
        capture_output=True,
        text=True,
        check=check,
    )


def _paginate_items(path: str, item_key: str, params: dict[str, str]) -> list[dict[str, Any]]:
    """Fetch all pages of a GitHub list endpoint and return the unpacked items.

    ``gh --paginate --jq`` applies the filter per page and streams one JSON
    value per line (NDJSON), so the list is unpacked server-side; ``[]?``
    tolerates pages where the key is absent.
    """
    cmd = ["api", "--method", "GET", "--paginate", "--jq", f".{item_key}[]?", path]
    for key, value in params.items():
        cmd += ["-F", f"{key}={value}"]
    result = gh(*cmd)
    return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]


def list_workflow_runs_for_sha(repo: str, head_sha: str) -> list[dict[str, Any]]:
    """List workflow runs for a commit SHA across all paginated API results."""
    return _paginate_items(
        f"repos/{repo}/actions/runs",
        "workflow_runs",
        {"head_sha": head_sha, "per_page": "100"},
    )


def list_run_artifacts(repo: str, run_id: int | str) -> list[dict[str, Any]]:
    """List artifacts for a workflow run across all pages."""
    try:
        run_id_int = int(run_id)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"run_id must be an integer, got {run_id!r}") from exc
    if run_id_int <= 0:
        raise ValueError(f"run_id must be positive, got {run_id_int}")

    return _paginate_items(
        f"repos/{repo}/actions/runs/{run_id_int}/artifacts",
        "artifacts",
        {"per_page": "100"},
    )


def parse_csv_list(value: str) -> list[str]:
    """Parse comma-separated values into a trimmed non-empty list."""
    return [item.strip() for item in value.split(",") if item.strip()]


def is_fork_pr() -> bool:
    """Check if the current event is a PR from a fork repository."""
    event = os.environ.get("EVENT_NAME", os.environ.get("GITHUB_EVENT_NAME", ""))
    if event != "pull_request":
        return False
    pr_repo = os.environ.get("PR_REPO", "").strip()
    this_repo = os.environ.get("THIS_REPO", os.environ.get("GITHUB_REPOSITORY", "")).strip()
    return bool(pr_repo and this_repo and pr_repo != this_repo)


def resolve_cache_policy(requested: str) -> str:
    """Resolve cache save policy.

    "auto" only saves on non-PR events (push, schedule, workflow_dispatch).
    PRs read from the shared cache but never write, so the 10 GB repo cap
    isn't churned by per-PR entries. Long-lived cache state is owned by
    push-to-default-branch builds.
    """
    if requested != "auto":
        return requested
    event = os.environ.get("EVENT_NAME", os.environ.get("GITHUB_EVENT_NAME", ""))
    if event in {"pull_request", "pull_request_target"}:
        return "false"
    return "false" if is_fork_pr() else "true"


def write_github_output(outputs: dict[str, str]) -> None:
    """Write key=value pairs to $GITHUB_OUTPUT for GitHub Actions.

    Handles multiline values using hash-based heredoc delimiters.
    """
    import hashlib

    github_output = os.environ.get('GITHUB_OUTPUT')
    if not github_output:
        return

    with open(github_output, 'a', encoding="utf-8") as f:
        for key, value in outputs.items():
            if "\n" in value:
                value_hash = hashlib.sha256(value.encode("utf-8")).hexdigest()[:12]
                delim = f"EOF_{key}_{value_hash}"
                while delim in value:
                    delim = f"{delim}_X"
                f.write(f"{key}<<{delim}\n{value}\n{delim}\n")
            else:
                f.write(f"{key}={value}\n")


def write_step_summary(markdown: str) -> None:
    """Append markdown content to $GITHUB_STEP_SUMMARY."""
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        f.write(markdown)


def append_github_env(values: dict[str, str]) -> None:
    """Append environment variables to $GITHUB_ENV."""
    path = os.environ.get("GITHUB_ENV")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        for key, value in values.items():
            f.write(f"{key}={value}\n")


def append_github_path(path_entry: str) -> None:
    """Append a path entry to $GITHUB_PATH for subsequent steps."""
    path = os.environ.get("GITHUB_PATH")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        f.write(f"{path_entry}\n")


def parse_bool(value: str | None) -> bool:
    """Parse a CI-style boolean. Accepts 1/true/yes/on (case-insensitive)."""
    if value is None:
        return False
    return value.strip().lower() in {"1", "true", "yes", "on"}


def _escape_annotation(message: str) -> str:
    # Workflow commands are newline-delimited; literal CR/LF truncates the
    # annotation, so percent-encode them (and bare % to keep decoding unambiguous).
    return message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def annotate(level: str, message: str) -> None:
    """Emit a GitHub Actions workflow annotation to stdout.

    level is one of "error", "warning", "notice". The runner parses these from
    the step log and renders them in the run UI; outside CI it is plain text.
    """
    print(f"::{level}::{_escape_annotation(message)}", flush=True)


def gh_error(message: str) -> None:
    """Emit a GitHub Actions error annotation."""
    annotate("error", message)


def gh_warning(message: str) -> None:
    """Emit a GitHub Actions warning annotation."""
    annotate("warning", message)


def gh_notice(message: str) -> None:
    """Emit a GitHub Actions notice annotation."""
    annotate("notice", message)
