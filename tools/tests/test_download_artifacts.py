#!/usr/bin/env python3
"""Tests for tools/setup/download_artifacts.py."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

TOOLS_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(TOOLS_DIR))

from setup import download_artifacts as mod


def _cp(stdout: str = "", stderr: str = "", returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(args=["gh"], returncode=returncode, stdout=stdout, stderr=stderr)


def test_parse_args_defaults() -> None:
    args = mod.parse_args(["--repo", "owner/repo", "--head-sha", "abc123"])
    assert args.repo == "owner/repo"
    assert args.head_sha == "abc123"
    assert args.output_dir == Path("artifacts")
    assert args.workflows == "Linux,Windows,MacOS,Android"
    assert args.artifact_prefixes == ""
    assert args.artifact_metadata_out == ""


def test_get_workflow_runs_filters_by_name_status_and_conclusion() -> None:
    all_runs = [
        {"id": 1, "name": "Linux", "status": "completed", "conclusion": "success"},
        {"id": 2, "name": "Linux", "status": "in_progress", "conclusion": None},
        {"id": 3, "name": "Windows", "status": "completed", "conclusion": "failure"},
        {"id": 4, "name": "Other", "status": "completed", "conclusion": "success"},
    ]

    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=all_runs):
        runs = mod.get_workflow_runs("owner/repo", "abc", ["Linux", "Windows"])

    assert len(runs) == 1
    assert runs[0]["id"] == 1
    assert runs[0]["name"] == "Linux"


def test_get_workflow_runs_keeps_latest_successful_run_per_workflow() -> None:
    all_runs = [
        {
            "id": 10,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-01-01T00:00:00Z",
        },
        {
            "id": 11,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-01-02T00:00:00Z",
        },
        {
            "id": 20,
            "name": "Windows",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-01-01T00:00:00Z",
        },
    ]

    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=all_runs):
        runs = mod.get_workflow_runs("owner/repo", "abc", ["Linux", "Windows"])

    by_name = {run["name"]: run["id"] for run in runs}
    assert by_name["Linux"] == 11
    assert by_name["Windows"] == 20


def test_download_run_artifacts_failure_returns_false() -> None:
    with patch.object(mod, "gh", return_value=_cp(stderr="bad", returncode=1)):
        ok = mod.download_run_artifacts(123, "owner/repo", Path("artifacts"))
    assert ok is False


def test_download_run_artifacts_passes_selected_names() -> None:
    with patch.object(mod, "gh", return_value=_cp()) as mock_gh:
        ok = mod.download_run_artifacts(
            123,
            "owner/repo",
            Path("artifacts"),
            artifact_names=["coverage-report", "test-results-linux_gcc_64"],
        )
    assert ok is True
    args = mock_gh.call_args.args
    assert args[:3] == ("run", "download", "123")
    assert "-n" in args
    assert "coverage-report" in args
    assert "test-results-linux_gcc_64" in args


def test_select_artifact_names_for_run_filters_by_prefix() -> None:
    artifacts = [
        {"name": "coverage-report"},
        {"name": "test-results-linux_gcc_64"},
        {"name": "emulator-diagnostics-123"},
        {"name": "QGroundControl-x86_64.AppImage"},
    ]
    with patch.object(mod._gh_actions, "list_run_artifacts", return_value=artifacts):
        names = mod.select_artifact_names_for_run(
            "owner/repo",
            42,
            ["coverage-report", "test-results-"],
        )
    assert names == ["coverage-report", "test-results-linux_gcc_64"]


def test_list_downloaded_artifacts_filters_extensions(tmp_path: Path) -> None:
    (tmp_path / "a.apk").write_bytes(b"x")
    (tmp_path / "b.txt").write_bytes(b"x")
    nested = tmp_path / "nested"
    nested.mkdir()
    (nested / "c.AppImage").write_bytes(b"x")

    files = mod.list_downloaded_artifacts(tmp_path)
    names = sorted(p.name for p in files)
    assert names == ["a.apk", "c.AppImage"]


def test_main_returns_zero_when_no_runs_found() -> None:
    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=[]), patch.object(
        mod, "select_latest_successful_runs", return_value=[],
    ):
        rc = mod.main(["--repo", "owner/repo", "--head-sha", "abc123"])
    assert rc == 0


def test_main_returns_one_when_downloads_fail_and_no_files(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=runs), patch.object(
        mod, "select_latest_successful_runs", return_value=runs,
    ), patch.object(
        mod, "download_run_artifacts", return_value=False,
    ), patch.object(mod, "list_downloaded_artifacts", return_value=[]):
        rc = mod.main(
            [
                "--repo", "owner/repo",
                "--head-sha", "abc123",
                "--output-dir", str(tmp_path),
                "--workflows", "Linux",
            ],
        )
    assert rc == 1


def test_main_returns_two_when_no_artifacts_match_prefixes(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=runs), patch.object(
        mod, "select_latest_successful_runs", return_value=runs,
    ), patch.object(
        mod._gh_actions, "list_run_artifacts", return_value=[{"name": "unrelated-artifact", "size_in_bytes": 1}],
    ), patch.object(
        mod, "download_run_artifacts", return_value=True,
    ), patch.object(
        mod, "list_downloaded_files", return_value=[],
    ):
        rc = mod.main(
            [
                "--repo", "owner/repo",
                "--head-sha", "abc123",
                "--output-dir", str(tmp_path),
                "--workflows", "Linux",
                "--artifact-prefixes", "coverage-report",
            ],
        )
    assert rc == 2


def test_main_returns_one_on_invalid_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    runs_file.write_text("{not-json", encoding="utf-8")

    rc = mod.main(
        [
            "--repo", "owner/repo",
            "--head-sha", "abc123",
            "--runs-file", str(runs_file),
        ],
    )
    assert rc == 1


def test_main_returns_one_on_non_list_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    runs_file.write_text(json.dumps({"runs": []}), encoding="utf-8")

    rc = mod.main(
        [
            "--repo", "owner/repo",
            "--head-sha", "abc123",
            "--runs-file", str(runs_file),
        ],
    )
    assert rc == 1


def test_main_falls_back_to_older_successful_run_with_matching_artifacts(tmp_path: Path) -> None:
    runs = [
        {
            "id": 101,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-02-25T10:00:00Z",
        },
        {
            "id": 100,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-02-25T09:00:00Z",
        },
    ]

    def _artifacts_for_run(repo: str, run_id: int) -> list[dict[str, object]]:
        if run_id == 101:
            return [{"name": "unrelated-artifact", "size_in_bytes": 1}]
        if run_id == 100:
            return [{"name": "coverage-report", "size_in_bytes": 42}]
        return []

    downloaded = tmp_path / "coverage.xml"
    downloaded.write_text("<xml/>", encoding="utf-8")

    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=runs), patch.object(
        mod._gh_actions, "list_run_artifacts", side_effect=_artifacts_for_run,
    ), patch.object(
        mod, "download_run_artifacts", return_value=True,
    ) as download_mock, patch.object(
        mod, "list_downloaded_files", return_value=[downloaded],
    ):
        rc = mod.main(
            [
                "--repo", "owner/repo",
                "--head-sha", "abc123",
                "--output-dir", str(tmp_path),
                "--workflows", "Linux",
                "--artifact-prefixes", "coverage-report",
            ],
        )

    assert rc == 0
    assert download_mock.call_args.args[0] == 100


def test_main_writes_artifact_metadata_file(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    artifacts = [
        {"name": "coverage-report", "size_in_bytes": 100},
        {"name": "QGroundControl-x86_64.AppImage", "size_in_bytes": 200},
    ]
    metadata_path = tmp_path / "artifact-metadata.json"
    downloaded = tmp_path / "dummy.txt"
    downloaded.write_text("x", encoding="utf-8")

    with patch.object(mod._gh_actions, "list_workflow_runs_for_sha", return_value=runs), patch.object(
        mod, "select_latest_successful_runs", return_value=runs,
    ), patch.object(
        mod._gh_actions, "list_run_artifacts", return_value=artifacts,
    ), patch.object(
        mod, "download_run_artifacts", return_value=True,
    ), patch.object(
        mod, "list_downloaded_files", return_value=[downloaded],
    ):
        rc = mod.main(
            [
                "--repo", "owner/repo",
                "--head-sha", "abc123",
                "--output-dir", str(tmp_path),
                "--workflows", "Linux",
                "--artifact-prefixes", "coverage-report",
                "--artifact-metadata-out", str(metadata_path),
            ],
        )

    assert rc == 0
    assert metadata_path.exists()
