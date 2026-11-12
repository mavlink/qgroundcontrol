"""Build and test actions share one default build directory, so callers don't repeat it."""

from __future__ import annotations

from ..._helpers import load_action
from .._workflows import local_action, step_groups

ACTIONS = ("cmake-configure", "cmake-build", "test-phase", "run-unit-tests")


def _defaults() -> set[str]:
    return {load_action(name)["inputs"]["build-dir"]["default"] for name in ACTIONS}


def test_build_actions_share_one_default_build_directory() -> None:
    assert len(_defaults()) == 1


def test_callers_do_not_repeat_the_default_build_directory() -> None:
    (default,) = _defaults()
    repeated = [
        f"{label}:{step.get('name', step['uses'])}"
        for label, _, steps in step_groups(actions=False)
        for step in steps
        if local_action(step) in ACTIONS
        and str((step.get("with") or {}).get("build-dir", "")).replace("\\", "/") == default
    ]
    assert not repeated
