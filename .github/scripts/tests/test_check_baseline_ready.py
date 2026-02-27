"""Tests for check_baseline_ready.py."""

from __future__ import annotations

from pathlib import Path

import pytest

from check_baseline_ready import evaluate_readiness
from check_baseline_ready import write_output


PLATFORMS = ["Linux", "Windows", "MacOS", "Android"]


def _run(
    name: str,
    *,
    status: str = "completed",
    conclusion: str = "success",
    event: str = "push",
    created_at: str = "2026-02-24T00:00:00Z",
) -> dict[str, str]:
    return {
        "name": name,
        "status": status,
        "conclusion": conclusion,
        "event": event,
        "created_at": created_at,
    }


def test_evaluate_readiness_ready_when_all_latest_push_runs_succeed() -> None:
    runs = [_run(name) for name in PLATFORMS]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is True
    assert missing == []
    assert incomplete == []
    assert failed == []


def test_evaluate_readiness_reports_missing_incomplete_and_failed() -> None:
    runs = [
        _run("Linux", status="in_progress"),
        _run("Windows", conclusion="failure"),
        _run("MacOS"),
    ]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is False
    assert missing == ["Android"]
    assert incomplete == ["Linux"]
    assert failed == ["Windows"]


def test_evaluate_readiness_uses_latest_run_per_platform() -> None:
    runs = [
        _run("Linux", conclusion="failure", created_at="2026-02-24T00:00:00Z"),
        _run("Linux", conclusion="success", created_at="2026-02-24T01:00:00Z"),
        _run("Windows"),
        _run("MacOS"),
        _run("Android"),
    ]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is True
    assert missing == []
    assert incomplete == []
    assert failed == []


def test_evaluate_readiness_filters_by_event() -> None:
    runs = [_run(name, event="pull_request") for name in PLATFORMS]
    ready, missing, incomplete, failed = evaluate_readiness(runs, PLATFORMS, "push")
    assert ready is False
    assert missing == PLATFORMS
    assert incomplete == []
    assert failed == []


def test_write_output_multiline_uses_collision_resistant_delimiter(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    out_path = tmp_path / "out.txt"
    monkeypatch.setenv("GITHUB_OUTPUT", str(out_path))

    value = "line1\nEOF_missing_deadbeef\ntail"
    write_output("missing", value)

    text = out_path.read_text(encoding="utf-8")
    assert "missing<<EOF_missing_" in text
    assert "EOF_missing\n" not in text
    assert "line1\nEOF_missing_deadbeef\ntail" in text
