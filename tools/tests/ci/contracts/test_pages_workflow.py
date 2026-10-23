"""Pages workflow contracts: master-only triggers and gates, job permissions, and deploy setup."""

from __future__ import annotations

import fnmatch
import json
from typing import Any

import pytest
from ci.publish import pages_history, pages_site

from ..._helpers import (
    REPO_ROOT,
    evaluate_expression,
    get_step,
    load_workflow,
    load_yaml,
    workflow_names,
)

WORKFLOW = "pages.yml"
REPO = "mavlink/qgroundcontrol"
SITE_INPUTS = {
    "src/FactSystem/factmetadata.schema.json",
    "tools/ci/publish/pages_history.py",
    "tools/ci/publish/pages_render.py",
    "tools/ci/publish/pages_site.py",
}


def _platform_workflows() -> list[str]:
    config = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
    return [name.strip() for name in config["build"]["platform_workflows"].split(",")]


def _context(
    event_name: str,
    *,
    workflow: str = "Linux",
    run_event: str = "push",
    branch: str = "master",
    head_repo: str = REPO,
    repo: str = REPO,
) -> dict[str, Any]:
    run = {
        "name": workflow,
        "event": run_event,
        "head_branch": branch,
        "head_repository": {"full_name": head_repo},
    }
    event = {"workflow_run": run} if event_name == "workflow_run" else {}
    return {"github": {"repository": repo, "event_name": event_name, "event": event}}


def _builds(context: dict[str, Any]) -> bool:
    return bool(evaluate_expression(load_workflow(WORKFLOW)["jobs"]["build"]["if"], context))


def test_triggers_never_include_pull_requests() -> None:
    triggers = load_workflow(WORKFLOW)["on"]
    # Pull request artifacts must never reach the site; CI Scripts unit-tests the tooling.
    assert set(triggers) == {"workflow_run", "push", "schedule", "workflow_dispatch"}


def test_workflow_run_follows_master_builds_by_their_names() -> None:
    trigger = load_workflow(WORKFLOW)["on"]["workflow_run"]
    expected = [*_platform_workflows(), "Code Analysis", "Doxygen API Docs"]
    assert trigger["workflows"] == expected
    assert trigger["types"] == ["completed"]
    assert trigger["branches"] == ["master"]
    names = {
        load_yaml(REPO_ROOT / ".github/workflows" / name).get("name") for name in workflow_names()
    }
    assert set(expected) <= names


def test_push_paths_cover_the_site_inputs() -> None:
    push = load_workflow(WORKFLOW)["on"]["push"]
    assert push["branches"] == ["master"]
    patterns = push["paths"]
    assert ".github/workflows/pages.yml" in patterns
    for path in SITE_INPUTS:
        assert any(fnmatch.fnmatch(path, pattern) for pattern in patterns), path
    for pattern in patterns:
        assert list(REPO_ROOT.glob(pattern)), f"{pattern} matches no file"


@pytest.mark.parametrize(
    ("context", "expected"),
    [
        (_context("workflow_run"), True),
        (_context("workflow_run", workflow="Code Analysis"), True),
        (_context("workflow_run", workflow="Doxygen API Docs", run_event="schedule"), True),
        (
            _context("workflow_run", workflow="Doxygen API Docs", run_event="workflow_dispatch"),
            True,
        ),
        (_context("push"), True),
        (_context("schedule"), True),
        (_context("workflow_dispatch"), True),
        # A fork's pull request from its own master branch.
        (
            _context("workflow_run", run_event="pull_request", head_repo="someone/qgroundcontrol"),
            False,
        ),
        (_context("workflow_run", run_event="pull_request"), False),
        (_context("workflow_run", workflow="Doxygen API Docs", run_event="pull_request"), False),
        (_context("workflow_run", run_event="workflow_dispatch"), False),
        (_context("workflow_run", branch="feature"), False),
        (_context("workflow_run", head_repo="someone/qgroundcontrol"), False),
        (_context("push", repo="someone/qgroundcontrol"), False),
        (_context("workflow_dispatch", repo="someone/qgroundcontrol"), False),
    ],
)
def test_build_runs_only_for_upstream_master_builds(
    context: dict[str, Any], expected: bool
) -> None:
    assert _builds(context) is expected


