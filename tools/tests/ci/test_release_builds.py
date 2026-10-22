"""Release dispatch identity must not silently select another build at the same SHA."""

import json
from unittest.mock import patch

import pytest
from ci.publish.release_builds import WORKFLOWS, identify_runs, wait_for_builds


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
    runs = [run(name, created_at="2099-01-01T00:00:00Z") for name in WORKFLOWS]

    def gh_api_json(path):
        return runs[int(path.split("/")[-1]) - 1]

    with (
        patch("ci.publish.release_builds.gh") as gh,
        patch("ci.publish.release_builds.gh_api_json", side_effect=gh_api_json),
        patch("ci.publish.release_builds.list_workflow_runs_for_sha", return_value=runs),
    ):
        wait_for_builds("o/r", "abc", "v1", tmp_path / "runs.json")
    assert gh.call_count == len(WORKFLOWS)
    assert json.loads((tmp_path / "runs.json").read_text()) == runs


def test_failed_dispatch_does_not_create_snapshot(tmp_path):
    failed = run(conclusion="failure", html_url="https://example.test/run")
    with (
        patch("ci.publish.release_builds.identify_runs", return_value={"Linux": failed}),
        patch("ci.publish.release_builds.list_workflow_runs_for_sha", return_value=[]),
        patch("ci.publish.release_builds.gh"),
        patch("ci.publish.release_builds.gh_api_json", return_value=failed),
        pytest.raises(RuntimeError, match="Release build failed"),
    ):
        wait_for_builds("o/r", "abc", "v1", tmp_path / "runs.json")
    assert not (tmp_path / "runs.json").exists()
