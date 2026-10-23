"""Tests for cached GitHub workflow-run handling."""

from __future__ import annotations

import argparse
import json
from typing import TYPE_CHECKING

import pytest
from ci.reporting.workflow_runs import (
    WorkflowRunsFileError,
    add_workflow_run_query_args,
    configured_platform_workflows,
    default_runs_cache,
    load_workflow_runs,
    resolve_workflow_runs,
    triggering_run,
)

from .._helpers import REPO_ROOT

if TYPE_CHECKING:
    from collections.abc import Callable
    from pathlib import Path

CONFIGURED_PLATFORMS = json.loads(
    (REPO_ROOT / ".github" / "build-config.json").read_text(encoding="utf-8")
)["build"]["platform_workflows"]


def test_load_workflow_runs_validates_json_list(tmp_path: Path) -> None:
    path = tmp_path / "runs.json"
    runs = [{"id": 1, "name": "Linux"}, {"id": 2, "name": "Windows"}]
    path.write_text(json.dumps(runs), encoding="utf-8")
    assert load_workflow_runs(path) == runs

    for payload in ({"id": 1}, ["invalid"]):
        path.write_text(json.dumps(payload), encoding="utf-8")
        with pytest.raises(WorkflowRunsFileError):
            load_workflow_runs(path)


def test_load_workflow_runs_wraps_file_and_json_errors(tmp_path: Path) -> None:
    with pytest.raises(WorkflowRunsFileError, match="failed to read runs file"):
        load_workflow_runs(tmp_path / "missing.json")

    path = tmp_path / "runs.json"
    path.write_text("{invalid", encoding="utf-8")
    with pytest.raises(WorkflowRunsFileError, match="failed to read runs file"):
        load_workflow_runs(path)


def test_resolve_workflow_runs_uses_cache_or_fetcher(tmp_path: Path, capsys) -> None:
    path = tmp_path / "runs.json"
    path.write_text('[{"id": 2}]', encoding="utf-8")
    calls: list[tuple[str, str]] = []

    def fetcher(repo: str, sha: str) -> list[dict[str, int]]:
        calls.append((repo, sha))
        return [{"id": 3}]

    assert resolve_workflow_runs("owner/repo", "abc", str(path), fetcher) == [{"id": 2}]
    assert calls == []
    assert resolve_workflow_runs("owner/repo", "abc", "", fetcher) == [{"id": 3}]
    assert calls == [("owner/repo", "abc")]
    # The default snapshot path exists only once collect_build_status.py has run.
    assert resolve_workflow_runs("owner/repo", "abc", str(tmp_path / "none.json"), fetcher) == [
        {"id": 3}
    ]

    path.write_text("{invalid", encoding="utf-8")
    assert resolve_workflow_runs("owner/repo", "abc", str(path), fetcher) is None
    assert "failed to read runs file" in capsys.readouterr().err


def test_add_workflow_run_query_args_supports_shared_variants() -> None:
    parser = argparse.ArgumentParser()
    add_workflow_run_query_args(
        parser,
        default_event="pull_request",
        workflows_option="--workflows",
        workflows_dest="workflows",
        restrict_event=True,
    )
    args = parser.parse_args(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc",
            "--event",
            "push",
            "--workflows",
            "Linux,Windows",
            "--runs-cache",
            "runs.json",
        ]
    )
    assert args.repo == "owner/repo"
    assert args.head_sha == "abc"
    assert args.event == "push"
    assert args.workflows == "Linux,Windows"
    assert args.runs_cache == "runs.json"


def _query_parser(default_event: str = "") -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    add_workflow_run_query_args(parser, default_event=default_event)
    return parser


@pytest.mark.parametrize("event", ["pull_request", "push"])
def test_query_args_default_to_the_triggering_run(
    event: str,
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    workflow_run_event: Callable[..., Path],
) -> None:
    workflow_run_event(head_sha="abc", event=event, name="iOS")
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))

    args = _query_parser("pull_request").parse_args([])

    assert (args.repo, args.head_sha, args.event) == ("owner/repo", "abc", event)
    assert args.platform_workflows == CONFIGURED_PLATFORMS
    assert args.runs_cache == str(tmp_path / "workflow-runs-cache.json")
    explicit = _query_parser().parse_args(
        ["--repo", "o/r", "--head-sha", "def", "--event", "", "--platform-workflows", "Linux"]
    )
    assert (explicit.repo, explicit.head_sha, explicit.event) == ("o/r", "def", "")
    assert explicit.platform_workflows == "Linux"


