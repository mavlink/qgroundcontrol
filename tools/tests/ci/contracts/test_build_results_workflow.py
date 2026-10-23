"""build-results.yml steps pass only what differs from their scripts' defaults.

The reporting scripts take the repository, commit, and event from the triggering workflow_run
payload, the platform workflows from build-config.json, and the run snapshot path from
RUNNER_TEMP, and the comment generator reads the files the earlier steps write at its default
paths. These tests replay each step's command line under fake pull_request and push payloads.
"""

from __future__ import annotations

import shlex
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest
from ci.reporting import collect_build_status, download_artifacts, generate_build_results_comment
from ci.reporting.workflow_runs import configured_platform_workflows

from ..._helpers import load_workflow

if TYPE_CHECKING:
    import argparse
    from collections.abc import Callable

WORKFLOW = "build-results.yml"
HEAD_SHA = "0123456789abcdef0123456789abcdef01234567"


def _steps(job: str) -> list[dict[str, Any]]:
    return load_workflow(WORKFLOW)["jobs"][job]["steps"]


def _step(job: str, name: str) -> dict[str, Any]:
    (step,) = (step for step in _steps(job) if step.get("name") == name)
    return step


def _script_args(job: str, script: str) -> dict[str, list[str]]:
    """Map each step of *job* that runs tools/ci/reporting/<script> to its arguments."""
    found = {}
    for step in _steps(job):
        argv = shlex.split(step.get("run", "").replace("\\\n", " "))
        if argv[:2] == ["python3", f"tools/ci/reporting/{script}"]:
            found[step["name"]] = argv[2:]
    return found


@pytest.fixture
def trigger(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, workflow_run_event: Callable[..., Path]
) -> Callable[[str], Path]:
    """Set up a build-results job triggered by a completed *event* build; return RUNNER_TEMP."""

    def setup(event: str) -> Path:
        workflow_run_event(head_sha=HEAD_SHA, event=event, name="iOS", conclusion="success")
        monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
        monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
        return tmp_path

    return setup


def _assert_triggering_query(args: argparse.Namespace, event: str, runner_temp: Path) -> None:
    assert (args.repo, args.head_sha, args.event) == ("owner/repo", HEAD_SHA, event)
    assert args.runs_cache == str(runner_temp / "workflow-runs-cache.json")


def _downloads(job: str) -> dict[str, argparse.Namespace]:
    return {
        name: download_artifacts.parse_args(argv)
        for name, argv in _script_args(job, "download_artifacts.py").items()
    }


def test_pr_comment_steps_query_the_triggering_pr_build(trigger: Callable[[str], Path]) -> None:
    runner_temp = trigger("pull_request")
    platforms = configured_platform_workflows()

    (collect,) = _script_args("post-pr-comment", "collect_build_status.py").values()
    status = collect_build_status.parse_args(collect)
    _assert_triggering_query(status, "pull_request", runner_temp)
    assert status.platform_workflows == platforms
    assert not status.require_success

    downloads = _downloads("post-pr-comment")
    assert {name: args.workflows for name, args in downloads.items()} == {
        "Download artifacts from all platform workflows": platforms,
        "Download pre-commit results": "pre-commit",
        "Download Code Analysis results": "Code Analysis",
    }
    for args in downloads.values():
        _assert_triggering_query(args, "pull_request", runner_temp)
        # Diagnostics of failed builds are what the comment is for; tests may not have run.
        assert args.include_failed and args.allow_missing and not args.require_sizes


def test_baseline_steps_query_the_triggering_master_push(trigger: Callable[[str], Path]) -> None:
    runner_temp = trigger("push")

    (gate,) = _script_args("save-baselines", "collect_build_status.py").values()
    status = collect_build_status.parse_args(gate)
    _assert_triggering_query(status, "push", runner_temp)
    assert status.platform_workflows == configured_platform_workflows()
    assert status.require_success and status.status_out is None

    (download,) = _downloads("save-baselines").values()
    _assert_triggering_query(download, "push", runner_temp)
    assert download.workflows == configured_platform_workflows()
    # Baselines come only from successful builds and must be complete.
    assert not (download.include_failed or download.allow_missing)
    assert download.require_sizes
    # Only the coverage summary is saved; sizes come from every run's artifact listing.
    assert download.artifact_prefixes == "coverage-report"
    prepare = _step("save-baselines", "Prepare immutable baseline")["run"]
    coverage = download.output_dir / "coverage-report" / "coverage-summary.json"
    assert f"test -f {download.sizes_out}" in prepare
    assert f"cp {coverage} baseline-coverage-summary.json" in prepare
    publish = _step("save-baselines", "Publish immutable baseline")["with"]["path"]
    assert set(publish.split()) == {str(download.sizes_out), "baseline-coverage-summary.json"}


