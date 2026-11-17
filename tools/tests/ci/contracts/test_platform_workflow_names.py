"""Detect drift between workflow display names and the places that match them by literal string.

GitHub provides no expression-based way to wait for workflows by file name, so `workflow_run`
triggers and `build-config.json`'s platform list match by `name:`. Renaming a workflow silently
stops its build-results reporting and its Pages records. These tests fail CI loudly instead.
"""

from __future__ import annotations

import json

from ci.reporting.collect_build_status import ANALYSIS_WORKFLOW, PRECOMMIT_WORKFLOW

from ..._helpers import REPO_ROOT, load_workflow, workflow_names


def _workflows() -> dict[str, dict]:
    """Every workflow by its display name."""
    return {
        str(document.get("name", "")): document for document in map(load_workflow, workflow_names())
    }


def _platform_workflows() -> list[str]:
    config = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
    raw = config["build"]["platform_workflows"]
    return [name.strip() for name in raw.split(",") if name.strip()]


def test_build_config_platform_workflows_name_existing_workflows() -> None:
    configured = _platform_workflows()
    assert configured
    assert set(configured) <= set(_workflows())


def test_workflow_run_triggers_name_existing_workflows() -> None:
    workflows = _workflows()
    followed = {
        name: document["on"]["workflow_run"]["workflows"]
        for name, document in workflows.items()
        if isinstance(document["on"], dict) and "workflow_run" in document["on"]
    }
    assert followed
    for name, names in followed.items():
        assert set(names) <= set(workflows), name


# Pull-request workflows that do not trigger build-results.yml, with the trigger that refreshes
# their row in the comment. In two weeks of upstream runs each ended within about a minute and
# was never the last check of its commit (.github/ci-notes/reporting.md); add a workflow only
# with the same evidence.
REFRESHED_BY = {
    "Check Links": "CodeQL",
    "Dependency Review": "CodeQL",
    "Docs": "CodeQL",
}


def _pull_request_filters(document: dict) -> dict | None:
    """The settings of a workflow's pull_request trigger, or None without that trigger."""
    triggers = document["on"]
    if isinstance(triggers, str):
        triggers = [triggers]
    if "pull_request" not in triggers:
        return None
    return (triggers["pull_request"] if isinstance(triggers, dict) else None) or {}


def test_build_results_triggers_on_every_pull_request_workflow() -> None:
    """A check left out could fail after the final comment and never show.

    The exceptions are in REFRESHED_BY, each with a trigger that runs on every pull request.
    """
    trigger = set(load_workflow("build-results.yml")["on"]["workflow_run"]["workflows"])
    pull_request = {
        name: filters
        for name, document in _workflows().items()
        if (filters := _pull_request_filters(document)) is not None
    }
    # The comment renders these workflows' artifacts, so each one's completion must update it.
    assert {*_platform_workflows(), PRECOMMIT_WORKFLOW, ANALYSIS_WORKFLOW} <= trigger
    assert not trigger & REFRESHED_BY.keys()
    reported = trigger | REFRESHED_BY.keys()
    assert reported == pull_request.keys(), (
        f"neither a trigger nor in REFRESHED_BY: {sorted(pull_request.keys() - reported)}; "
        f"not a pull_request workflow: {sorted(reported - pull_request.keys())}"
    )
    for name, refresher in REFRESHED_BY.items():
        assert refresher in trigger, name
        # A path, branch, or type filter could skip the refresher on a pull request that
        # runs the workflow it stands in for.
        assert not pull_request[refresher], refresher
