"""Copilot setup steps workflow contracts."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_step, load_workflow


def test_runs_only_on_manual_dispatch_and_configures() -> None:
    workflow = load_workflow("copilot-setup-steps.yml")
    assert set(workflow["on"]) == {"workflow_dispatch"}
    assert get_step(workflow["jobs"]["copilot-setup-steps"]["steps"], "Configure")["run"] == (
        "just configure"
    )
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    recipe = re.search(r"^configure:\n((?:[ \t]+.*\n)+)", justfile, re.MULTILINE)
    assert recipe
    assert "./tools/configure.py" in recipe[1]
