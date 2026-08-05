#!/usr/bin/env python3
"""Tests for tools/common/gh_actions.py."""

from __future__ import annotations

import json
import os
from unittest.mock import patch

from common import gh_actions as mod

from ._helpers import completed


def test_list_workflow_runs_for_sha_uses_jq_get_method() -> None:
    payload = json.dumps({"id": 1, "name": "Linux"})
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert runs == [{"id": 1, "name": "Linux"}]
    called_args = gh_mock.call_args[0]
    assert "--method" in called_args
    assert "GET" in called_args
    assert ".workflow_runs[]?" in called_args


def test_list_workflow_runs_for_sha_unpacks_ndjson_stream() -> None:
    payload = (
        json.dumps({"id": 1, "name": "Linux"}) + "\n" + json.dumps({"id": 2, "name": "Windows"})
    )
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert [run["id"] for run in runs] == [1, 2]
    gh_mock.assert_called_once()


def test_list_run_artifacts_parses_ndjson_stream() -> None:
    payload = (
        json.dumps({"name": "QGroundControl", "size_in_bytes": 1})
        + "\n"
        + json.dumps({"name": "QGroundControl2", "size_in_bytes": 2})
    )
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        artifacts = mod.list_run_artifacts("owner/repo", 77)

    assert [a["name"] for a in artifacts] == ["QGroundControl", "QGroundControl2"]
    called_args = gh_mock.call_args[0]
    assert ".artifacts[]?" in called_args
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
            assert mod.resolve_cache_policy("auto") == "false"

    def test_auto_fork_pr(self) -> None:
        env = {"EVENT_NAME": "pull_request", "PR_REPO": "fork/repo", "THIS_REPO": "owner/repo"}
        with patch.dict(os.environ, env, clear=False):
            assert mod.resolve_cache_policy("auto") == "false"

    def test_auto_pull_request_target(self) -> None:
        env = {
            "EVENT_NAME": "pull_request_target",
            "PR_REPO": "owner/repo",
            "THIS_REPO": "owner/repo",
        }
        with patch.dict(os.environ, env, clear=False):
            assert mod.resolve_cache_policy("auto") == "false"

    def test_auto_schedule(self) -> None:
        with patch.dict(os.environ, {"EVENT_NAME": "schedule"}, clear=False):
            assert mod.resolve_cache_policy("auto") == "true"

    def test_auto_workflow_dispatch(self) -> None:
        with patch.dict(os.environ, {"EVENT_NAME": "workflow_dispatch"}, clear=False):
            assert mod.resolve_cache_policy("auto") == "true"


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

    def test_multiline_delimiter_resists_collision(self, tmp_path) -> None:
        out = tmp_path / "output"
        out.touch()
        value = "line1\nEOF_body_deadbeef\ntail"
        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}, clear=False):
            mod.write_github_output({"body": value})
        content = out.read_text()
        assert "body<<EOF_body_" in content
        assert "EOF_body\n" not in content
        assert value in content

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


class TestAnnotations:
    def test_gh_error_format(self, capsys) -> None:
        mod.gh_error("boom")
        assert capsys.readouterr().out == "::error::boom\n"

    def test_gh_warning_format(self, capsys) -> None:
        mod.gh_warning("careful")
        assert capsys.readouterr().out == "::warning::careful\n"

    def test_gh_notice_format(self, capsys) -> None:
        mod.gh_notice("fyi")
        assert capsys.readouterr().out == "::notice::fyi\n"

    def test_escapes_newlines_and_percent(self, capsys) -> None:
        mod.gh_error("a\nb\rc%d")
        assert capsys.readouterr().out == "::error::a%0Ab%0Dc%25d\n"

    def test_emits_to_stdout_not_stderr(self, capsys) -> None:
        mod.gh_warning("x")
        captured = capsys.readouterr()
        assert captured.out == "::warning::x\n"
        assert captured.err == ""


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
