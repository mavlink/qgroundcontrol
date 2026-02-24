#!/usr/bin/env python3
"""Shared helpers for GitHub Actions API access via gh CLI."""

from __future__ import annotations

import json
import os
import subprocess
import time
from typing import Any
from urllib import error as urllib_error
from urllib import parse as urllib_parse
from urllib import request as urllib_request


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


def _extract_next_link(link_header: str) -> str:
    for part in link_header.split(","):
        section = part.strip()
        if 'rel="next"' not in section:
            continue
        start = section.find("<")
        end = section.find(">", start + 1)
        if start != -1 and end != -1:
            return section[start + 1:end]
    return ""


def _http_get_json(url: str, headers: dict[str, str], retries: int = 3) -> tuple[dict[str, Any], str]:
    for attempt in range(1, retries + 1):
        req = urllib_request.Request(url, headers=headers, method="GET")
        try:
            with urllib_request.urlopen(req, timeout=30) as resp:
                payload = resp.read().decode("utf-8")
                doc = json.loads(payload)
                if not isinstance(doc, dict):
                    raise ValueError(f"Unexpected non-object response from GitHub API at {url}")
                next_url = _extract_next_link(resp.headers.get("Link", ""))
                return doc, next_url
        except urllib_error.HTTPError as exc:
            if attempt < retries and exc.code in {429, 500, 502, 503, 504}:
                time.sleep(attempt)
                continue
            raise RuntimeError(f"GitHub API request failed ({exc.code}) for {url}: {exc.reason}") from exc
        except (urllib_error.URLError, TimeoutError, json.JSONDecodeError, ValueError) as exc:
            if attempt < retries:
                time.sleep(attempt)
                continue
            raise RuntimeError(f"GitHub API request failed for {url}: {exc}") from exc

    raise RuntimeError(f"GitHub API request failed for {url}: exhausted retries")


def _http_paginated_docs(path: str, params: dict[str, str]) -> list[dict[str, Any]]:
    base_url = os.environ.get("GITHUB_API_URL", "https://api.github.com").rstrip("/")
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "qgc-ci-gh-actions/1.0",
        "X-GitHub-Api-Version": "2022-11-28",
        "Authorization": f"Bearer {_github_token()}",
    }

    query = urllib_parse.urlencode(params)
    url = f"{base_url}/{path.lstrip('/')}"
    if query:
        url = f"{url}?{query}"

    docs: list[dict[str, Any]] = []
    while url:
        doc, next_url = _http_get_json(url, headers=headers)
        docs.append(doc)
        url = next_url
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
