"""Contracts for the qt-install action: one plan step resolves the request, key, and save policy."""

from __future__ import annotations

from ..._helpers import get_steps, load_action

REQUEST_INPUTS = ("host", "target", "arch", "modules", "archives")


def _steps() -> dict[str, dict]:
    return {step.get("id") or step["name"]: step for step in get_steps(load_action("qt-install"))}


def test_qt_install_plans_the_request_once_and_ensure_reuses_it() -> None:
    action = load_action("qt-install")
    # The Qt version comes from build-config.json and the installer from QGC_QT_INSTALLER.
    assert set(action["inputs"]) == {*REQUEST_INPUTS, "export-env"}
    steps = _steps()
    plan, ensure = steps["plan"], steps["qt"]
    for name in REQUEST_INPUTS:
        variable = f"QT_{name.upper()}"
        assert plan["env"][variable] == f"${{{{ inputs.{name} }}}}"
        assert f'--{name} "${{{variable}}}"' in plan["run"]
    # The cache digest covers the installer, so ensure installs exactly the planned request.
    assert ensure["env"] == {
        "PLAN": "${{ steps.plan.outputs.plan }}",
        "CACHE_HIT": "${{ steps.qt-cache.outputs.cache-hit }}",
        "EXPORT_ENV": "${{ inputs.export-env }}",
    }
    assert '--plan "${PLAN}"' in ensure["run"]
    assert action["outputs"]["source"]["value"] == "${{ steps.qt.outputs.source }}"


def test_only_runs_that_may_write_the_cache_save_a_fresh_install() -> None:
    steps = _steps()
    plan = steps["plan"]
    assert " ".join(plan["env"]["SAVE_ALLOWED"].split()) == (
        "${{ github.event_name != 'pull_request' || "
        "github.event.pull_request.head.repo.full_name == github.repository }}"
    )
    assert '--save-allowed "${SAVE_ALLOWED}"' in plan["run"]
    fresh = "steps.plan.outputs.save == 'true' && steps.qt-cache.outputs.cache-hit != 'true'"
    assert steps["qt-presave"]["if"] == fresh
    save = steps["Save Qt cache"]
    assert (
        " ".join(save["if"].split()) == f"{fresh} && steps.qt-presave.outputs.cache-hit != 'true'"
    )
    for step in (steps["qt-cache"], steps["qt-presave"], save):
        assert step["with"]["key"] == "${{ steps.plan.outputs.key }}"
