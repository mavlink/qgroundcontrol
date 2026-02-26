"""Tests for collect_build_status.py."""

from __future__ import annotations

from pathlib import Path

import pytest

import collect_build_status as mod


def _run(
    name: str,
    *,
    created_at: str = "2026-02-24T00:00:00Z",
    status: str = "completed",
    conclusion: str = "success",
    event: str = "pull_request",
    html_url: str = "https://example.test/run",
    run_id: int = 1,
) -> dict[str, object]:
    return {
        "name": name,
        "created_at": created_at,
        "status": status,
        "conclusion": conclusion,
        "event": event,
        "html_url": html_url,
        "id": run_id,
    }


def test_latest_runs_by_name_filters_by_event_and_picks_latest() -> None:
    runs = [
        _run("Linux", created_at="2026-02-24T00:00:00Z"),
        _run("Linux", created_at="2026-02-24T01:00:00Z", conclusion="failure"),
        _run("Linux", created_at="2026-02-24T02:00:00Z", event="push"),
        _run("Windows", created_at="2026-02-24T01:00:00Z"),
    ]

    latest = mod.latest_runs_by_name(runs, {"Linux", "Windows"}, "pull_request")

    assert latest["Linux"]["conclusion"] == "failure"
    assert latest["Windows"]["status"] == "completed"


def test_main_writes_expected_outputs(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    out_path = tmp_path / "out.txt"
    monkeypatch.setenv("GITHUB_OUTPUT", str(out_path))

    runs = [
        _run("Linux", html_url="https://example.test/linux"),
        _run("Windows", html_url="https://example.test/windows"),
        _run("MacOS", html_url="https://example.test/macos"),
        _run("Android", html_url="https://example.test/android"),
        _run("pre-commit", run_id=99, html_url="https://example.test/precommit"),
    ]
    monkeypatch.setattr(mod, "list_workflow_runs", lambda repo, sha: runs)

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

    text = out_path.read_text(encoding="utf-8")
    assert "all_complete=true" in text
    assert "summary=All builds passed." in text
    assert "precommit_status=Passed" in text
    assert "precommit_run_id=99" in text
    assert "| Linux | Passed | [View](https://example.test/linux) |" in text


def test_write_output_uses_collision_resistant_delimiter(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    out_path = tmp_path / "out.txt"
    monkeypatch.setenv("GITHUB_OUTPUT", str(out_path))

    value = "line1\nEOF_table_deadbeef\ntail"
    mod.write_output("table", value)

    text = out_path.read_text(encoding="utf-8")
    assert "table<<EOF_table_" in text
    assert "EOF_table\n" not in text
    assert "line1\nEOF_table_deadbeef\ntail" in text
