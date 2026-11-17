"""CI Scripts workflow contracts: the files its tests read re-run them, on the pinned Python."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

from ..._helpers import REPO_ROOT, TOOLS_DIR, get_steps, load_workflow
from .._workflows import local_action, path_filter_covers

if TYPE_CHECKING:
    import pytest


def test_every_file_the_test_jobs_check_out_triggers_the_workflow(
    pytestconfig: pytest.Config,
) -> None:
    # A test that parametrizes over repository files fails collection, instead of silently
    # running nothing, when a sparse checkout lacks them.
    assert pytestconfig.getini("empty_parameter_set_mark") == "fail_at_collect"
    workflow = load_workflow("ci-scripts.yml")
    untriggered = set()
    for event in ("pull_request", "push"):
        paths = workflow["on"][event]["paths"]
        for job in workflow["jobs"].values():
            for step in job["steps"]:
                entries = (step.get("with") or {}).get("sparse-checkout", "").split()
                untriggered.update(e for e in entries if not path_filter_covers(e, paths))
    assert not untriggered


def test_python_tests_run_on_the_pinned_python_only() -> None:
    job = load_workflow("ci-scripts.yml")["jobs"]["test-ci-scripts"]
    assert "strategy" not in job
    pinned = (TOOLS_DIR / ".python-version").read_text(encoding="utf-8").strip()
    # The check name carries the version; setup-python takes it from the pin.
    assert job["name"] == f"Test CI Scripts (py{pinned})"


def test_just_test_python_installs_the_dependency_groups_ci_does() -> None:
    """Without a group its tests skip (git-cliff, rasterio, pymavlink), so both must sync it."""
    steps = get_steps(load_workflow("ci-scripts.yml"), "test-ci-scripts")
    (setup,) = (step for step in steps if local_action(step) == "setup-python")
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    recipe = re.search(r"^test-python .*:\n[ \t]+(.+)$", justfile, re.MULTILINE)
    assert recipe is not None
    assert set(re.findall(r"--group (\w+)", recipe[1])) == set(setup["with"]["groups"].split(","))
