"""Contract tests for workflow artifact downloads."""

from __future__ import annotations

import json
from pathlib import Path
from unittest.mock import patch

import download_artifacts as mod
from _helpers import completed, workflow_run


def _main_args(tmp_path: Path, *extra: str) -> list[str]:
    return [
        "--repo",
        "owner/repo",
        "--head-sha",
        "abc",
        "--output-dir",
        str(tmp_path),
        "--workflows",
        "Linux",
        "--artifact-prefixes",
        "coverage-report",
        *extra,
    ]


def test_workflow_run_selection_filters_and_keeps_latest() -> None:
    runs = [
        workflow_run("Linux", 10),
        workflow_run("Linux", 11, created_at="2026-02-25T00:00:00Z"),
        workflow_run("Windows", 20),
        workflow_run("Windows", 21, conclusion="failure"),
        workflow_run("Other", 30, event="push"),
    ]
    selected = mod.select_latest_successful_runs(runs, ["Linux", "Windows"], event="pull_request")
    assert {run["name"]: run["id"] for run in selected} == {"Linux": 11, "Windows": 20}


def test_download_command_passes_selected_names_and_reports_failure() -> None:
    with patch.object(mod, "gh", return_value=completed()) as gh:
        assert mod.download_run_artifacts(
            123,
            "owner/repo",
            Path("artifacts"),
            artifact_names=["coverage-report", "test-results-linux"],
        )
    assert gh.call_args.args[:3] == ("run", "download", "123")
    assert "coverage-report" in gh.call_args.args
    assert "test-results-linux" in gh.call_args.args

    with patch.object(mod, "gh", return_value=completed(stderr="bad", returncode=1)):
        assert not mod.download_run_artifacts(123, "owner/repo", Path("artifacts"))


def test_artifact_filters_match_prefixes_and_product_extensions(tmp_path: Path) -> None:
    artifacts = [
        {"name": "coverage-report"},
        {"name": "test-results-linux"},
        {"name": "QGroundControl-x86_64.AppImage"},
    ]
    assert mod.select_artifact_names(artifacts, ["coverage-report", "test-results-"]) == [
        "coverage-report",
        "test-results-linux",
    ]

    (tmp_path / "app.apk").write_bytes(b"x")
    (tmp_path / "notes.txt").write_bytes(b"x")
    nested = tmp_path / "nested"
    nested.mkdir()
    (nested / "app.AppImage").write_bytes(b"x")
    assert sorted(path.name for path in mod.list_downloaded_artifacts(tmp_path)) == [
        "app.AppImage",
        "app.apk",
    ]


def test_main_reports_empty_and_failed_download_states(tmp_path: Path) -> None:
    for runs, artifacts, downloads, expected in (
        ([], [], True, 0),
        ([workflow_run("Linux", 42)], [{"name": "coverage-report"}], False, 1),
        ([workflow_run("Linux", 42)], [{"name": "unrelated"}], True, 2),
    ):
        with (
            patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
            patch.object(mod, "select_latest_successful_runs", return_value=runs),
            patch.object(mod, "list_run_artifacts", return_value=artifacts),
            patch.object(mod, "download_run_artifacts", return_value=downloads),
            patch.object(mod, "list_downloaded_files", return_value=[]),
            patch.object(mod, "list_downloaded_artifacts", return_value=[]),
        ):
            assert mod.main(_main_args(tmp_path)) == expected


def test_main_fails_when_only_some_workflow_downloads_succeed(tmp_path: Path) -> None:
    runs = [workflow_run("Linux", 41), workflow_run("Windows", 42)]
    downloaded = tmp_path / "QGroundControl.AppImage"
    downloaded.write_text("partial download")
    outcomes = iter([True, False])

    def download(*_args, **_kwargs) -> bool:
        return next(outcomes)

    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "select_latest_successful_runs", return_value=runs),
        patch.object(mod, "download_run_artifacts", side_effect=download),
        patch.object(mod, "list_downloaded_artifacts", return_value=[downloaded]),
    ):
        args = _main_args(tmp_path)
        args[args.index("Linux")] = "Linux,Windows"
        args[args.index("coverage-report") - 1 :] = []
        assert mod.main(args) == 1


def test_main_falls_back_to_older_run_with_matching_artifacts(tmp_path: Path) -> None:
    runs = [
        workflow_run("Linux", 101, created_at="2026-02-25T10:00:00Z"),
        workflow_run("Linux", 100, created_at="2026-02-25T09:00:00Z"),
    ]

    def artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        del repo
        return [{"name": "unrelated" if run_id == 101 else "coverage-report", "size_in_bytes": 1}]

    downloaded = tmp_path / "coverage.xml"
    downloaded.write_text("<xml/>")

    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", side_effect=artifacts),
        patch.object(mod, "download_run_artifacts", return_value=True) as download,
        patch.object(mod, "list_downloaded_files", return_value=[downloaded]),
    ):
        assert mod.main(_main_args(tmp_path)) == 0
    assert download.call_args.args[0] == 100


def test_main_writes_selected_artifact_metadata(tmp_path: Path) -> None:
    metadata = tmp_path / "metadata.json"
    downloaded = tmp_path / "dummy.txt"
    downloaded.write_text("x")
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=[workflow_run("Linux", 42)]),
        patch.object(
            mod, "select_latest_successful_runs", return_value=[workflow_run("Linux", 42)]
        ),
        patch.object(
            mod,
            "list_run_artifacts",
            return_value=[
                {"name": "coverage-report", "size_in_bytes": 100},
                {"name": "QGroundControl.AppImage", "size_in_bytes": 200},
            ],
        ),
        patch.object(mod, "download_run_artifacts", return_value=True),
        patch.object(mod, "list_downloaded_files", return_value=[downloaded]),
    ):
        assert mod.main(_main_args(tmp_path, "--artifact-metadata-out", str(metadata))) == 0
    assert json.loads(metadata.read_text()) == {
        "runs": {
            "42": [
                {"name": "coverage-report", "size_in_bytes": 100},
                {"name": "QGroundControl.AppImage", "size_in_bytes": 200},
            ]
        }
    }


def test_main_rejects_invalid_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    for payload in ("{invalid", json.dumps({"runs": []})):
        runs_file.write_text(payload)
        assert mod.main(_main_args(tmp_path, "--runs-file", str(runs_file))) == 1
