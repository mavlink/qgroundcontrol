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


def test_main_takes_its_query_from_the_triggering_run(
    tmp_path: Path,
    gh_output: Path,
    monkeypatch: pytest.MonkeyPatch,
    workflow_run_event: Callable[..., Path],
) -> None:
    """build-results.yml passes only --status-out."""
    event = "pull_request"
    workflow_run_event(head_sha="abc123", event=event, name="iOS", conclusion="success")
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.chdir(tmp_path)
    platforms = configured_platform_workflows().split(",")
    runs = [
        *(_run(name, event=event, run_id=index) for index, name in enumerate(platforms, 1)),
        _run("iOS", event=event, conclusion="failure", created_at="2026-02-24T01:00:00Z"),
        _run("Linux", event="push"),
    ]
    queries: list[tuple[str, str]] = []

    def list_runs(repo: str, sha: str) -> list[dict[str, object]]:
        queries.append((repo, sha))
        return runs

    monkeypatch.setattr(mod, "list_workflow_runs_for_sha", list_runs)
    monkeypatch.setattr(mod, "list_run_jobs", lambda repo, run_id: [])

    assert mod.main(["--status-out", "status.json"]) == 0

    assert queries == [("owner/repo", "abc123")]
    assert gh_output.read_text(encoding="utf-8") == ""
    snapshot = json.loads((tmp_path / "workflow-runs-cache.json").read_text())
    assert sorted(run["name"] for run in snapshot) == sorted(platforms)
    assert all(run["event"] == event for run in snapshot)
    status = json.loads((tmp_path / "status.json").read_text())
    assert [entry["name"] for entry in status["platforms"]] == platforms
    assert (status["repo"], status["head_sha"]) == ("owner/repo", "abc123")


PLATFORMS = ["Linux", "Windows", "MacOS", "Android"]


def _main(
    runs: list[dict[str, object]],
    monkeypatch: pytest.MonkeyPatch,
    *args: str,
    platforms: list[str] = PLATFORMS,
) -> None:
    monkeypatch.setattr(mod, "list_workflow_runs_for_sha", lambda repo, sha: runs)
    argv = ["--repo", "o/r", "--head-sha", "abc", "--platform-workflows", ",".join(platforms)]
    assert mod.main([*argv, *args]) == 0


