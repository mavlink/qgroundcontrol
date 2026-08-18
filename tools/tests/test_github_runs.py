"""Tests for cached GitHub workflow-run handling."""

from __future__ import annotations

import argparse
import json
from typing import TYPE_CHECKING

import pytest
from common.github_runs import (
    WorkflowRunsFileError,
    add_workflow_run_query_args,
    load_workflow_runs,
    resolve_workflow_runs,
)

if TYPE_CHECKING:
    from pathlib import Path


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
        runs_option="--runs-file",
        include_runs_cache=True,
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
            "--runs-file",
            "runs.json",
            "--runs-cache",
            "cache.json",
        ]
    )
    assert args.repo == "owner/repo"
    assert args.head_sha == "abc"
    assert args.event == "push"
    assert args.workflows == "Linux,Windows"
    assert args.runs_file == "runs.json"
    assert args.runs_cache == "cache.json"
