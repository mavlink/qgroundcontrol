#!/usr/bin/env python3
"""Shared helpers for GitHub Actions API access via gh CLI."""

from __future__ import annotations

import json
import os
import subprocess
import time
from typing import Any

def gh(*args: str, check: bool = True) -> subprocess.CompletedProcess:
    """Run a gh CLI command and return the process result."""
    return subprocess.run(
        ["gh", *args],
        capture_output=True,
        text=True,
        check=check,
    )


def _github_token() -> str:
    return os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN") or ""


def _should_use_http_api() -> bool:
    mode = os.environ.get("QGC_GH_API_MODE", "").strip().lower()
    return mode == "http" and bool(_github_token())


def _build_http_client():
    import httpx
    transport = httpx.HTTPTransport(retries=3)
    base_url = os.environ.get("GITHUB_API_URL", "https://api.github.com").rstrip("/")
    return httpx.Client(
        base_url=base_url,
        transport=transport,
        timeout=30.0,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "qgc-ci-gh-actions/1.0",
            "X-GitHub-Api-Version": "2022-11-28",
            "Authorization": f"Bearer {_github_token()}",
        },
    )


def _retry_after_seconds(value: str, fallback: float) -> float:
    """Parse a Retry-After header value, falling back to a simple backoff."""
    try:
        retry_after = float(value)
    except (TypeError, ValueError):
        return fallback
    return retry_after if retry_after > 0 else fallback


def _http_get_with_retries(client, url: str, **kwargs) -> Any:
    """GET a GitHub API endpoint with retries for transient status failures."""
    import httpx

    retryable_statuses = {429, 500, 502, 503, 504}

    for attempt in range(1, 4):
        try:
            resp = client.get(url, **kwargs)
            resp.raise_for_status()
            return resp
        except httpx.HTTPStatusError as exc:
            status_code = exc.response.status_code
            if attempt >= 3 or status_code not in retryable_statuses:
                raise
            delay = _retry_after_seconds(exc.response.headers.get("Retry-After", ""), float(attempt))
        except httpx.RequestError:
            if attempt >= 3:
                raise
            delay = float(attempt)

        time.sleep(delay)


def _http_paginated_docs(path: str, params: dict[str, str]) -> list[dict[str, Any]]:
    docs: list[dict[str, Any]] = []
    with _build_http_client() as client:
        resp = _http_get_with_retries(client, f"/{path.lstrip('/')}", params=params)
        docs.append(resp.json())

        while "next" in resp.links:
            resp = _http_get_with_retries(client, resp.links["next"]["url"])
            docs.append(resp.json())
    return docs


def parse_json_documents(stdout: str) -> list[dict[str, Any]]:
    """Parse one or more concatenated JSON documents."""
    docs: list[dict[str, Any]] = []
    decoder = json.JSONDecoder()
    index = 0
    length = len(stdout)

    while index < length:
        while index < length and stdout[index].isspace():
            index += 1
        if index >= length:
            break
        try:
            value, next_index = decoder.raw_decode(stdout, index)
        except json.JSONDecodeError as exc:
            snippet = stdout[index:index + 120].replace("\n", "\\n")
            raise ValueError(f"Failed to parse GitHub API JSON output near: {snippet}") from exc
        if isinstance(value, dict):
            docs.append(value)
        index = next_index

    return docs


def list_workflow_runs_for_sha(repo: str, head_sha: str) -> list[dict[str, Any]]:
    """List workflow runs for a commit SHA across all paginated API results."""
    if _should_use_http_api():
        docs = _http_paginated_docs(
            f"repos/{repo}/actions/runs",
            {
                "head_sha": head_sha,
                "per_page": "100",
            },
        )
    else:
        result = gh(
            "api",
            "--method",
            "GET",
            "--paginate",
            f"repos/{repo}/actions/runs",
            "-F",
            f"head_sha={head_sha}",
            "-F",
            "per_page=100",
        )
        docs = parse_json_documents(result.stdout)

    runs: list[dict[str, Any]] = []
    for doc in docs:
        items = doc.get("workflow_runs", [])
        if isinstance(items, list):
            runs.extend(item for item in items if isinstance(item, dict))
    return runs


def list_run_artifacts(repo: str, run_id: int | str) -> list[dict[str, Any]]:
    """List artifacts for a workflow run across all pages."""
    try:
        run_id_int = int(run_id)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"run_id must be an integer, got {run_id!r}") from exc
    if run_id_int <= 0:
        raise ValueError(f"run_id must be positive, got {run_id_int}")

    if _should_use_http_api():
        docs = _http_paginated_docs(
            f"repos/{repo}/actions/runs/{run_id_int}/artifacts",
            {
                "per_page": "100",
            },
        )
    else:
        result = gh(
            "api",
            "--method",
            "GET",
            "--paginate",
            f"repos/{repo}/actions/runs/{run_id_int}/artifacts",
            "-F",
            "per_page=100",
        )
        docs = parse_json_documents(result.stdout)

    artifacts: list[dict[str, Any]] = []
    for doc in docs:
        items = doc.get("artifacts", [])
        if isinstance(items, list):
            artifacts.extend(item for item in items if isinstance(item, dict))
    return artifacts


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

    Args:
        requested: "auto", "true", or "false"

    Returns:
        "true" or "false"
    """
    if requested != "auto":
        return requested
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
