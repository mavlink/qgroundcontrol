#!/usr/bin/env python3
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


def _workflow_name(path: Path) -> str:
    return yaml.safe_load(path.read_text(encoding="utf-8")).get("name", "")


def _platform_workflows() -> list[str]:
    raw = json.loads(BUILD_CONFIG_JSON.read_text(encoding="utf-8")).get("build", {}).get(
        "platform_workflows", ""
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


def test_release_wait_for_builds_lists_match_platforms() -> None:
    if not RELEASE_YML.exists() or not BUILD_CONFIG_JSON.exists():
        pytest.skip("release.yml or build-config.json not in checkout")
    doc = yaml.safe_load(RELEASE_YML.read_text(encoding="utf-8"))
    jobs = doc.get("jobs") or {}
    wait = jobs.get("wait-for-builds") or {}
    steps = wait.get("steps") or []
    names_block: str | None = None
    for step in steps:
        with_ = step.get("with") or {}
        if "workflow-names" in with_:
            names_block = str(with_["workflow-names"])
            break
    assert names_block is not None, "wait-for-builds step missing workflow-names"
    listed = {line.strip() for line in names_block.splitlines() if line.strip()}
    expected = set(_platform_workflows())
    assert listed == expected, (
        f"release.yml wait-for-builds workflow-names {sorted(listed)} != "
        f"build-config.json platform_workflows {sorted(expected)}"
    )
