#!/usr/bin/env python3
"""Detect drift between build-results.yml's workflow_run.workflows trigger and
build-config.json's platform_workflows.

GitHub requires literal workflow names in `workflow_run.workflows` (no
expressions), so we can't derive the trigger list dynamically — but we can
fail CI when the two diverge.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest
import yaml

REPO_ROOT = Path(__file__).resolve().parents[3]
BUILD_RESULTS_YML = REPO_ROOT / ".github" / "workflows" / "build-results.yml"
BUILD_CONFIG_JSON = REPO_ROOT / ".github" / "build-config.json"

# Non-platform workflows that legitimately trigger build-results.yml.
EXTRA_TRIGGERS: frozenset[str] = frozenset({"pre-commit"})

def _load_trigger_workflows() -> list[str]:
    with BUILD_RESULTS_YML.open(encoding="utf-8") as fh:
        # PyYAML parses the `on` key as the boolean True; load raw to sidestep.
        doc = yaml.safe_load(fh)
    on = doc.get("on") or doc.get(True)
    assert isinstance(on, dict), "build-results.yml `on:` is not a mapping"
    workflow_run = on.get("workflow_run")
    assert isinstance(workflow_run, dict), "missing workflow_run trigger"
    workflows = workflow_run.get("workflows")
    assert isinstance(workflows, list), "workflow_run.workflows must be a list"
    return [str(w) for w in workflows]

def _load_platform_workflows() -> list[str]:
    config = json.loads(BUILD_CONFIG_JSON.read_text(encoding="utf-8"))
    raw = config.get("platform_workflows", "")
    assert raw, "platform_workflows missing from build-config.json"
    return [name.strip() for name in raw.split(",") if name.strip()]

def test_workflow_run_trigger_matches_build_config() -> None:
    # Skip when the consumer is checked out without these files (sparse-checkout
    # variants, partial clones); the corresponding sparse-checkout list owns
    # ensuring availability in CI.
    if not BUILD_RESULTS_YML.exists() or not BUILD_CONFIG_JSON.exists():
        pytest.skip("build-results.yml or build-config.json not in checkout")

    trigger = set(_load_trigger_workflows())
    platforms = set(_load_platform_workflows())

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

BOOTSTRAP_ACTION_YML = (
    REPO_ROOT / ".github" / "actions" / "build-results-bootstrap" / "action.yml"
)

EXPECTED_BOOTSTRAP_PATHS: frozenset[str] = frozenset(
    {
        ".github/actions/download-all-artifacts",
        ".github/actions/collect-artifact-sizes",
        ".github/actions/replace-cache-entry",
        ".github/actions/setup-python",
        ".github/scripts/check_baseline_ready.py",
        ".github/scripts/ci_bootstrap.py",
        "tools/_bootstrap.py",
        ".github/scripts/collect_artifact_sizes.py",
        ".github/scripts/collect_build_status.py",
        ".github/scripts/download_artifacts.py",
        ".github/scripts/generate_build_results_comment.py",
        ".github/scripts/templates/build_results.md.j2",
        ".github/scripts/xml_utils.py",
        "tools/common/__init__.py",
        "tools/common/build_config.py",
        "tools/common/file_traversal.py",
        "tools/common/format.py",
        "tools/common/gh_actions.py",
        "tools/common/github_runs.py",
        "tools/common/platform.py",
        "tools/pyproject.toml",
        "tools/uv.lock",
        "tools/setup/setup_bootstrap.py",
        "tools/setup/install_python.py",
    }
)

def _bootstrap_sparse_checkout_paths() -> frozenset[str]:
    with BOOTSTRAP_ACTION_YML.open(encoding="utf-8") as fh:
        doc = yaml.safe_load(fh)
    for step in doc.get("runs", {}).get("steps", []):
        if not isinstance(step, dict):
            continue
        if not str(step.get("uses", "")).startswith("actions/checkout"):
            continue
        paths = (step.get("with") or {}).get("sparse-checkout")
        if isinstance(paths, str) and "\n" in paths:
            return frozenset(p.strip() for p in paths.splitlines() if p.strip())
    raise AssertionError("build-results-bootstrap action.yml has no sparse-checkout step")

def test_build_results_bootstrap_sparse_checkout_matches_expected() -> None:
    """Drift guard for the build-results-bootstrap composite: removing a path
    here silently breaks post-pr-comment or save-baselines at runtime. Update
    EXPECTED_BOOTSTRAP_PATHS in lockstep when the consuming scripts change."""
    if not BOOTSTRAP_ACTION_YML.exists():
        pytest.skip("build-results-bootstrap action.yml not in checkout")

    actual = _bootstrap_sparse_checkout_paths()
    missing = EXPECTED_BOOTSTRAP_PATHS - actual
    extra = actual - EXPECTED_BOOTSTRAP_PATHS

    msg_parts = []
    if missing:
        msg_parts.append(f"expected paths missing from composite: {sorted(missing)}")
    if extra:
        msg_parts.append(
            f"composite has paths not in EXPECTED_BOOTSTRAP_PATHS "
            f"(add them here if intentional): {sorted(extra)}"
        )
    if msg_parts:
        pytest.fail(" / ".join(msg_parts))
