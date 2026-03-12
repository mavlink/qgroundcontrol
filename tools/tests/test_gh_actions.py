#!/usr/bin/env python3
"""Tests for tools/common/gh_actions.py."""

from __future__ import annotations

import json
import os
import subprocess
from unittest.mock import patch

import httpx

from common import gh_actions as mod


def _cp(stdout: str = "", stderr: str = "", returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(args=["gh"], returncode=returncode, stdout=stdout, stderr=stderr)


def _mock_response(
    data: dict,
    next_url: str = "",
    status_code: int = 200,
    extra_headers: dict[str, str] | None = None,
) -> httpx.Response:
    headers = {}
    if next_url:
        headers["link"] = f'<{next_url}>; rel="next"'
    if extra_headers:
        headers.update(extra_headers)
    request = httpx.Request("GET", "https://api.github.com/test")
    resp = httpx.Response(status_code, json=data, headers=headers, request=request)
    return resp


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
    first = _mock_response(
        {"workflow_runs": [{"id": 1, "name": "Linux"}]},
        next_url="https://api.github.com/next",
    )
    second = _mock_response({"workflow_runs": [{"id": 2, "name": "Windows"}]})

    with patch.dict(os.environ, {"QGC_GH_API_MODE": "http", "GH_TOKEN": "token"}, clear=False):
        with patch.object(mod, "_build_http_client") as mock_client:
            client = mock_client.return_value.__enter__.return_value
            client.get.side_effect = [first, second]
            with patch.object(mod, "gh") as gh_mock:
                runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert [run["id"] for run in runs] == [1, 2]
    gh_mock.assert_not_called()


def test_list_workflow_runs_for_sha_http_mode_retries_retryable_status() -> None:
    first = _mock_response(
        {"message": "try again"},
        status_code=503,
        extra_headers={"Retry-After": "0"},
    )
    second = _mock_response({"workflow_runs": [{"id": 7, "name": "Linux"}]})

    with patch.dict(os.environ, {"QGC_GH_API_MODE": "http", "GH_TOKEN": "token"}, clear=False):
        with patch.object(mod, "_build_http_client") as mock_client:
            client = mock_client.return_value.__enter__.return_value
            client.get.side_effect = [first, second]
            with patch.object(mod.time, "sleep") as sleep_mock:
                runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert runs == [{"id": 7, "name": "Linux"}]
    assert client.get.call_count == 2
    sleep_mock.assert_called_once_with(1.0)


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


class TestIsForkPr:
    def test_not_pr_event(self) -> None:
        with patch.dict(os.environ, {"EVENT_NAME": "push"}, clear=False):
            assert mod.is_fork_pr() is False

    def test_same_repo_pr(self) -> None:
        env = {"EVENT_NAME": "pull_request", "PR_REPO": "owner/repo", "THIS_REPO": "owner/repo"}
        with patch.dict(os.environ, env, clear=False):
            assert mod.is_fork_pr() is False

    def test_fork_pr(self) -> None:
        env = {"EVENT_NAME": "pull_request", "PR_REPO": "fork/repo", "THIS_REPO": "owner/repo"}
        with patch.dict(os.environ, env, clear=False):
            assert mod.is_fork_pr() is True

    def test_empty_pr_repo(self) -> None:
        env = {"EVENT_NAME": "pull_request", "PR_REPO": "", "THIS_REPO": "owner/repo"}
        with patch.dict(os.environ, env, clear=False):
            assert mod.is_fork_pr() is False


class TestResolveCachePolicy:
    def test_explicit_true(self) -> None:
        assert mod.resolve_cache_policy("true") == "true"

    def test_explicit_false(self) -> None:
        assert mod.resolve_cache_policy("false") == "false"

    def test_auto_non_pr(self) -> None:
        with patch.dict(os.environ, {"EVENT_NAME": "push"}, clear=False):
            assert mod.resolve_cache_policy("auto") == "true"

    def test_auto_same_repo_pr(self) -> None:
        env = {"EVENT_NAME": "pull_request", "PR_REPO": "owner/repo", "THIS_REPO": "owner/repo"}
        with patch.dict(os.environ, env, clear=False):
            assert mod.resolve_cache_policy("auto") == "true"

    def test_auto_fork_pr(self) -> None:
        env = {"EVENT_NAME": "pull_request", "PR_REPO": "fork/repo", "THIS_REPO": "owner/repo"}
        with patch.dict(os.environ, env, clear=False):
            assert mod.resolve_cache_policy("auto") == "false"


class TestWriteGithubOutput:
    def test_simple_values(self, tmp_path) -> None:
        out = tmp_path / "output"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}, clear=False):
            mod.write_github_output({"key1": "val1", "key2": "val2"})
        content = out.read_text()
        assert "key1=val1\n" in content
        assert "key2=val2\n" in content

    def test_multiline_value(self, tmp_path) -> None:
        out = tmp_path / "output"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}, clear=False):
            mod.write_github_output({"body": "line1\nline2"})
        content = out.read_text()
        assert "body<<EOF_body_" in content
        assert "line1\nline2\n" in content

    def test_noop_without_env(self) -> None:
        with patch.dict(os.environ, {}, clear=True):
            mod.write_github_output({"key": "val"})


class TestWriteStepSummary:
    def test_writes_markdown(self, tmp_path) -> None:
        out = tmp_path / "summary"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(out)}, clear=False):
            mod.write_step_summary("## Hello\n")
        assert out.read_text() == "## Hello\n"

    def test_noop_without_env(self) -> None:
        with patch.dict(os.environ, {}, clear=True):
            mod.write_step_summary("test")


class TestAppendGithubEnv:
    def test_writes_env_vars(self, tmp_path) -> None:
        out = tmp_path / "env"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_ENV": str(out)}, clear=False):
            mod.append_github_env({"FOO": "bar", "BAZ": "qux"})
        content = out.read_text()
        assert "FOO=bar\n" in content
        assert "BAZ=qux\n" in content

    def test_noop_without_env(self) -> None:
        with patch.dict(os.environ, {}, clear=True):
            mod.append_github_env({"FOO": "bar"})
