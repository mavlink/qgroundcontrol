"""Copilot setup steps workflow contracts."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_step, load_workflow


def test_runs_when_just_configure_changes() -> None:
    workflow = load_workflow("copilot-setup-steps.yml")
    paths = workflow["on"]["push"]["paths"]
    assert workflow["on"]["pull_request"]["paths"] == paths
    assert get_step(workflow["jobs"]["copilot-setup-steps"]["steps"], "Configure")["run"] == (
        "just configure"
    )
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    recipe = re.search(r"^configure:\n((?:[ \t]+.*\n)+)", justfile, re.MULTILINE)
    assert recipe
    assert "./tools/configure.py" in recipe[1]
    assert {
        ".github/workflows/copilot-setup-steps.yml",
        "justfile",
        "tools/configure.py",
        "tools/qgc_tools/cmake_configure.py",
        "tools/pyproject.toml",
        "tools/uv.lock",
    } <= set(paths)
