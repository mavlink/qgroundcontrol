"""Tests for collect_build_status.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from ci.reporting import collect_build_status as mod
from ci.reporting.workflow_runs import configured_platform_workflows

from .._helpers import load_workflow

if TYPE_CHECKING:
    from collections.abc import Callable
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

    assert gh_output.read_text(encoding="utf-8") == "ready=true\n"


@pytest.mark.parametrize(
    ("event", "args", "ready"),
    [
        ("pull_request", ["--status-out", "status.json"], "true"),
        ("push", ["--require-success"], "false"),
    ],
)
def test_main_takes_its_query_from_the_triggering_run(
    event: str,
    args: list[str],
    ready: str,
    tmp_path: Path,
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    workflow_run_event: Callable[..., Path],
) -> None:
    """build-results.yml passes only --status-out (PR comment) or --require-success (baselines)."""
    workflow_run_event(head_sha="abc123", event=event, name="iOS", conclusion="success")
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.chdir(tmp_path)
    platforms = configured_platform_workflows().split(",")
    runs = [
        *(_run(name, event=event, run_id=index) for index, name in enumerate(platforms, 1)),
        _run("iOS", event=event, conclusion="failure", created_at="2026-02-24T01:00:00Z"),
        _run("Linux", event="push" if event == "pull_request" else "pull_request"),
    ]
    queries: list[tuple[str, str]] = []

    def list_runs(repo: str, sha: str) -> list[dict[str, object]]:
        queries.append((repo, sha))
        return runs

    monkeypatch.setattr(mod, "list_workflow_runs_for_sha", list_runs)
    monkeypatch.setattr(mod, "list_run_jobs", lambda repo, run_id: [])

    assert mod.main(args) == 0

    assert queries == [("owner/repo", "abc123")]
    assert gh_output.read_text(encoding="utf-8") == f"ready={ready}\n"
    snapshot = json.loads((tmp_path / "workflow-runs-cache.json").read_text())
    assert sorted(run["name"] for run in snapshot) == sorted(platforms)
    assert all(run["event"] == event for run in snapshot)
    if event == "pull_request":
        status = json.loads((tmp_path / "status.json").read_text())
        assert [entry["name"] for entry in status["platforms"]] == platforms
        assert (status["repo"], status["head_sha"]) == ("owner/repo", "abc123")


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


# The scripts take these from the workflow_run payload, build-config.json, and RUNNER_TEMP.
_DEFAULTED_OPTIONS = ("--repo", "--head-sha", "--event", "--platform-workflows", "--runs-cache")


def test_save_baselines_gates_on_one_status_query() -> None:
    steps = load_workflow("build-results.yml")["jobs"]["save-baselines"]["steps"]
    (status,) = (step for step in steps if "collect_build_status.py" in step.get("run", ""))
    assert status["id"] == "gate"
    assert "if" not in status
    assert status["run"] == "python3 tools/ci/reporting/collect_build_status.py --require-success"
    assert status["env"] == {"GH_TOKEN": "${{ github.token }}"}
    for step in steps[steps.index(status) + 1 :]:
        assert step["if"] in {
            "steps.gate.outputs.ready == 'true'",
            "steps.current.outputs.current == 'true'",
        }


def test_pr_comment_updates_on_every_trigger_from_one_status_query() -> None:
    steps = load_workflow("build-results.yml")["jobs"]["post-pr-comment"]["steps"]
    (status,) = (step for step in steps if "collect_build_status.py" in step.get("run", ""))
    assert status["run"] == (
        "python3 tools/ci/reporting/collect_build_status.py --status-out build-status.json"
    )
    assert status["env"] == {"GH_TOKEN": "${{ github.token }}"}
    later = steps[steps.index(status) + 1 :]
    # Partial results are posted too: nothing waits for every platform workflow or reads the
    # status step's outputs.
    assert not any("if" in step for step in later[:-1])
    (generate,) = (
        step for step in later if "generate_build_results_comment" in step.get("run", "")
    )
    assert generate["env"]["BASELINE_KEY"] == "${{ steps.baseline.outputs.cache-matched-key }}"
    assert later[-1]["if"] == "steps.current.outputs.current == 'true'"


@pytest.mark.parametrize("job", ["post-pr-comment", "save-baselines"])
def test_build_results_scripts_take_the_run_query_from_the_event(job: str) -> None:
    steps = load_workflow("build-results.yml")["jobs"][job]["steps"]
    runs = [step["run"] for step in steps if "tools/ci/reporting/" in step.get("run", "")]
    assert runs
    for run in runs:
        assert "${{" not in run
        assert not any(option in run for option in _DEFAULTED_OPTIONS), run


def _job(name: str, conclusion: str, steps: list[tuple[str, str]] = ()) -> dict[str, object]:  # type: ignore[assignment]
    return {
        "name": name,
        "status": "completed",
        "conclusion": conclusion,
        "html_url": f"https://example.test/job/{name}",
        "started_at": "2026-02-24T00:00:00Z",
        "completed_at": "2026-02-24T00:10:30Z",
        "steps": [
            {"name": step, "number": index, "conclusion": result}
            for index, (step, result) in enumerate(steps, 1)
        ],
    }


def test_summarize_job_links_failed_steps() -> None:
    job = mod.summarize_job(
        _job("Build x64", "failure", [("Checkout", "success"), ("Build", "failure")])
    )
    assert job == {
        "name": "Build x64",
        "status": "completed",
        "conclusion": "failure",
        "url": "https://example.test/job/Build x64",
        "duration_seconds": 630.0,
        "failed_steps": [
            {"name": "Build", "number": 2, "url": "https://example.test/job/Build x64#step:2:1"}
        ],
    }


@pytest.mark.parametrize(
    ("run", "jobs", "state"),
    [
        (None, [], "pending"),
        ({"status": "queued"}, [], "pending"),
        ({"status": "in_progress"}, [], "running"),
        ({"status": "completed", "conclusion": "success"}, [("Build", "success")], "success"),
        ({"status": "completed", "conclusion": "timed_out"}, [], "failure"),
        ({"status": "completed", "conclusion": "cancelled"}, [], "cancelled"),
        ({"status": "completed", "conclusion": "action_required"}, [], "action_required"),
        # Change detection skipped every build; only the planner and the gate ran.
        (
            {"status": "completed", "conclusion": "success"},
            [("changes / Detect Changes", "success"), ("Build", "skipped"), ("Linux", "success")],
            "skipped",
        ),
        (
            {"status": "completed", "conclusion": "success"},
            [("Plan Analysis", "success"), ("Android (linux)", "skipped"), ("Linux", "success")],
            "skipped",
        ),
        (
            {"status": "completed", "conclusion": "success"},
            [("changes / Detect Changes", "success"), ("Build", "success"), ("Lint", "skipped")],
            "success",
        ),
    ],
)
def test_workflow_state(
    run: dict[str, str] | None, jobs: list[tuple[str, str]], state: str
) -> None:
    summaries = [mod.summarize_job(_job(name, conclusion)) for name, conclusion in jobs]
    assert mod.workflow_state(run, summaries, "Linux") == state


def test_status_out_describes_platforms_precommit_and_analysis(
    tmp_path: Path, gh_output: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [
        {**_run("Linux", run_id=11), "run_number": 7, "run_started_at": "2026-02-24T00:00:00Z"}
        | {"updated_at": "2026-02-24T00:25:00Z"},
        _run("pre-commit", run_id=12, conclusion="failure"),
        _run("Code Analysis", run_id=13, status="in_progress"),
    ]
    jobs = {
        11: [_job("Release", "success")],
        12: [_job("pre-commit", "failure", [("Run pre-commit", "failure")])],
        13: [],
    }
    monkeypatch.setattr(mod, "list_run_jobs", lambda repo, run_id: jobs[run_id])
    out = tmp_path / "status.json"
    text = _gate(
        runs, gh_output, monkeypatch, "--status-out", str(out), platforms=["Linux", "MacOS"]
    )
    status = json.loads(out.read_text())
    linux, macos = status["platforms"]
    assert (linux["state"], linux["run_number"], linux["duration_seconds"]) == (
        "success",
        7,
        1500.0,
    )
    assert macos == {
        "name": "MacOS",
        "state": "pending",
        "run_id": "",
        "url": "",
        "run_number": None,
        "duration_seconds": None,
        "jobs": [],
    }
    assert status["precommit"]["state"] == "failure"
    assert status["precommit"]["jobs"][0]["failed_steps"][0]["name"] == "Run pre-commit"
    assert status["analysis"]["state"] == "running"
    assert text == "ready=false\n"


def test_status_out_tolerates_job_listing_failures(
    tmp_path: Path,
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    def fail(repo: str, run_id: int) -> list[dict[str, object]]:
        raise RuntimeError("API down")

    monkeypatch.setattr(mod, "list_run_jobs", fail)
    out = tmp_path / "status.json"
    _gate(
        [_run("Linux", run_id=11)],
        gh_output,
        monkeypatch,
        "--status-out",
        str(out),
        platforms=["Linux"],
    )
    assert json.loads(out.read_text())["platforms"][0]["state"] == "success"
    assert "::warning::Cannot list the jobs of Linux run 11: API down" in capsys.readouterr().out


def test_save_baselines_skips_setup_python() -> None:
    # tools/tests/test_stdlib_only.py checks that its scripts import with the standard library only.
    workflow = load_workflow("build-results.yml")
    steps = workflow["jobs"]["save-baselines"]["steps"]
    assert not any(step.get("uses") == "./.github/actions/setup-python" for step in steps)
