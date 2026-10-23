"""Repository-wide security policy checks for GitHub Actions workflows."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING, Any

import pytest

from ..._helpers import REPO_ROOT, action_names, load_yaml, workflow_names

if TYPE_CHECKING:
    from collections.abc import Iterator
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
WORKFLOWS = [WORKFLOWS_DIR / name for name in workflow_names()]
COMPOSITE_ACTIONS = [REPO_ROOT / f".github/actions/{name}/action.yml" for name in action_names()]
CI_SCRIPTS_WORKFLOW = WORKFLOWS_DIR / "ci-scripts.yml"
DEPENDENCY_REVIEW_WORKFLOW = WORKFLOWS_DIR / "dependency-review.yml"
BUILD_ACTION = REPO_ROOT / ".github" / "actions" / "build-action" / "action.yml"
HARDEN_RUNNER = "step-security/harden-runner@v2"
VERSIONED_ACTION_REF = re.compile(r"(?:[0-9a-f]{40}|v\d+(?:\.\d+){0,2})")


def _load_yaml_mapping(path: Path) -> dict[str, Any]:
    document = load_yaml(path)
    assert isinstance(document, dict), f"{path} must contain a YAML mapping"
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


def _runs_only_on_runson(job: dict[str, Any]) -> bool:
    """A RunsOn label with no GitHub-hosted fallback, where Harden-Runner could never run."""
    route = job.get("runs-on")
    return isinstance(route, str) and "runs-on=" in route and "||" not in route


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
            if _runs_only_on_runson(job):
                continue
            assert steps[0].get("uses") == HARDEN_RUNNER, (
                f"{path.name}:{job_name} must run {HARDEN_RUNNER} before other steps"
            )


def test_harden_runner_audits_only_github_hosted_runners() -> None:
    # The community tier cannot audit self-hosted runners; RunsOn would need StepSecurity Enterprise.
    hosted_only = "runner.environment == 'github-hosted'"
    for path in WORKFLOWS:
        for job_name, job in _executable_jobs(path):
            if _runs_only_on_runson(job):
                continue
            condition = str(job["steps"][0].get("if", ""))
            if any(_runson_routes(job)):
                assert hosted_only in condition, f"{path.name}:{job_name} may run on RunsOn"
            else:
                assert "runner.environment" not in condition, f"{path.name}:{job_name}"


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
    upstream = "github.repository_owner == 'mavlink'"
    for path in WORKFLOWS:
        jobs = _load_yaml_mapping(path)["jobs"]
        for job_name, job in _executable_jobs(path):
            if _runs_only_on_runson(job):
                needs = job.get("needs", [])
                needs = [needs] if isinstance(needs, str) else needs
                assert any(upstream in str(jobs[need].get("if", "")) for need in needs), (
                    f"{path.name}:{job_name} must depend on a job limited to upstream workflows"
                )
                continue
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
        ("windows.yml", 1),
        ("android.yml", 1),
        ("docker.yml", 1),
        ("vm-builds.yml", 2),
        ("analysis.yml", 1),
    ],
)
def test_upstream_runson_routes_do_not_restrict_pr_origin(workflow: str, route_count: int) -> None:
    routes = list(_runson_routes(_load_yaml_mapping(WORKFLOWS_DIR / workflow)["jobs"]))
    assert len(routes) == route_count
    for route in routes:
        assert "github.event." not in route
        assert "github.actor" not in route
        # The event type never decides whether or how RunsOn is used.
        assert "github.event_name" not in route


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
    assert "gradle/actions/wrapper-validation@v6" in uses


def test_pr_title_check_only_reads_the_pull_request() -> None:
    # `wip` would post a commit status, which needs statuses: write; its permission error was
    # raised before the title error, so a bad title went unreported.
    job = _load_yaml_mapping(WORKFLOWS_DIR / "pr-checks.yml")["jobs"]["title"]
    assert job["permissions"] == {"pull-requests": "read"}
    (check,) = (step for step in job["steps"] if step.get("name") == "Check PR title format")
    assert check["uses"].startswith("amannn/action-semantic-pull-request@")
    assert "wip" not in check["with"]


def test_build_action_uses_lockfile_aware_npm_install() -> None:
    steps = _load_yaml_mapping(BUILD_ACTION)["runs"]["steps"]
    build_script = next(step["run"] for step in steps if step.get("name", "").startswith("Build "))

    assert "npm ci || npm install" not in build_script
    assert "[[ -f package-lock.json || -f npm-shrinkwrap.json ]]" in build_script
    assert "npm ci" in build_script
    assert "npm install" in build_script
