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


def test_get_workflow_runs_filters_by_name_status_and_conclusion() -> None:
    payload = "\n".join(
        [
            json.dumps({"id": 1, "name": "Linux", "status": "completed", "conclusion": "success"}),
            json.dumps({"id": 2, "name": "Linux", "status": "in_progress", "conclusion": None}),
            json.dumps({"id": 3, "name": "Windows", "status": "completed", "conclusion": "failure"}),
            json.dumps({"id": 4, "name": "Other", "status": "completed", "conclusion": "success"}),
            "not-json",
        ],
    )

    with patch.object(mod, "gh", return_value=_cp(stdout=payload)):
        runs = mod.get_workflow_runs("owner/repo", "abc", ["Linux", "Windows"])

    assert len(runs) == 1
    assert runs[0]["id"] == 1
    assert runs[0]["name"] == "Linux"


def test_download_run_artifacts_failure_returns_false() -> None:
    with patch.object(mod, "gh", return_value=_cp(stderr="bad", returncode=1)):
        ok = mod.download_run_artifacts(123, "owner/repo", Path("artifacts"))
    assert ok is False


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
    with patch.object(mod, "get_workflow_runs", return_value=[]), patch.object(
        mod, "gh", return_value=_cp(stdout="Linux: completed/success\n"),
    ):
        rc = mod.main(["--repo", "owner/repo", "--head-sha", "abc123"])
    assert rc == 0


def test_main_returns_one_when_downloads_fail_and_no_files(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    with patch.object(mod, "get_workflow_runs", return_value=runs), patch.object(
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