def test_build_gate_names_the_triggering_run_event_and_branch() -> None:
    condition = load_workflow(WORKFLOW)["jobs"]["build"]["if"]
    assert "github.event.workflow_run.event == 'push'" in condition
    assert "github.event.workflow_run.head_branch == 'master'" in condition
    assert "github.repository == 'mavlink/qgroundcontrol'" in condition


def test_deploys_are_serialized_and_never_cancelled() -> None:
    workflow = load_workflow(WORKFLOW)
    assert workflow["concurrency"] == {"group": "pages", "cancel-in-progress": False}
    assert workflow["permissions"] == {"contents": "read"}


def test_build_job_reads_only_and_builds_the_site() -> None:
    job = load_workflow(WORKFLOW)["jobs"]["build"]
    assert job["permissions"] == {"contents": "read", "actions": "read"}
    steps = job["steps"]
    assert steps[0]["uses"] == "step-security/harden-runner@v2"
    checkout = get_step(steps, "Checkout")
    assert checkout["with"]["persist-credentials"] is False
    assert "ref" not in checkout["with"]
    assert set(checkout["with"]["sparse-checkout"].split()) >= SITE_INPUTS
    build = get_step(steps, "Build site")
    assert build["run"] == "python3 tools/ci/publish/pages_site.py build --out _site"
    assert build["env"] == {
        "GH_TOKEN": "${{ github.token }}",
        "PAGES_NEW_HISTORY": "${{ inputs.new_history }}",
    }
    upload = get_step(steps, "Upload Pages artifact")
    assert upload["uses"] == "actions/upload-pages-artifact@v5"
    # upload-pages-artifact leaves out dot files unless asked, and .nojekyll is one.
    assert upload["with"] == {"path": "_site", "include-hidden-files": True}


def test_deploy_job_only_deploys() -> None:
    job = load_workflow(WORKFLOW)["jobs"]["deploy"]
    assert job["needs"] == "build"
    assert job["if"] == "github.repository == 'mavlink/qgroundcontrol'"
    assert job["permissions"] == {"pages": "write", "id-token": "write"}
    assert job["environment"] == {
        "name": "github-pages",
        "url": "${{ steps.deployment.outputs.page_url }}",
    }
    steps = job["steps"]
    assert steps[0]["uses"] == "step-security/harden-runner@v2"
    assert [step.get("uses") for step in steps[1:]] == ["actions/deploy-pages@v5"]
    assert steps[1]["id"] == "deployment"


def test_every_successful_run_deployed_the_site() -> None:
    # pages_history.require_first_deploy reads a successful run as a deploy; that holds while
    # the deploy job runs after every successful build and no other job can succeed alone.
    assert pages_history.PAGES_WORKFLOW == WORKFLOW
    jobs = load_workflow(WORKFLOW)["jobs"]
    assert set(jobs) == {"build", "deploy"}
    assert jobs["deploy"]["needs"] == "build"


def test_dispatch_can_start_a_new_history() -> None:
    inputs = load_workflow(WORKFLOW)["on"]["workflow_dispatch"]["inputs"]
    assert set(inputs) == {"new_history"}
    assert inputs["new_history"]["type"] == "boolean"
    assert inputs["new_history"]["default"] is False


def test_downloads_end_well_inside_the_build_job() -> None:
    # Collection, then the API docs download at most; setup, the other pages, and the artifact
    # upload get the rest.
    minutes = load_workflow(WORKFLOW)["jobs"]["build"]["timeout-minutes"]
    budget = pages_history.COLLECT_SECONDS + pages_site.DOXYGEN_ATTEMPTS * (
        pages_site.DOXYGEN_TIMEOUT_SECONDS
    )
    assert budget <= (minutes - 10) * 60


@pytest.mark.parametrize(
    ("event", "days"), [("pull_request", "1"), ("schedule", "8"), ("workflow_dispatch", "8")]
)
def test_doxygen_artifact_outlives_the_weekly_schedule(event: str, days: str) -> None:
    # The Pages build takes /api/ from the latest master Doxygen run, which runs weekly.
    upload = get_step(load_workflow("doxygen.yml")["jobs"]["build"]["steps"], "Upload artifact")
    assert upload["with"]["name"] == "doxygen_build"
    retention = upload["with"]["retention-days"]
    assert evaluate_expression(retention, {"github": {"event_name": event}}) == days
