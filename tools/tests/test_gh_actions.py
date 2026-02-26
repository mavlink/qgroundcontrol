#!/usr/bin/env python3
"""Tests for tools/common/gh_actions.py."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

TOOLS_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(TOOLS_DIR))

from common import gh_actions as mod


def _cp(stdout: str = "", stderr: str = "", returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(args=["gh"], returncode=returncode, stdout=stdout, stderr=stderr)


class _FakeHttpResponse:
    def __init__(self, payload: dict[str, object], link: str = "") -> None:
        self._payload = payload
        self.headers = {"Link": link}

    def read(self) -> bytes:
        return json.dumps(self._payload).encode("utf-8")

    def __enter__(self) -> _FakeHttpResponse:
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        return None


def test_parse_json_documents_handles_paginated_stream() -> None:
    payload = (
        json.dumps({"workflow_runs": [{"id": 1}]})
        + "\n"
        + json.dumps({"workflow_runs": [{"id": 2}]})
    )
    docs = mod.parse_json_documents(payload)
    assert [doc["workflow_runs"][0]["id"] for doc in docs] == [1, 2]


def test_parse_json_documents_raises_on_invalid_json() -> None:
    payload = '{"workflow_runs":[{"id":1}]}\nnot-json'
    try:
        mod.parse_json_documents(payload)
    except ValueError as exc:
        assert "Failed to parse GitHub API JSON output near:" in str(exc)
    else:
        raise AssertionError("Expected ValueError for invalid JSON stream")


def test_list_workflow_runs_for_sha_uses_get_method() -> None:
    payload = json.dumps({"workflow_runs": [{"id": 1, "name": "Linux"}]})
    with patch.dict(os.environ, {"QGC_GH_API_MODE": "gh"}, clear=False):
        with patch.object(mod, "gh", return_value=_cp(stdout=payload)) as gh_mock:
            runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert runs == [{"id": 1, "name": "Linux"}]
    called_args = gh_mock.call_args[0]
    assert "--method" in called_args
    assert "GET" in called_args


def test_list_workflow_runs_for_sha_http_mode() -> None:
    first = _FakeHttpResponse(
        {"workflow_runs": [{"id": 1, "name": "Linux"}]},
        '<https://api.github.com/next>; rel="next"',
    )
    second = _FakeHttpResponse({"workflow_runs": [{"id": 2, "name": "Windows"}]})
    with patch.dict(os.environ, {"QGC_GH_API_MODE": "http", "GH_TOKEN": "token"}, clear=False):
        with patch.object(mod.urllib_request, "urlopen", side_effect=[first, second]), patch.object(mod, "gh") as gh_mock:
            runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert [run["id"] for run in runs] == [1, 2]
    gh_mock.assert_not_called()


def test_list_run_artifacts_parses_paginated_stream() -> None:
    payload = (
        json.dumps({"artifacts": [{"name": "QGroundControl", "size_in_bytes": 1}]})
        + "\n"
        + json.dumps({"artifacts": [{"name": "QGroundControl2", "size_in_bytes": 2}]})
    )
    with patch.object(mod, "gh", return_value=_cp(stdout=payload)) as gh_mock:
        artifacts = mod.list_run_artifacts("owner/repo", 77)

    assert [a["name"] for a in artifacts] == ["QGroundControl", "QGroundControl2"]
    gh_mock.assert_called_once()


def test_list_run_artifacts_rejects_invalid_run_id() -> None:
    try:
        mod.list_run_artifacts("owner/repo", "not-an-int")
    except ValueError as exc:
        assert "run_id must be an integer" in str(exc)
    else:
        raise AssertionError("Expected ValueError for invalid run_id")
