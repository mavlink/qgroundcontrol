"""Contracts for the qt-install action's installer choice."""

from __future__ import annotations

from ..._helpers import get_steps, load_action


def test_qt_install_plans_the_installer_once_and_reuses_it() -> None:
    action = load_action("qt-install")
    assert action["inputs"]["installer"]["default"] == ""
    steps = get_steps(action)
    plan = next(step for step in steps if step.get("id") == "plan")
    ensure = next(step for step in steps if step.get("id") == "qt")
    assert plan["env"]["QT_INSTALLER"] == "${{ inputs.installer }}"
    assert plan["env"]["QT_HOST"] == "${{ inputs.host }}"
    assert plan["env"]["QT_TARGET"] == "${{ inputs.target }}"
    for option in (
        '--host "${QT_HOST}"',
        '--target "${QT_TARGET}"',
        '--installer "${QT_INSTALLER}"',
    ):
        assert option in plan["run"]
    # The cache digest covers the installer, so ensure must install with the planned one.
    assert ensure["env"]["QT_INSTALLER"] == "${{ steps.plan.outputs.installer }}"
    assert '--installer "${QT_INSTALLER}"' in ensure["run"]
