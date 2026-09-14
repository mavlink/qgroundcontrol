"""Release dispatch identity must not silently select another build at the same SHA."""

import json
from unittest.mock import patch

import pytest
from release_builds import WORKFLOWS, identify_runs, wait_for_builds


def run(name="Linux", **kwargs):
    return {
        "name": name,
        "id": list(WORKFLOWS).index(name) + 1,
        "event": "workflow_dispatch",
        "head_branch": "v1",
        "head_sha": "abc",
        "created_at": "2026-09-14T00:00:01Z",
        "status": "completed",
        "conclusion": "success",
        "run_attempt": 1,
        **kwargs,
    }


def test_selection_ignores_old_dispatches_and_other_refs():
    runs = [
        run(created_at="2026-09-13T00:00:00Z"),
        run(head_branch="master"),
        run(event="push"),
        run(),
    ]
    assert identify_runs(runs, "v1", "2026-09-14T00:00:00Z") == {"Linux": run()}


def test_ambiguous_dispatches_fail():
    with pytest.raises(RuntimeError, match="Ambiguous"):
        identify_runs([run(), run(id=2)], "v1", "2026-09-14T00:00:00Z")


def test_completed_dispatches_save_exact_ids(tmp_path):
    from subprocess import CompletedProcess

    runs = [run(name, created_at="2099-01-01T00:00:00Z") for name in WORKFLOWS]

    def gh(*args):
        if args[0] == "api":
            return CompletedProcess([], 0, json.dumps(runs[int(args[1].split("/")[-1]) - 1]))
        return CompletedProcess([], 0, "")

    with (
        patch("release_builds.gh", side_effect=gh),
        patch("release_builds.list_workflow_runs_for_sha", return_value=runs),
    ):
        wait_for_builds("o/r", "abc", "v1", tmp_path / "runs.json")
    assert json.loads((tmp_path / "runs.json").read_text()) == runs


def test_failed_dispatch_does_not_create_snapshot(tmp_path):
    from subprocess import CompletedProcess

    failed = run(conclusion="failure", html_url="https://example.test/run")
    with (
        patch("release_builds.identify_runs", return_value={"Linux": failed}),
        patch("release_builds.list_workflow_runs_for_sha", return_value=[]),
        patch("release_builds.gh", return_value=CompletedProcess([], 0, json.dumps(failed))),
        pytest.raises(RuntimeError, match="Release build failed"),
    ):
        wait_for_builds("o/r", "abc", "v1", tmp_path / "runs.json")
    assert not (tmp_path / "runs.json").exists()
