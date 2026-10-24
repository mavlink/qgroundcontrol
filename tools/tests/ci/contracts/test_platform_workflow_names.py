"""Detect drift between platform workflow display names and the places that
match against them by literal string.

GitHub provides no expression-based way to wait for workflows by file name,
so `build-results.yml`'s `workflow_run.workflows` matches by `name:`. A rename
of any platform workflow silently stops its build-results reporting. These
tests fail CI loudly when names drift.
"""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

from ..._helpers import REPO_ROOT, load_workflow, load_yaml

if TYPE_CHECKING:
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
BUILD_CONFIG_JSON = REPO_ROOT / ".github" / "build-config.json"

PLATFORM_FILES: dict[str, str] = {
    "Linux": "linux.yml",
    "Windows": "windows.yml",
    "MacOS": "macos.yml",
    "Android": "android.yml",
    "iOS": "ios.yml",
}


def _workflow_name(path: Path) -> str:
    return load_yaml(path).get("name", "")


def _pull_request_workflows() -> set[str]:
    """Display names of every workflow a pull_request event runs; the comment lists them all."""
    names = set()
    for path in WORKFLOWS_DIR.glob("*.yml"):
        doc = load_yaml(path)
        on = doc.get("on")
        if (isinstance(on, dict) and "pull_request" in on) or on == "pull_request":
            names.add(str(doc.get("name", "")))
    return names


def _platform_workflows() -> list[str]:
    raw = (
        json.loads(BUILD_CONFIG_JSON.read_text(encoding="utf-8"))
        .get("build", {})
        .get("platform_workflows", "")
    )
    assert raw, "platform_workflows missing from build-config.json"
    return [n.strip() for n in raw.split(",") if n.strip()]


def test_workflow_file_name_matches_build_config() -> None:
    configured = _platform_workflows()
    assert sorted(configured) == sorted(PLATFORM_FILES), (
        f"build-config.json platform_workflows {configured} no longer match "
        f"the known file mapping {sorted(PLATFORM_FILES)} — update this test."
    )
    for expected_name, filename in PLATFORM_FILES.items():
        actual = _workflow_name(WORKFLOWS_DIR / filename)
        assert actual == expected_name, (
            f"{filename} `name:` is {actual!r}; expected {expected_name!r}. "
            "Renaming silently breaks build-results.yml's workflow_run trigger."
        )


def test_build_results_triggers_on_every_pull_request_workflow() -> None:
    """A check left out of the trigger list could fail after the final comment and never show."""
    on = load_workflow("build-results.yml").get("on")
    assert isinstance(on, dict), "build-results.yml `on:` is not a mapping"
    workflow_run = on.get("workflow_run")
    assert isinstance(workflow_run, dict), "missing workflow_run trigger"
    workflows = workflow_run.get("workflows")
    assert isinstance(workflows, list), "workflow_run.workflows must be a list"

    trigger = {str(w) for w in workflows}
    expected = _pull_request_workflows()
    assert set(_platform_workflows()) <= expected
    assert {"pre-commit", "Code Analysis", "CI Scripts"} <= expected
    assert sorted(trigger) == sorted(expected), (
        f"not triggering build-results.yml: {sorted(expected - trigger)}; "
        f"not a pull_request workflow: {sorted(trigger - expected)}"
    )


def test_build_results_reuses_the_resolved_pr_context() -> None:
    doc = load_workflow("build-results.yml")
    outputs = doc["jobs"]["report-context"]["outputs"]
    assert outputs["base_sha"] == "${{ steps.context.outputs.base_sha }}"
    steps = doc["jobs"]["post-pr-comment"]["steps"]
    report = next(step for step in steps if step.get("name") == "Generate combined report")
    assert report["env"]["BASE_SHA"] == "${{ needs.report-context.outputs.base_sha }}"
    assert report["env"]["PR_NUMBER"] == "${{ needs.report-context.outputs.pr }}"
