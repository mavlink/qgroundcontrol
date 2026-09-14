"""Detect drift between platform workflow display names and the places that
match against them by literal string.

GitHub provides no expression-based way to wait for workflows by file name,
so consumers (`release.yml`'s `int128/wait-for-workflows-action`,
`build-results.yml`'s `workflow_run.workflows`) match by `name:`. A rename
of any platform workflow silently breaks those waits — release would proceed
with missing artifacts. These tests fail CI loudly when names drift.
"""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
import yaml
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
BUILD_CONFIG_JSON = REPO_ROOT / ".github" / "build-config.json"
RELEASE_YML = WORKFLOWS_DIR / "release.yml"
BUILD_RESULTS_YML = WORKFLOWS_DIR / "build-results.yml"

# Non-platform workflows that legitimately trigger build-results.yml.
EXTRA_TRIGGERS: frozenset[str] = frozenset({"pre-commit"})

PLATFORM_FILES: dict[str, str] = {
    "Linux": "linux.yml",
    "Windows": "windows.yml",
    "MacOS": "macos.yml",
    "Android": "android.yml",
}

# iOS produces release artifacts but is intentionally excluded from the
# build-results workflow's regular PR platform set.
RELEASE_PLATFORM_FILES: dict[str, str] = PLATFORM_FILES | {"iOS": "ios.yml"}


def _workflow_name(path: Path) -> str:
    return yaml.safe_load(path.read_text(encoding="utf-8")).get("name", "")


def _platform_workflows() -> list[str]:
    raw = (
        json.loads(BUILD_CONFIG_JSON.read_text(encoding="utf-8"))
        .get("build", {})
        .get("platform_workflows", "")
    )
    assert raw, "platform_workflows missing from build-config.json"
    return [n.strip() for n in raw.split(",") if n.strip()]


def test_workflow_file_name_matches_build_config() -> None:
    if not BUILD_CONFIG_JSON.exists():
        pytest.skip("build-config.json not in checkout")
    configured = _platform_workflows()
    assert sorted(configured) == sorted(PLATFORM_FILES), (
        f"build-config.json platform_workflows {configured} no longer match "
        f"the known file mapping {sorted(PLATFORM_FILES)} — update this test."
    )
    for expected_name, filename in PLATFORM_FILES.items():
        path = WORKFLOWS_DIR / filename
        if not path.exists():
            pytest.skip(f"{filename} not in checkout")
        actual = _workflow_name(path)
        assert actual == expected_name, (
            f"{filename} `name:` is {actual!r}; expected {expected_name!r}. "
            "Renaming silently breaks release.yml + build-results.yml workflow_run."
        )


def test_build_results_trigger_matches_build_config() -> None:
    if not BUILD_RESULTS_YML.exists() or not BUILD_CONFIG_JSON.exists():
        pytest.skip("build-results.yml or build-config.json not in checkout")
    with BUILD_RESULTS_YML.open(encoding="utf-8") as fh:
        # PyYAML parses the `on` key as the boolean True.
        doc = yaml.safe_load(fh)
    on = doc.get("on") or doc.get(True)
    assert isinstance(on, dict), "build-results.yml `on:` is not a mapping"
    workflow_run = on.get("workflow_run")
    assert isinstance(workflow_run, dict), "missing workflow_run trigger"
    workflows = workflow_run.get("workflows")
    assert isinstance(workflows, list), "workflow_run.workflows must be a list"

    trigger = {str(w) for w in workflows}
    platforms = set(_platform_workflows())
    missing = platforms - trigger
    extra = trigger - platforms - EXTRA_TRIGGERS

    msg_parts = []
    if missing:
        msg_parts.append(
            f"platform_workflows in build-config.json but not triggering build-results.yml: {sorted(missing)}"
        )
    if extra:
        msg_parts.append(
            f"workflow_run.workflows entries not in platform_workflows nor EXTRA_TRIGGERS: {sorted(extra)}"
        )
    if msg_parts:
        pytest.fail(" / ".join(msg_parts))


def test_build_results_pr_number_uses_plain_string_output() -> None:
    if not BUILD_RESULTS_YML.exists():
        pytest.skip("build-results.yml not in checkout")
    doc = yaml.safe_load(BUILD_RESULTS_YML.read_text(encoding="utf-8"))
    steps = doc["jobs"]["post-pr-comment"]["steps"]
    get_pr = next(step for step in steps if step.get("name") == "Get PR number")
    assert "needs.load-config.outputs.pr" in get_pr["env"]["PR_NUMBER"]
    assert '"result=$PR_NUMBER"' in get_pr["run"]
    report = next(step for step in steps if step.get("name") == "Generate combined report")
    assert report["env"]["BASELINE_SHA"] == "${{ steps.baseline.outputs.sha }}"


def test_only_pr_reporting_allows_missing_diagnostic_artifacts() -> None:
    doc = yaml.safe_load(BUILD_RESULTS_YML.read_text(encoding="utf-8"))
    for job, allows_missing in (("post-pr-comment", "true"), ("save-baselines", "false")):
        download = next(
            step
            for step in doc["jobs"][job]["steps"]
            if step.get("uses") == "./.github/actions/download-all-artifacts"
        )
        assert download["with"].get("allow-missing", "false") == allows_missing
        sizes = next(
            step
            for step in doc["jobs"][job]["steps"]
            if step.get("uses") == "./.github/actions/collect-artifact-sizes"
        )
        assert sizes["with"].get("require-artifacts", "false") == (
            "true" if job == "save-baselines" else "false"
        )
    action = yaml.safe_load(
        (REPO_ROOT / ".github/actions/download-all-artifacts/action.yml").read_text(
            encoding="utf-8"
        )
    )
    assert action["inputs"]["allow-missing"]["default"] == "false"
    assert "--allow-missing" in action["runs"]["steps"][0]["run"]


def test_release_wait_for_builds_lists_match_platforms() -> None:
    if not RELEASE_YML.exists() or not BUILD_CONFIG_JSON.exists():
        pytest.skip("release.yml or build-config.json not in checkout")
    doc = yaml.safe_load(RELEASE_YML.read_text(encoding="utf-8"))
    jobs = doc.get("jobs") or {}
    wait = jobs.get("wait-for-builds") or {}
    steps = wait.get("steps") or []
    from release_builds import WORKFLOWS

    assert WORKFLOWS == RELEASE_PLATFORM_FILES
    dispatch = next(
        step for step in steps if step.get("name") == "Dispatch and wait for release builds"
    )
    assert "release_builds.py" in dispatch["run"]
    assert '--sha "$GITHUB_SHA"' in dispatch["run"]
    downloads = jobs["upload-artifacts"]["steps"]
    download = next(step for step in downloads if step.get("name") == "Download artifacts")
    assert download["with"]["runs-file"] == "release-build-runs.json"
    assert download["with"]["strict-runs"] == "true"
    assert download["with"].get("allow-missing", "false") == "false"
