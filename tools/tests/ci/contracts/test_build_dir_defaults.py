"""Build and test actions share the runner-temp build directory, so callers don't repeat it."""

from __future__ import annotations

import pytest

from ..._helpers import load_action, load_workflow, workflow_names

ACTIONS = ("cmake-configure", "cmake-build", "test-phase", "run-unit-tests")
DEFAULT = "${{ runner.temp }}/build"


@pytest.mark.parametrize("name", ACTIONS)
def test_build_actions_default_to_the_runner_temp_build_directory(name: str) -> None:
    assert load_action(name)["inputs"]["build-dir"]["default"] == DEFAULT


def test_callers_do_not_repeat_the_default_build_directory() -> None:
    uses = {f"./.github/actions/{name}" for name in ACTIONS}
    repeated = [
        f"{workflow}:{job_name}:{step.get('name', step['uses'])}"
        for workflow in workflow_names()
        for job_name, job in (load_workflow(workflow).get("jobs") or {}).items()
        for step in job.get("steps") or []
        if step.get("uses") in uses
        and str((step.get("with") or {}).get("build-dir", "")).replace("\\", "/") == DEFAULT
    ]
    assert not repeated
