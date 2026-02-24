#!/usr/bin/env python3
"""Helpers for querying GitHub Actions workflow runs via gh CLI."""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

# Ensure repository root is importable when scripts are executed by path.
_REPO_ROOT = Path(__file__).resolve().parents[2]
_COMMON_DIR = _REPO_ROOT / "tools" / "common"
if str(_COMMON_DIR) not in sys.path:
    sys.path.insert(0, str(_COMMON_DIR))

import gh_actions as _gh_actions

_gh = _gh_actions.gh
parse_json_documents = _gh_actions.parse_json_documents
list_run_artifacts = _gh_actions.list_run_artifacts
_list_workflow_runs_for_sha = _gh_actions.list_workflow_runs_for_sha


def parse_csv_list(value: str) -> list[str]:
    """Parse comma-separated values into a trimmed non-empty list."""
    return [item.strip() for item in value.split(",") if item.strip()]


def list_workflow_runs(repo: str, head_sha: str) -> list[dict[str, Any]]:
    """List workflow runs for a commit SHA across all pages."""
    return _list_workflow_runs_for_sha(repo, head_sha)