def test_query_args_outside_a_workflow_run(
    capsys: pytest.CaptureFixture[str], workflow_run_event: Callable[..., Path]
) -> None:
    with pytest.raises(SystemExit):
        _query_parser().parse_args([])
    assert "--repo, --head-sha" in capsys.readouterr().err

    args = _query_parser("pull_request").parse_args(["--repo", "o/r", "--head-sha", "abc"])
    assert (args.event, args.runs_cache) == ("pull_request", "")

    # A workflow_run payload without an event keeps the script's default.
    workflow_run_event(head_sha="abc")
    assert _query_parser("pull_request").parse_args(["--repo", "o/r"]).event == "pull_request"


@pytest.mark.parametrize(
    "config",
    [
        None,
        "{invalid",
        "[]",
        json.dumps({"build": {}}),
        json.dumps({"build": {"platform_workflows": " "}}),
    ],
)
def test_platform_workflows_fail_loudly_without_build_config(
    config: str | None,
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    path = tmp_path / "build-config.json"
    if config is not None:
        path.write_text(config, encoding="utf-8")
    monkeypatch.setenv("CONFIG_FILE", str(path))

    with pytest.raises(ValueError, match=r"build\.platform_workflows"):
        configured_platform_workflows()
    argv = ["--repo", "o/r", "--head-sha", "abc"]
    with pytest.raises(SystemExit) as exc:
        _query_parser().parse_args(argv)
    assert exc.value.code == 2
    assert "argument --platform-workflows: " in capsys.readouterr().err
    # An explicit list needs no build config.
    assert _query_parser().parse_args(
        [*argv, "--platform-workflows", "Linux"]
    ).platform_workflows == ("Linux")


def test_configured_platform_workflows_reads_build_config() -> None:
    assert configured_platform_workflows() == CONFIGURED_PLATFORMS
    assert "iOS" in CONFIGURED_PLATFORMS.split(",")


def test_triggering_run_reads_the_event_payload(workflow_run_event: Callable[..., Path]) -> None:
    assert triggering_run() == {}
    path = workflow_run_event(head_sha="abc", name="Linux")
    assert triggering_run() == {"head_sha": "abc", "name": "Linux"}
    assert triggering_run({"GITHUB_EVENT_PATH": str(path)})["name"] == "Linux"
    assert triggering_run({}) == {}

    # Other events (push, pull_request, schedule, ...) carry no workflow_run.
    path.write_text(json.dumps({"pull_request": {"number": 1}}), encoding="utf-8")
    assert triggering_run() == {}
    path.write_text(json.dumps({"workflow_run": None}), encoding="utf-8")
    assert triggering_run() == {}


def test_default_runs_cache_lives_in_runner_temp(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    assert default_runs_cache() == ""
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    assert default_runs_cache() == str(tmp_path / "workflow-runs-cache.json")


def test_cached_run_rejects_invalid_fields_before_selection(tmp_path: Path) -> None:
    path = tmp_path / "runs.json"
    path.write_text(json.dumps([{"name": "Linux", "status": ["completed"]}]), encoding="utf-8")
    with pytest.raises(WorkflowRunsFileError, match=r"status.*string"):
        load_workflow_runs(path)


def test_run_order_uses_instants_across_timezones() -> None:
    from ci.reporting.workflow_runs import group_runs_by_name

    runs = [
        {"name": "Linux", "id": 1, "created_at": "2026-01-01T02:00:00+03:00"},
        {"name": "Linux", "id": 2, "created_at": "2026-01-01T00:00:00Z"},
    ]
    assert [run["id"] for run in group_runs_by_name(runs, ["Linux"])["Linux"]] == [2, 1]
