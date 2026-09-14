"""Tests for check_baseline_ready.py."""

from __future__ import annotations

from check_baseline_ready import evaluate_readiness

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


def test_terminal_failures_are_complete_but_not_successful():
    from qgc_tools.workflow_runs import evaluate_runs

    for conclusion in (
        "timed_out",
        "action_required",
        "startup_failure",
        "skipped",
        "cancelled",
        "failure",
        "neutral",
    ):
        runs = [_run("Linux", conclusion=conclusion)]
        assert evaluate_runs(runs, ["Linux"], "push", require_success=False) == (
            True,
            [],
            [],
            ["Linux"],
        )
        assert not evaluate_runs(runs, ["Linux"], "push")[0]


def test_gate_snapshot_preserves_latest_precommit_and_platform_runs(tmp_path, monkeypatch):
    import json

    from check_baseline_ready import main

    runs = [_run("Linux"), _run("pre-commit"), _run("Linux", created_at="2026-02-23T00:00:00Z")]
    source = tmp_path / "runs.json"
    target = tmp_path / "snapshot.json"
    source.write_text(json.dumps(runs))
    monkeypatch.setenv("GITHUB_OUTPUT", str(tmp_path / "out"))
    assert (
        main(
            [
                "--repo",
                "o/r",
                "--head-sha",
                "abc",
                "--platform-workflows",
                "Linux",
                "--runs-input",
                str(source),
                "--runs-cache",
                str(target),
            ]
        )
        == 0
    )
    assert json.loads(target.read_text()) == runs[:2]