def test_pr_downloads_fail_only_for_platform_artifacts() -> None:
    steps = {step.get("name"): step for step in _steps("post-pr-comment")}
    assert "continue-on-error" not in steps["Download artifacts from all platform workflows"]
    assert "if" not in steps["Download artifacts from all platform workflows"]
    # pre-commit and Code Analysis results are extras; download_artifacts.py skips their runs
    # until they complete, so they need no gate.
    for name in ("Download pre-commit results", "Download Code Analysis results"):
        assert steps[name]["continue-on-error"] is True
        assert "if" not in steps[name]


def test_comment_reads_what_the_earlier_steps_write(trigger: Callable[[str], Path]) -> None:
    trigger("pull_request")
    paths = {key: Path(value) for key, value in generate_build_results_comment.INPUT_PATHS.items()}
    (collect,) = _script_args("post-pr-comment", "collect_build_status.py").values()
    assert collect_build_status.parse_args(collect).status_out == paths["BUILD_STATUS_JSON"]

    downloads = _downloads("post-pr-comment")
    platform = downloads["Download artifacts from all platform workflows"]
    precommit = downloads["Download pre-commit results"]
    analysis = downloads["Download Code Analysis results"]
    assert platform.output_dir == precommit.output_dir == paths["ARTIFACTS_DIR"]
    assert analysis.output_dir == paths["ANALYSIS_ARTIFACTS_DIR"]
    assert platform.sizes_out == paths["PR_SIZES_JSON"]
    assert "coverage-report" in platform.artifact_prefixes.split(",")
    assert paths["COVERAGE_SUMMARY"].parent == platform.output_dir / "coverage-report"
    # A single downloaded artifact lands in <output-dir>/<artifact name>/.
    assert paths["PRECOMMIT_RESULTS_PATH"].parent == precommit.output_dir / "pre-commit-results"
    assert precommit.artifact_prefixes == "pre-commit-results"

    restore = _step("post-pr-comment", "Restore baseline snapshot")["with"]["path"]
    assert set(map(Path, restore.split())) == {
        paths["BASELINE_SIZES_JSON"],
        paths["BASELINE_COVERAGE_SUMMARY"],
    }

    generate = _step("post-pr-comment", "Generate combined report")
    assert generate["run"] == "python3 tools/ci/reporting/generate_build_results_comment.py"
    assert set(generate["env"]) == {"BASELINE_KEY", "BASELINE_SHA", "PR_NUMBER"}


def test_comment_defaults_to_the_triggering_build(
    trigger: Callable[[str], Path],
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
) -> None:
    trigger("pull_request")
    workspace = tmp_path / "workspace"
    workspace.mkdir()
    monkeypatch.chdir(workspace)
    (workspace / "build-status.json").write_text(
        f'{{"repo": "owner/repo", "head_sha": "{HEAD_SHA}", "platforms": []}}', encoding="utf-8"
    )

    assert generate_build_results_comment.main([]) == 0

    posted = _step("post-pr-comment", "Post combined comment")["with"]["file-path"]
    comment = (workspace / posted).read_text(encoding="utf-8")
    assert f"/commit/{HEAD_SHA})" in comment
    assert "triggered by iOS" in comment
    assert gh_summary.read_text(encoding="utf-8") == comment


def test_jobs_run_for_the_events_report_context_selects() -> None:
    jobs = load_workflow(WORKFLOW)["jobs"]
    outputs = jobs["report-context"]["outputs"]
    assert outputs["pr"] == "${{ steps.context.outputs.pr }}"
    assert outputs["baseline"] == "${{ steps.context.outputs.baseline }}"
    # report_context.py sets `pr` only for pull_request builds and `baseline` only for successful
    # master pushes (tools/tests/ci/test_report_context.py).
    assert jobs["post-pr-comment"]["if"] == "needs.report-context.outputs.pr != ''"
    assert jobs["save-baselines"]["if"] == "needs.report-context.outputs.baseline == 'true'"