def test_snapshot_keeps_latest_platform_and_precommit_runs(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [
        _run("Linux", created_at="2026-02-24T01:00:00Z"),
        _run("pre-commit"),
        _run("Linux"),
        _run("Linux", created_at="2026-02-24T02:00:00Z", event="push"),
        _run("Docker"),
    ]
    snapshot = tmp_path / "snapshot.json"
    _main(runs, monkeypatch, "--runs-cache", str(snapshot), platforms=["Linux"])
    assert json.loads(snapshot.read_text()) == runs[:2]


# The scripts take these from the workflow_run payload, build-config.json, and RUNNER_TEMP.
_DEFAULTED_OPTIONS = ("--repo", "--head-sha", "--event", "--platform-workflows", "--runs-cache")


def test_pr_comment_updates_on_every_trigger_from_one_status_query() -> None:
    steps = load_workflow("build-results.yml")["jobs"]["post-pr-comment"]["steps"]
    (status,) = (step for step in steps if "collect_build_status.py" in step.get("run", ""))
    assert status["run"] == (
        "python3 tools/ci/reporting/collect_build_status.py --status-out build-status.json"
    )
    assert status["env"] == {"GH_TOKEN": "${{ github.token }}"}
    later = steps[steps.index(status) + 1 :]
    # Partial results are posted too: nothing waits for every platform workflow or reads the
    # status step's outputs, and publishing rechecks the PR head itself.
    assert not any("if" in step for step in later)
    assert later[-1]["run"] == "python3 tools/ci/reporting/report_context.py --comment comment.md"


def test_build_results_scripts_take_the_run_query_from_the_event() -> None:
    steps = load_workflow("build-results.yml")["jobs"]["post-pr-comment"]["steps"]
    runs = {
        step["name"]: step["run"] for step in steps if "tools/ci/reporting/" in step.get("run", "")
    }
    # Only the PR base's download queries another commit, named through the environment.
    base = runs.pop("Download the PR base commit's build artifacts")
    assert '--head-sha "$BASE_SHA" --event push' in base
    assert "--runs-cache ''" in base
    assert runs
    for run in [base, *runs.values()]:
        assert "${{" not in run
    for run in runs.values():
        assert not any(option in run for option in _DEFAULTED_OPTIONS), run


def _job(name: str, conclusion: str, steps: list[tuple[str, str]] = ()) -> dict[str, object]:  # type: ignore[assignment]
    return {
        "id": 120600016436,
        "name": name,
        "status": "completed",
        "conclusion": conclusion,
        "html_url": f"https://example.test/job/{name}",
        "started_at": "2026-02-24T00:00:00Z",
        "completed_at": "2026-02-24T00:10:30Z",
        "steps": [
            {"name": step, "number": index, "status": "completed", "conclusion": result}
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
        "runner_lost": None,
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


# A Windows RunsOn job (run 40007740206, job 120600016436) whose runner vanished during Build Setup:
# that step stayed in progress, the later ones pending, and the job failed with no annotation.
LOST_STEPS = [
    {
        "name": "Checkout repo",
        "number": 7,
        "status": "completed",
        "conclusion": "success",
        "started_at": "2026-10-23T19:02:10Z",
    },
    {
        "name": "Build Setup",
        "number": 9,
        "status": "in_progress",
        "conclusion": None,
        "started_at": "2026-10-23T19:02:59Z",
    },
    {
        "name": "Configure",
        "number": 10,
        "status": "pending",
        "conclusion": None,
        "started_at": None,
    },
]
HOSTED_LOST = (
    "The hosted runner: GitHub Actions 1000370428 lost communication with the server. Anything "
    "in your workflow that terminates the runner process, starves it for CPU/Memory, or blocks "
    "its network access can cause this error."
)
SELF_HOSTED_LOST = (
    "The self-hosted runner: runs-on--i-07f4fddec4e1df5c8 lost communication with the server. "
    "Verify the machine is running and has a healthy network connection."
)


def _annotations(monkeypatch: pytest.MonkeyPatch, *messages: str) -> list[str]:
    queried: list[str] = []

    def api(path: str) -> list[dict[str, str]]:
        queried.append(path)
        return [{"annotation_level": "failure", "message": message} for message in messages]

    monkeypatch.setattr(mod, "gh_api_json", api)
    return queried


def test_runner_lost_mid_step_needs_no_annotations(monkeypatch: pytest.MonkeyPatch) -> None:
    queried = _annotations(monkeypatch)
    job = {
        **_job("Build win64_msvc2022_64 Release", "failure"),
        "started_at": "2026-10-23T19:01:56Z",
        "completed_at": "2026-10-23T19:04:31Z",
        "steps": LOST_STEPS,
    }
    assert mod.runner_lost("mavlink/qgroundcontrol", job) == {
        "step": "Build Setup",
        "started_at": "2026-10-23T19:01:56Z",
        "completed_at": "2026-10-23T19:04:31Z",
    }
    assert queried == []
    summary = mod.summarize_job(job, lost=mod.runner_lost("mavlink/qgroundcontrol", job))
    assert summary["runner_lost"]["step"] == "Build Setup"
    assert summary["failed_steps"] == []


@pytest.mark.parametrize("message", [HOSTED_LOST, SELF_HOSTED_LOST])
def test_runner_lost_annotation_names_the_last_started_step(
    monkeypatch: pytest.MonkeyPatch, message: str
) -> None:
    queried = _annotations(monkeypatch, "Process completed with exit code 1.", message)
    job = _job("Build", "failure", [("Checkout", "success"), ("Build", "success")])
    job["steps"][1]["started_at"] = "2026-02-24T00:01:00Z"  # type: ignore[index]
    lost = mod.runner_lost("o/r", job)
    assert lost == {
        "step": "Build",
        "started_at": "2026-02-24T00:00:00Z",
        "completed_at": "2026-02-24T00:10:30Z",
    }
    assert queried == ["repos/o/r/check-runs/120600016436/annotations"]


@pytest.mark.parametrize(
    ("conclusion", "steps", "annotations", "queried"),
    [
        # A failed step explains the failure; no annotation lookup.
        ("failure", [("Checkout", "success"), ("Build", "failure")], [HOSTED_LOST], False),
        # Passing, cancelled, and timed-out jobs are not lost runners.
        ("success", [("Build", "success")], [HOSTED_LOST], False),
        ("cancelled", [("Build", "cancelled")], [HOSTED_LOST], False),
        ("timed_out", [("Build", "success")], [HOSTED_LOST], False),
        # Without a failed step, only GitHub's lost-runner message counts.
        ("failure", [("Build", "success")], ["Process completed with exit code 1."], True),
    ],
)
def test_other_failures_are_not_lost_runners(
    monkeypatch: pytest.MonkeyPatch,
    conclusion: str,
    steps: list[tuple[str, str]],
    annotations: list[str],
    queried: bool,
) -> None:
    calls = _annotations(monkeypatch, *annotations)
    assert mod.runner_lost("o/r", _job("Build", conclusion, steps)) is None
    assert bool(calls) is queried


def test_unreadable_annotations_leave_a_failure_failed(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    def fail(path: str) -> None:
        raise RuntimeError("API down")

    monkeypatch.setattr(mod, "gh_api_json", fail)
    assert mod.runner_lost("owner/repo", _job("Build", "failure", [("Build", "success")])) is None
    assert (
        "::warning::Cannot read the annotations of job Build: API down" in capsys.readouterr().out
    )


def test_status_out_describes_platforms_precommit_and_analysis(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [
        # Attempt 2 started 25 minutes after the run was created and took 25 minutes.
        {**_run("Linux", run_id=11), "run_number": 7, "run_attempt": 2}
        | {"run_started_at": "2026-02-24T00:25:00Z", "updated_at": "2026-02-24T00:50:00Z"},
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
    _main(runs, monkeypatch, "--status-out", str(out), platforms=["Linux", "MacOS"])
    status = json.loads(out.read_text())
    linux, macos = status["platforms"]
    assert (linux["state"], linux["run_number"], linux["attempt"]) == ("success", 7, 2)
    assert (linux["duration_seconds"], linux["elapsed_seconds"]) == (1500.0, 3000.0)
    assert macos == {
        "name": "MacOS",
        "state": "pending",
        "run_id": "",
        "url": "",
        "run_number": None,
        "attempt": None,
        "duration_seconds": None,
        "elapsed_seconds": None,
        "jobs": [],
    }
    assert status["precommit"]["state"] == "failure"
    assert status["precommit"]["jobs"][0]["failed_steps"][0]["name"] == "Run pre-commit"
    assert status["analysis"]["state"] == "running"
    assert status["others"] == []


def test_status_out_lists_other_workflows_and_jobs_only_of_failed_ones(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    runs = [
        _run("Linux", run_id=11),
        _run("CI Scripts", run_id=21, conclusion="failure"),
        _run("docker", run_id=22, status="in_progress"),
        _run("CodeQL", run_id=23),
        _run("CodeQL", run_id=24, created_at="2026-02-23T00:00:00Z", conclusion="failure"),
        # PR Checks runs on pull_request_target, so the event filter leaves it out.
        _run("PR Checks", run_id=25, event="pull_request_target"),
    ]
    listed: list[int] = []

    def list_jobs(repo: str, run_id: int) -> list[dict[str, object]]:
        listed.append(run_id)
        return [_job("Test CI Scripts (py3.12)", "failure", [("Run CI script tests", "failure")])]

    monkeypatch.setattr(mod, "list_run_jobs", list_jobs)
    out = tmp_path / "status.json"
    snapshot = tmp_path / "snapshot.json"
    _main(
        runs,
        monkeypatch,
        *("--status-out", str(out), "--runs-cache", str(snapshot), "--event", "pull_request"),
        platforms=["Linux"],
    )
    others = json.loads(out.read_text())["others"]
    assert [(entry["name"], entry["state"]) for entry in others] == [
        ("CI Scripts", "failure"),
        ("CodeQL", "success"),
        ("docker", "running"),
    ]
    assert others[0]["jobs"][0]["failed_steps"][0]["name"] == "Run CI script tests"
    assert others[1]["jobs"] == others[2]["jobs"] == []
    assert sorted(listed) == [11, 21]
    # The run snapshot for artifact downloads keeps only the platform workflows.
    assert [run["name"] for run in json.loads(snapshot.read_text())] == ["Linux"]


def test_status_out_tolerates_job_listing_failures(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    def fail(repo: str, run_id: int) -> list[dict[str, object]]:
        raise RuntimeError("API down")

    monkeypatch.setattr(mod, "list_run_jobs", fail)
    out = tmp_path / "status.json"
    _main([_run("Linux", run_id=11)], monkeypatch, "--status-out", str(out), platforms=["Linux"])
    assert json.loads(out.read_text())["platforms"][0]["state"] == "success"
    assert "::warning::Cannot list the jobs of Linux run 11: API down" in capsys.readouterr().out
