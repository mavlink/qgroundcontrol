"""Shared helpers for CI script tests."""

from __future__ import annotations

import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]


def completed(
    stdout: str = "", returncode: int = 0, stderr: str = ""
) -> subprocess.CompletedProcess:
    """Fake subprocess.CompletedProcess for stubbing gh/git calls."""
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr=stderr)


def workflow_run(
    name: str,
    run_id: int = 1,
    *,
    created_at: str = "2026-02-24T00:00:00Z",
    status: str = "completed",
    conclusion: str = "success",
    event: str = "pull_request",
    html_url: str = "https://example.test/run",
) -> dict[str, object]:
    """Build the canonical workflow-run fixture shared by CI script tests."""
    return {
        "name": name,
        "id": run_id,
        "created_at": created_at,
        "status": status,
        "conclusion": conclusion,
        "event": event,
        "html_url": html_url,
    }
