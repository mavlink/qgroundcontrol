"""Repository-wide security policy checks for GitHub Actions workflows."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING, Any

import yaml
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
WORKFLOWS = sorted(WORKFLOWS_DIR.glob("*.y*ml"))
COMPOSITE_ACTIONS = sorted((REPO_ROOT / ".github" / "actions").rglob("action.y*ml"))
CI_SCRIPTS_WORKFLOW = WORKFLOWS_DIR / "ci-scripts.yml"
DEPENDENCY_REVIEW_WORKFLOW = WORKFLOWS_DIR / "dependency-review.yml"
HARDEN_RUNNER = "step-security/harden-runner@v2"
RUNS_ON_ACTION = "runs-on/action@v2"
VERSIONED_ACTION_REF = re.compile(r"(?:[0-9a-f]{40}|v\d+(?:\.\d+){0,2})")
FORK_SAFETY_GUARDS = (
    "github.repository_owner == 'mavlink'",
    "github.event_name != 'pull_request'",
    "github.event.pull_request.head.repo.full_name == github.repository",
)


def _load_yaml_mapping(path: Path) -> dict[str, Any]:
    document = yaml.safe_load(path.read_text(encoding="utf-8"))
    assert isinstance(document, dict), f"{path} must contain a YAML mapping"
    if True in document and "on" not in document:
        document["on"] = document.pop(True)
    return document


def _executable_jobs(path: Path) -> Iterator[tuple[str, dict[str, Any]]]:
    jobs = _load_yaml_mapping(path).get("jobs", {})
    assert isinstance(jobs, dict), f"{path.name} jobs must be a mapping"
    for job_name, job in jobs.items():
        if isinstance(job, dict) and "steps" in job:
            yield job_name, job


def _uses_values(value: Any) -> Iterator[str]:
    if isinstance(value, dict):
        for key, child in value.items():
            if key == "uses" and isinstance(child, str):
                yield child
            else:
                yield from _uses_values(child)
    elif isinstance(value, list):
        for child in value:
            yield from _uses_values(child)


def test_ci_scripts_checks_every_workflow() -> None:
    workflow = _load_yaml_mapping(CI_SCRIPTS_WORKFLOW)
    steps = workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(step for step in steps if step.get("uses", "").startswith("actions/checkout@"))
    sparse_checkout = checkout["with"]["sparse-checkout"].splitlines()
    assert ".github/workflows" in sparse_checkout


def test_workflows_have_explicit_permissions() -> None:
    for path in WORKFLOWS:
        permissions = _load_yaml_mapping(path).get("permissions")
        assert isinstance(permissions, dict), (
            f"{path.name} must declare an explicit permission baseline"
        )


def test_executable_jobs_are_bounded_and_hardened() -> None:
    for path in WORKFLOWS:
        for job_name, job in _executable_jobs(path):
            steps = job["steps"]
            assert isinstance(steps, list) and steps, f"{path.name}:{job_name} must have steps"
            timeout = job.get("timeout-minutes")
            assert (isinstance(timeout, int) and not isinstance(timeout, bool) and timeout > 0) or (
                isinstance(timeout, str) and timeout.startswith("${{") and timeout.endswith("}}")
            ), f"{path.name}:{job_name} must set a positive timeout-minutes"
            assert steps[0].get("uses") == HARDEN_RUNNER, (
                f"{path.name}:{job_name} must run {HARDEN_RUNNER} before other steps"
            )


def test_workflow_checkouts_do_not_persist_credentials() -> None:
    for path in WORKFLOWS:
        for job_name, job in _executable_jobs(path):
            for step in job["steps"]:
                if not str(step.get("uses", "")).startswith("actions/checkout@"):
                    continue
                assert step.get("with", {}).get("persist-credentials") is False, (
                    f"{path.name}:{job_name} checkout must set persist-credentials: false"
                )


def test_runson_selection_is_fork_safe() -> None:
    for path in WORKFLOWS:
        for job_name, job in _executable_jobs(path):
            runner = str(job.get("runs-on", ""))
            if "runs-on=" in runner:
                assert all(guard in runner for guard in FORK_SAFETY_GUARDS), (
                    f"{path.name}:{job_name} must keep RunsOn selection fork-safe"
                )
                assert "||" in runner, (
                    f"{path.name}:{job_name} must provide a GitHub-hosted runner fallback"
                )

            for step in job["steps"]:
                if step.get("uses") != RUNS_ON_ACTION:
                    continue
                condition = str(step.get("if", ""))
                assert all(guard in condition for guard in FORK_SAFETY_GUARDS), (
                    f"{path.name}:{job_name} must not enable RunsOn for fork pull requests"
                )


def test_external_actions_use_versioned_refs() -> None:
    for path in [*WORKFLOWS, *COMPOSITE_ACTIONS]:
        document = _load_yaml_mapping(path)
        for uses in _uses_values(document):
            if uses.startswith("./"):
                continue
            action, separator, ref = uses.rpartition("@")
            assert separator and action, f"{path}: external action must include a ref: {uses}"
            assert VERSIONED_ACTION_REF.fullmatch(ref), (
                f"{path}: external action must use a numeric version tag or commit SHA: {uses}"
            )


def test_dependency_review_covers_every_pull_request() -> None:
    workflow = _load_yaml_mapping(DEPENDENCY_REVIEW_WORKFLOW)
    assert workflow["on"]["pull_request"] is None
    assert workflow["permissions"] == {"contents": "read", "pull-requests": "write"}

    steps = workflow["jobs"]["dependency-review"]["steps"]
    uses = {step.get("uses") for step in steps}
    assert "actions/dependency-review-action@v5" in uses
    assert "gradle/actions/wrapper-validation@v4" in uses
