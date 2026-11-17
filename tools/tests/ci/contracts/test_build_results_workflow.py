"""build-results.yml steps pass only what differs from their scripts' defaults.

The reporting scripts take the repository, commit, and event from the triggering workflow_run
payload, the platform workflows from build-config.json, and the run snapshot path from
RUNNER_TEMP, and the comment generator reads the files the earlier steps write at its default
paths. These tests replay each step's command line under a fake pull_request payload.
"""

from __future__ import annotations

import re
import shlex
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest
from ci.reporting import (
    analysis_review,
    collect_build_status,
    download_artifacts,
    generate_build_results_comment,
)
from ci.reporting.workflow_runs import configured_platform_workflows

from ..._helpers import REPO_ROOT, evaluate_expression, load_workflow

if TYPE_CHECKING:
    import argparse
    from collections.abc import Callable

WORKFLOW = "build-results.yml"
HEAD_SHA = "0123456789abcdef0123456789abcdef01234567"


def _steps(job: str) -> list[dict[str, Any]]:
    return load_workflow(WORKFLOW)["jobs"][job]["steps"]


def _script_args(job: str, script: str) -> list[list[str]]:
    """Arguments of each step of *job* that runs tools/ci/reporting/<script>."""
    found = []
    for step in _steps(job):
        argv = shlex.split(step.get("run", "").replace("\\\n", " "))
        if argv[:2] == ["python3", f"tools/ci/reporting/{script}"]:
            found.append(argv[2:])
    return found


def _downloads(job: str) -> list[argparse.Namespace]:
    return [
        download_artifacts.parse_args(argv) for argv in _script_args(job, "download_artifacts.py")
    ]


@pytest.fixture
def trigger(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, workflow_run_event: Callable[..., Path]
) -> Path:
    """Set up a build-results job triggered by a completed pull_request build; return RUNNER_TEMP."""
    workflow_run_event(head_sha=HEAD_SHA, event="pull_request", name="iOS", conclusion="success")
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    return tmp_path


@pytest.mark.parametrize(
    ("event", "starts"),
    [
        ("pull_request", True),
        ("push", False),
        ("schedule", False),
        ("workflow_dispatch", False),
        ("merge_group", False),
    ],
)
def test_only_a_pull_request_build_starts_a_job(event: str, starts: bool) -> None:
    """Any other build has no comment to update, so its run must end without using a runner."""
    jobs = load_workflow(WORKFLOW)["jobs"]
    context = {"github": {"event": {"workflow_run": {"event": event}}}}
    first = {name: job for name, job in jobs.items() if "needs" not in job}
    assert first
    for name, job in first.items():
        assert bool(evaluate_expression(job["if"], context)) is starts, name
    # A job needing a skipped job is skipped too, unless its condition calls a status function.
    for name in jobs.keys() - first.keys():
        condition = jobs[name].get("if", "")
        assert not re.search(r"\b(always|cancelled|failure|success)\(", condition), name


def test_comment_steps_query_the_triggering_pr_build_unless_they_name_another_commit(
    trigger: Path,
) -> None:
    (collect,) = _script_args("post-pr-comment", "collect_build_status.py")
    status = collect_build_status.parse_args(collect)
    assert status.platform_workflows == configured_platform_workflows()

    downloads = _downloads("post-pr-comment")
    other_commit = [args for args in downloads if args.head_sha != HEAD_SHA]
    assert other_commit
    for args in [status, *downloads]:
        if args in other_commit:
            # The run snapshot holds the triggering build's runs, not this commit's.
            assert args.runs_cache == ""
            continue
        assert (args.repo, args.head_sha, args.event) == ("owner/repo", HEAD_SHA, "pull_request")
        assert args.runs_cache == str(trigger / "workflow-runs-cache.json")


def test_comment_reads_what_the_earlier_steps_write(trigger: Path) -> None:
    paths = {key: Path(value) for key, value in generate_build_results_comment.INPUT_PATHS.items()}
    (collect,) = _script_args("post-pr-comment", "collect_build_status.py")
    assert collect_build_status.parse_args(collect).status_out == paths["BUILD_STATUS_JSON"]

    downloads = _downloads("post-pr-comment")
    assert {args.output_dir for args in downloads} == {
        path for key, path in paths.items() if key.endswith("ARTIFACTS_DIR")
    }
    assert {args.sizes_out for args in downloads} - {None} == {
        path for key, path in paths.items() if key.endswith("SIZES_JSON")
    }
    # A downloaded artifact lands in <output-dir>/<artifact name>/, where the comment reads it.
    artifact_files = {key: path for key, path in paths.items() if len(path.parts) == 3}
    assert artifact_files
    for key, path in artifact_files.items():
        directory, artifact = path.parent.parent, path.parent.name
        assert any(
            args.output_dir == directory and artifact in args.artifact_prefixes.split(",")
            for args in downloads
        ), key


def test_comment_defaults_to_the_triggering_build(
    trigger: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    workspace = trigger / "workspace"
    workspace.mkdir()
    monkeypatch.chdir(workspace)
    (workspace / "build-status.json").write_text(
        f'{{"repo": "owner/repo", "head_sha": "{HEAD_SHA}", "platforms": []}}', encoding="utf-8"
    )

    assert generate_build_results_comment.main([]) == 0

    # The Publish step posts the file the generator wrote.
    (publish,) = _script_args("post-pr-comment", "report_context.py")
    comment = (workspace / publish[publish.index("--comment") + 1]).read_text(encoding="utf-8")
    assert f"/commit/{HEAD_SHA})" in comment
    assert "triggered by iOS" in comment
    assert gh_summary.read_text(encoding="utf-8") == comment


def test_inline_analysis_review_runs_apart_from_the_comment_on_the_same_reports(
    trigger: Path,
) -> None:
    jobs = load_workflow(WORKFLOW)["jobs"]
    review = jobs["analysis-review"]
    # A newer run queueing in a concurrency group cancels the one pending there, so a review
    # waiting behind a comment job would be lost.
    assert review["concurrency"]["group"] != jobs["post-pr-comment"]["concurrency"]["group"]
    # analysis_review.py posts nothing for any other workflow's run.
    gate = f"github.event.workflow_run.path == '{analysis_review.ANALYSIS_WORKFLOW}'"
    assert gate in " ".join(review["if"].split())
    assert (REPO_ROOT / analysis_review.ANALYSIS_WORKFLOW).is_file()

    # It reviews the reports the comment reads, downloaded the same way to the script's default
    # directory but only from the run that triggered it, whose link and conclusion it reports.
    (download,) = _downloads("analysis-review")
    assert download.output_dir == analysis_review.DEFAULT_ANALYSIS_DIR
    (comment_download,) = (
        args for args in _downloads("post-pr-comment") if args.output_dir == download.output_dir
    )
    assert not comment_download.only_triggering_run
    assert vars(download) == {**vars(comment_download), "only_triggering_run": True}
