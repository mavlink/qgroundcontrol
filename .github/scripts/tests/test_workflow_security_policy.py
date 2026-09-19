"""Repository-wide security policy checks for GitHub Actions workflows."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING, Any

import pytest
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
VM_BUILDS_WORKFLOW = WORKFLOWS_DIR / "vm-builds.yml"
BUILD_ACTION = REPO_ROOT / ".github" / "actions" / "build-action" / "action.yml"
HARDEN_RUNNER = "step-security/harden-runner@v2"
VERSIONED_ACTION_REF = re.compile(r"(?:[0-9a-f]{40}|v\d+(?:\.\d+){0,2})")


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


def _runson_routes(value: Any) -> Iterator[str]:
    if isinstance(value, dict):
        for key, child in value.items():
            if key in ("runs-on", "runs_on") and isinstance(child, str) and "runs-on=" in child:
                yield child
            else:
                yield from _runson_routes(child)
    elif isinstance(value, list):
        for child in value:
            yield from _runson_routes(child)


def test_ci_scripts_checks_every_workflow() -> None:
    workflow = _load_yaml_mapping(CI_SCRIPTS_WORKFLOW)
    steps = workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(step for step in steps if step.get("uses", "").startswith("actions/checkout@"))
    sparse_checkout = checkout["with"]["sparse-checkout"].splitlines()
    assert ".github" in sparse_checkout or ".github/workflows" in sparse_checkout


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


def test_runson_selection_keeps_independent_forks_on_hosted_runners() -> None:
    for path in WORKFLOWS:
        for job_name, job in _executable_jobs(path):
            for runner in _runson_routes(job):
                normalized = " ".join(runner.removeprefix("${{").removesuffix("}}").split())
                assert normalized.startswith("github.repository_owner == 'mavlink' && "), (
                    f"{path.name}:{job_name} must limit RunsOn selection to upstream workflows"
                )
                assert "||" in runner, (
                    f"{path.name}:{job_name} must provide a GitHub-hosted runner fallback"
                )


@pytest.mark.parametrize(
    ("workflow", "route_count"),
    [
        ("linux.yml", 3),
        ("windows.yml", 2),
        ("android.yml", 1),
        ("docker.yml", 1),
        ("custom-build.yml", 1),
        ("vm-builds.yml", 2),
    ],
)
def test_upstream_runson_routes_do_not_restrict_pr_origin(workflow: str, route_count: int) -> None:
    routes = list(_runson_routes(_load_yaml_mapping(WORKFLOWS_DIR / workflow)["jobs"]))
    assert len(routes) == route_count
    for route in routes:
        assert "github.event" not in route
        assert "github.actor" not in route


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


def test_build_action_uses_lockfile_aware_npm_install() -> None:
    steps = _load_yaml_mapping(BUILD_ACTION)["runs"]["steps"]
    build_script = next(step["run"] for step in steps if step.get("name", "").startswith("Build "))

    assert "npm ci || npm install" not in build_script
    assert "[[ -f package-lock.json || -f npm-shrinkwrap.json ]]" in build_script
    assert "npm ci" in build_script
    assert "npm install" in build_script


def test_vm_build_hashicorp_key_download_is_bounded() -> None:
    steps = _load_yaml_mapping(VM_BUILDS_WORKFLOW)["jobs"]["vagrant-build"]["steps"]
    install_script = next(
        step["run"] for step in steps if step.get("name") == "Install Vagrant + libvirt"
    )

    assert install_script.startswith("set -o pipefail\n")
    assert "wget " not in install_script
    for option in (
        "--fail",
        "--retry 5",
        "--retry-delay 2",
        "--retry-max-time 120",
        "--connect-timeout 10",
        "--max-time 30",
    ):
        assert option in install_script
