"""Tests for collect_build_status.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import collect_build_status as mod
from _helpers import workflow_run

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_main_writes_expected_outputs(gh_output: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    runs = [
        workflow_run("Linux", html_url="https://example.test/linux"),
        workflow_run("Windows", html_url="https://example.test/windows"),
        workflow_run("MacOS", html_url="https://example.test/macos"),
        workflow_run("Android", html_url="https://example.test/android"),
        workflow_run("pre-commit", 99, html_url="https://example.test/precommit"),
    ]
    monkeypatch.setattr(mod, "list_workflow_runs_for_sha", lambda repo, sha: runs)

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--platform-workflows",
            "Linux,Windows,MacOS,Android",
            "--event",
            "pull_request",
        ]
    )
    assert rc == 0

    text = gh_output.read_text(encoding="utf-8")
    assert "all_complete=true" in text
    assert "summary=All builds passed." in text
    assert "precommit_status=Passed" in text
    assert "precommit_run_id=99" in text
    assert "| Linux | Passed | [View](https://example.test/linux) |" in text


def test_timed_out_workflow_is_complete_but_failed(
    gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [workflow_run("Linux", conclusion="timed_out")]
    monkeypatch.setattr(mod, "list_workflow_runs_for_sha", lambda repo, sha: runs)

    assert (
        mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--platform-workflows",
                "Linux",
                "--event",
                "pull_request",
            ]
        )
        == 0
    )
    text = gh_output.read_text(encoding="utf-8")
    assert "all_complete=true" in text
    assert "summary=Some builds failed." in text
