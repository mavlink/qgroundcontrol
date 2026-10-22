"""Tests for collect_build_status.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from ci.reporting import collect_build_status as mod

from .._helpers import load_workflow

if TYPE_CHECKING:
    from pathlib import Path


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

    latest = mod.select_latest_runs_by_name(runs, {"Linux", "Windows"}, event="pull_request")

    assert latest["Linux"]["conclusion"] == "failure"
    assert latest["Windows"]["status"] == "completed"


def test_main_writes_expected_outputs(gh_output: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    runs = [
        _run("Linux", html_url="https://example.test/linux"),
        _run("Windows", html_url="https://example.test/windows"),
        _run("MacOS", html_url="https://example.test/macos"),
        _run("Android", html_url="https://example.test/android"),
        _run("pre-commit", run_id=99, html_url="https://example.test/precommit"),
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
    assert "ready=true" in text
    assert "summary=All builds passed." in text
    assert "precommit_status=Passed" in text
    assert "precommit_run_id=99" in text
    assert "| Linux | Passed | [View](https://example.test/linux) |" in text


PLATFORMS = ["Linux", "Windows", "MacOS", "Android"]


def _gate(
    runs: list[dict[str, object]],
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    *args: str,
    platforms: list[str] = PLATFORMS,
) -> str:
    monkeypatch.setattr(mod, "list_workflow_runs_for_sha", lambda repo, sha: runs)
    argv = ["--repo", "o/r", "--head-sha", "abc", "--platform-workflows", ",".join(platforms)]
    assert mod.main([*argv, *args]) == 0
    return gh_output.read_text(encoding="utf-8")


def test_gate_ready_when_all_latest_push_runs_succeed(
    gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [_run(name, event="push") for name in PLATFORMS]
    text = _gate(runs, gh_output, monkeypatch, "--event", "push", "--require-success")
    assert "ready=true\n" in text


def test_gate_reports_missing_incomplete_and_failed(
    gh_output: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    runs = [
        _run("Linux", status="in_progress", event="push"),
        _run("Windows", conclusion="failure", event="push"),
        _run("MacOS", event="push"),
    ]
    text = _gate(runs, gh_output, monkeypatch, "--event", "push", "--require-success")
    assert "ready=false\n" in text
    assert "Missing=[Android] Incomplete=[Linux] Failed=[Windows]" in capsys.readouterr().out


def test_gate_uses_latest_run_per_platform(
    gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [
        _run("Linux", conclusion="failure", event="push"),
        _run("Linux", created_at="2026-02-24T01:00:00Z", event="push"),
        *(_run(name, event="push") for name in PLATFORMS[1:]),
    ]
    text = _gate(runs, gh_output, monkeypatch, "--event", "push", "--require-success")
    assert "ready=true\n" in text


def test_gate_filters_by_event(
    gh_output: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    runs = [_run(name) for name in PLATFORMS]
    text = _gate(runs, gh_output, monkeypatch, "--event", "push", "--require-success")
    assert "ready=false\n" in text
    assert "Missing=[Linux, Windows, MacOS, Android]" in capsys.readouterr().out


@pytest.mark.parametrize(
    "conclusion",
    [
        "timed_out",
        "action_required",
        "startup_failure",
        "skipped",
        "cancelled",
        "failure",
        "neutral",
    ],
)
def test_terminal_failures_are_complete_but_not_successful(
    conclusion: str, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [_run("Linux", conclusion=conclusion)]
    completed = _gate(runs, gh_output, monkeypatch, platforms=["Linux"])
    assert "ready=true\n" in completed
    assert "summary=Some builds failed.\n" in completed
    gh_output.write_text("")
    succeeded = _gate(runs, gh_output, monkeypatch, "--require-success", platforms=["Linux"])
    assert "ready=false\n" in succeeded


def test_snapshot_keeps_latest_platform_and_precommit_runs(
    tmp_path: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [
        _run("Linux", created_at="2026-02-24T01:00:00Z"),
        _run("pre-commit"),
        _run("Linux"),
        _run("Linux", created_at="2026-02-24T02:00:00Z", event="push"),
        _run("Docker"),
    ]
    snapshot = tmp_path / "snapshot.json"
    _gate(runs, gh_output, monkeypatch, "--runs-cache", str(snapshot), platforms=["Linux"])
    assert json.loads(snapshot.read_text()) == runs[:2]


@pytest.mark.parametrize(
    ("job", "step_id", "require_success"),
    [("post-pr-comment", "builds", False), ("save-baselines", "gate", True)],
)
def test_build_results_jobs_gate_on_one_status_query(
    job: str, step_id: str, require_success: bool
) -> None:
    workflow = load_workflow("build-results.yml")
    steps = workflow["jobs"][job]["steps"]
    (status,) = (step for step in steps if "collect_build_status.py" in step.get("run", ""))
    assert status["id"] == step_id
    assert "if" not in status
    assert ("--require-success" in status["run"]) == require_success
    assert "--runs-cache" in status["run"]
    for step in steps[steps.index(status) + 1 :]:
        assert step["if"] in {
            f"steps.{step_id}.outputs.ready == 'true'",
            "steps.current.outputs.current == 'true'",
        } or step["if"].startswith(f"steps.{step_id}.outputs.ready == 'true' &&")


def test_save_baselines_skips_setup_python() -> None:
    # tools/tests/test_stdlib_only.py checks that its scripts import with the standard library only.
    workflow = load_workflow("build-results.yml")
    steps = workflow["jobs"]["save-baselines"]["steps"]
    assert not any(step.get("uses") == "./.github/actions/setup-python" for step in steps)
