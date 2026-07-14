"""Contracts for shared GitHub Actions helpers."""

from __future__ import annotations

import argparse
import json
from typing import TYPE_CHECKING
from unittest.mock import patch

import common.gh_actions as mod
import pytest
from common.github_runs import (
    WorkflowRunsFileError,
    add_workflow_run_query_args,
    load_workflow_runs,
    resolve_workflow_runs,
    select_latest_runs_by_name,
)

from ._helpers import completed

if TYPE_CHECKING:
    from pathlib import Path


def test_paginated_api_helpers_parse_ndjson_and_validate_run_ids() -> None:
    payloads = [
        (mod.list_workflow_runs_for_sha, ("owner/repo", "abc"), "workflow_runs", [1, 2]),
        (mod.list_run_artifacts, ("owner/repo", 77), "artifacts", ["one", "two"]),
    ]
    for function, args, item_key, values in payloads:
        key = "id" if item_key == "workflow_runs" else "name"
        output = "\n".join(json.dumps({key: value}) for value in values)
        with patch.object(mod, "gh", return_value=completed(stdout=output)) as gh:
            assert [item[key] for item in function(*args)] == values
        assert tuple(gh.call_args.args[index] for index in (1, 2, 3)) == (
            "--method",
            "GET",
            "--paginate",
        )
        assert f".{item_key}[]?" in gh.call_args.args

    for invalid in ("bad", 0, -1):
        with pytest.raises(ValueError, match="run_id"):
            mod.list_run_artifacts("owner/repo", invalid)


def test_repository_and_cache_policy_follow_event_environment(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    keys = ("GH_REPO", "GITHUB_REPOSITORY", "EVENT_NAME", "PR_REPO", "THIS_REPO")

    def set_environment(values: dict[str, str]) -> None:
        for key in keys:
            monkeypatch.delenv(key, raising=False)
        for key, value in values.items():
            monkeypatch.setenv(key, value)

    for environment, expected in (
        ({"GH_REPO": "explicit/repo", "GITHUB_REPOSITORY": "actions/repo"}, "explicit/repo"),
        ({"GITHUB_REPOSITORY": "actions/repo"}, "actions/repo"),
    ):
        set_environment(environment)
        assert mod.require_repository() == expected

    set_environment({})
    with pytest.raises(SystemExit):
        mod.require_repository()

    for environment, fork, cache in (
        ({"EVENT_NAME": "push"}, False, "true"),
        (
            {"EVENT_NAME": "pull_request", "PR_REPO": "owner/repo", "THIS_REPO": "owner/repo"},
            False,
            "false",
        ),
        (
            {"EVENT_NAME": "pull_request", "PR_REPO": "fork/repo", "THIS_REPO": "owner/repo"},
            True,
            "false",
        ),
        ({"EVENT_NAME": "pull_request_target"}, False, "false"),
        ({"EVENT_NAME": "schedule"}, False, "true"),
    ):
        set_environment(environment)
        assert mod.is_fork_pr() is fork
        assert mod.resolve_cache_policy("auto") == cache
        assert (mod.resolve_cache_policy("true"), mod.resolve_cache_policy("false")) == (
            "true",
            "false",
        )


def test_github_output_supports_simple_and_collision_safe_multiline_values(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    multiline = "line1\nEOF_body_deadbeef\ntail"

    mod.write_github_output({"key": "value", "body": multiline})

    content = output.read_text()
    assert "key=value\n" in content
    assert "body<<EOF_body_" in content and "EOF_body\n" not in content
    assert multiline in content


def test_summary_environment_and_noop_writers(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    summary, environment = tmp_path / "summary", tmp_path / "env"
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    monkeypatch.setenv("GITHUB_ENV", str(environment))
    mod.write_step_summary("## Hello\n")
    mod.append_github_env({"FOO": "bar", "BAZ": "qux"})
    assert summary.read_text() == "## Hello\n"
    assert set(environment.read_text().splitlines()) == {"FOO=bar", "BAZ=qux"}

    for key in ("GITHUB_OUTPUT", "GITHUB_STEP_SUMMARY", "GITHUB_ENV"):
        monkeypatch.delenv(key, raising=False)
    mod.write_github_output({"key": "value"})
    mod.write_step_summary("ignored")
    mod.append_github_env({"FOO": "ignored"})


def test_annotations_escape_workflow_commands(capsys: pytest.CaptureFixture[str]) -> None:
    mod.gh_error("a\nb\rc%d")
    mod.gh_warning("careful")
    mod.gh_notice("fyi")
    captured = capsys.readouterr()
    assert captured.out == "::error::a%0Ab%0Dc%25d\n::warning::careful\n::notice::fyi\n"
    assert captured.err == ""


def test_workflow_run_arguments_share_platform_and_cache_options() -> None:
    parser = argparse.ArgumentParser()
    add_workflow_run_query_args(parser, default_event="push", include_runs_cache=True)
    args = parser.parse_args(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--runs-input",
            "runs.json",
            "--runs-cache",
            "cache.json",
        ]
    )
    assert args.platform_workflows == "Linux,Windows,MacOS,Android,iOS"
    assert (args.event, args.runs_file, args.runs_cache) == ("push", "runs.json", "cache.json")


def test_workflow_run_selection_filters_and_uses_real_timestamp_order() -> None:
    def run(name: str, run_id: int, created_at: str, **overrides: str) -> dict[str, object]:
        return {
            "name": name,
            "id": run_id,
            "created_at": created_at,
            "status": overrides.get("status", "completed"),
            "conclusion": overrides.get("conclusion", "success"),
            "event": overrides.get("event", "pull_request"),
        }

    runs = [
        run("Linux", 1, "2026-02-24T01:30:00+01:00"),
        run("Linux", 2, "2026-02-24T01:00:00Z"),
        run("Windows", 3, "2026-02-24T00:00:00Z", conclusion="failure"),
        run("Windows", 4, "2026-02-24T00:00:00Z"),
        run("Windows", 5, "2026-02-24T02:00:00Z", event="push"),
    ]
    latest = select_latest_runs_by_name(
        runs,
        {"Linux", "Windows"},
        event="pull_request",
        status="completed",
        conclusion="success",
    )
    assert {name: item["id"] for name, item in latest.items()} == {"Linux": 2, "Windows": 4}


def test_workflow_run_cache_validation_and_resolution(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    path = tmp_path / "runs.json"
    for payload in ({"runs": []}, ["not-an-object"], "{invalid"):
        path.write_text(payload if isinstance(payload, str) else json.dumps(payload))
        with pytest.raises(WorkflowRunsFileError):
            load_workflow_runs(path)

    runs = [{"id": 1}, {"id": 2}]
    path.write_text(json.dumps(runs))
    assert load_workflow_runs(path) == runs

    calls: list[tuple[str, str]] = []

    def fetcher(repo: str, sha: str) -> list[dict[str, int]]:
        calls.append((repo, sha))
        return [{"id": 3}]

    assert resolve_workflow_runs("owner/repo", "abc", "", fetcher) == [{"id": 3}]
    assert calls == [("owner/repo", "abc")]

    path.write_text("{invalid")
    assert resolve_workflow_runs("owner/repo", "abc", str(path), fetcher) is None
    assert "failed to read runs file" in capsys.readouterr().err
