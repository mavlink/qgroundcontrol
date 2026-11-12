"""Pages workflow contracts: only upstream master builds feed the site, inside its time budget."""

from __future__ import annotations

import json
from typing import Any

import pytest
from ci.publish import pages_history, pages_site

from ..._helpers import REPO_ROOT, evaluate_expression, get_steps, load_workflow
from .._workflows import path_filter_covers, uses_action

WORKFLOW = "pages.yml"
REPO = "mavlink/qgroundcontrol"
SITE_INPUTS = {
    "src/FactSystem/factmetadata.schema.json",
    "tools/ci/publish/pages_history.py",
    "tools/ci/publish/pages_render.py",
    "tools/ci/publish/pages_site.py",
}


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


def test_site_follows_every_platform_build_and_never_a_pull_request() -> None:
    triggers = load_workflow(WORKFLOW)["on"]
    # Pull request artifacts must never reach the site; CI Scripts unit-tests the tooling.
    assert not {"pull_request", "pull_request_target"} & set(triggers)
    config = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
    platforms = {name.strip() for name in config["build"]["platform_workflows"].split(",")}
    assert platforms <= set(triggers["workflow_run"]["workflows"])


def test_push_paths_cover_the_site_inputs() -> None:
    patterns = load_workflow(WORKFLOW)["on"]["push"]["paths"]
    for path in SITE_INPUTS:
        assert path_filter_covers(path, patterns), path
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
    condition = load_workflow(WORKFLOW)["jobs"]["build"]["if"]
    assert bool(evaluate_expression(condition, context)) is expected


def test_every_successful_run_deployed_the_site() -> None:
    # pages_history.require_first_deploy reads a successful run as a deploy; that holds while
    # the deploy job runs after every successful build and no other job can succeed alone.
    assert pages_history.PAGES_WORKFLOW == WORKFLOW
    jobs = load_workflow(WORKFLOW)["jobs"]
    assert set(jobs) == {"build", "deploy"}
    assert jobs["deploy"]["needs"] == "build"


def test_downloads_end_well_inside_the_build_job() -> None:
    # Collection, then the API docs download at most; setup, the other pages, and the artifact
    # upload get the rest.
    minutes = load_workflow(WORKFLOW)["jobs"]["build"]["timeout-minutes"]
    budget = pages_history.COLLECT_SECONDS + pages_site.DOXYGEN_ATTEMPTS * (
        pages_site.DOXYGEN_TIMEOUT_SECONDS
    )
    assert budget <= (minutes - 10) * 60


def test_uploaded_site_keeps_the_dot_file_the_build_writes() -> None:
    # upload-pages-artifact leaves out dot files unless asked, and pages_site.py writes .nojekyll.
    steps = get_steps(load_workflow(WORKFLOW), "build")
    (upload,) = (step for step in steps if uses_action(step, "actions/upload-pages-artifact"))
    assert upload["with"]["include-hidden-files"] is True


def test_doxygen_uploads_the_artifact_the_site_downloads_and_keeps_it_past_a_week() -> None:
    # The Pages build takes /api/ from the latest master Doxygen run, which runs weekly.
    steps = get_steps(load_workflow("doxygen.yml"), "build")
    (upload,) = (step for step in steps if uses_action(step, "actions/upload-artifact"))
    assert upload["with"]["name"] == pages_site.DOXYGEN_ARTIFACT
    retention = upload["with"]["retention-days"]
    for event in ("schedule", "workflow_dispatch"):
        assert int(evaluate_expression(retention, {"github": {"event_name": event}})) > 7
