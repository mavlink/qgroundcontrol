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
BASE_STEP = "Download the PR base commit's build artifacts"


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

    downloads = _downloads("post-pr-comment")
    assert {name: args.workflows for name, args in downloads.items()} == {
        "Download artifacts from all platform workflows": platforms,
        "Download pre-commit results": "pre-commit",
        "Download Code Analysis results": "Code Analysis",
        BASE_STEP: platforms,
    }
    base = downloads.pop(BASE_STEP)
    for args in downloads.values():
        _assert_triggering_query(args, "pull_request", runner_temp)
    for args in [*downloads.values(), base]:
        # Diagnostics of failed builds are what the comment is for; tests may not have run.
        assert args.include_failed and args.allow_missing


def test_base_download_queries_the_pr_base_commits_push_builds(
    trigger: Callable[[str], Path],
) -> None:
    trigger("pull_request")
    step = _step("post-pr-comment", BASE_STEP)
    base = _downloads("post-pr-comment")[BASE_STEP]
    # The shell expands $BASE_SHA; an empty --runs-cache skips the PR build's run snapshot.
    assert (base.repo, base.head_sha, base.event, base.runs_cache) == (
        "owner/repo",
        "$BASE_SHA",
        "push",
        "",
    )
    assert step["env"]["BASE_SHA"] == "${{ needs.report-context.outputs.base_sha }}"
    # Base test names, coverage, and sizes, without pre-commit or Code Analysis results.
    assert set(base.artifact_prefixes.split(",")) == {"coverage-report", "test-results-"}


def test_pr_downloads_never_keep_the_comment_from_updating() -> None:
    # A failed download is named in the comment (download-manifest.json) instead of leaving it
    # stale; download_artifacts.py skips runs until they complete, so no step needs a gate.
    downloads = [
        step for step in _steps("post-pr-comment") if "download_artifacts.py" in step.get("run", "")
    ]
    assert len(downloads) == 4
    for step in downloads:
        assert step["continue-on-error"] is True
        assert "if" not in step


def test_comment_reads_what_the_earlier_steps_write(trigger: Callable[[str], Path]) -> None:
    trigger("pull_request")
    paths = {key: Path(value) for key, value in generate_build_results_comment.INPUT_PATHS.items()}
    (collect,) = _script_args("post-pr-comment", "collect_build_status.py").values()
    assert collect_build_status.parse_args(collect).status_out == paths["BUILD_STATUS_JSON"]

    downloads = _downloads("post-pr-comment")
    platform = downloads["Download artifacts from all platform workflows"]
    precommit = downloads["Download pre-commit results"]
    analysis = downloads["Download Code Analysis results"]
    base = downloads[BASE_STEP]
    assert platform.output_dir == precommit.output_dir == paths["ARTIFACTS_DIR"]
    assert analysis.output_dir == paths["ANALYSIS_ARTIFACTS_DIR"]
    assert platform.sizes_out == paths["PR_SIZES_JSON"]
    assert "coverage-report" in platform.artifact_prefixes.split(",")
    assert paths["COVERAGE_SUMMARY"].parent == platform.output_dir / "coverage-report"
    # A single downloaded artifact lands in <output-dir>/<artifact name>/.
    assert paths["PRECOMMIT_RESULTS_PATH"].parent == precommit.output_dir / "pre-commit-results"
    assert precommit.artifact_prefixes == "pre-commit-results"
    assert base.output_dir == paths["BASE_ARTIFACTS_DIR"]
    assert base.sizes_out == paths["BASE_SIZES_JSON"]
    assert paths["BASE_COVERAGE_SUMMARY"].parent == base.output_dir / "coverage-report"

    generate = _step("post-pr-comment", "Generate combined report")
    assert generate["run"] == "python3 tools/ci/reporting/generate_build_results_comment.py"
    assert generate["env"] == {
        "BASE_SHA": "${{ needs.report-context.outputs.base_sha }}",
        "PR_NUMBER": "${{ needs.report-context.outputs.pr }}",
    }


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

    publish = shlex.split(_step("post-pr-comment", "Publish comment")["run"])
    assert publish[:3] == ["python3", "tools/ci/reporting/report_context.py", "--comment"]
    comment = (workspace / publish[3]).read_text(encoding="utf-8")
    assert f"/commit/{HEAD_SHA})" in comment
    assert "triggered by iOS" in comment
    assert gh_summary.read_text(encoding="utf-8") == comment


def test_jobs_run_only_for_pull_request_builds() -> None:
    jobs = load_workflow(WORKFLOW)["jobs"]
    assert set(jobs) == {"report-context", "post-pr-comment"}
    assert jobs["report-context"]["if"] == "github.event.workflow_run.event == 'pull_request'"
    assert jobs["report-context"]["outputs"] == {
        "pr": "${{ steps.context.outputs.pr }}",
        "base_sha": "${{ steps.context.outputs.base_sha }}",
    }
    # report_context.py sets `pr` only while the build's commit is an open PR's head
    # (tools/tests/ci/test_report_context.py).
    assert jobs["post-pr-comment"]["if"] == "needs.report-context.outputs.pr != ''"


def test_publishing_needs_only_the_jobs_comment_permissions() -> None:
    job = load_workflow(WORKFLOW)["jobs"]["post-pr-comment"]
    publish = _step("post-pr-comment", "Publish comment")
    assert publish["env"] == {"GH_TOKEN": "${{ github.token }}"}
    assert job["permissions"]["issues"] == job["permissions"]["pull-requests"] == "write"
    assert not any(
        "actions-comment-pull-request" in str(step.get("uses", "")) for step in job["steps"]
    )
