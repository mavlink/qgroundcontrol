"""Contracts for the Qt installer parity workflow and the qt-install action's installer choice."""

from __future__ import annotations

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow

WORKFLOW = "qt-installer-parity.yml"


def test_runs_when_the_installer_or_its_inputs_change() -> None:
    on = load_workflow(WORKFLOW)["on"]
    assert set(on) == {"pull_request", "workflow_dispatch"}
    paths = on["pull_request"]["paths"]
    assert paths == [
        "tools/setup/qt_installer/**",
        "tools/setup/install_qt.py",
        ".github/actions/qt-install/**",
        ".github/workflows/qt-installer-parity.yml",
        ".github/build-config.json",
    ]
    for path in paths:
        assert (REPO_ROOT / path.removesuffix("/**")).exists(), path


def test_every_platform_installs_the_ci_combinations_with_both_installers() -> None:
    job = load_workflow(WORKFLOW)["jobs"]["parity"]
    legs = {
        leg["runner"]: leg["installs"].split("\n") for leg in job["strategy"]["matrix"]["include"]
    }
    assert {runner: [line for line in lines if line] for runner, lines in legs.items()} == {
        "ubuntu-24.04": ["linux desktop linux_gcc_64", "linux android android_arm64_v8a"],
        "ubuntu-24.04-arm": ["linux_arm64 desktop linux_gcc_arm64"],
        "macos-15": ["mac desktop clang_64", "mac ios ios_device"],
        "windows-2025": [
            "windows desktop win64_msvc2022_64",
            "windows desktop win64_msvc2022_arm64_cross_compiled",
        ],
        "windows-11-arm": ["windows_arm64 desktop win64_msvc2022_arm64"],
    }
    assert job["strategy"]["fail-fast"] is False

    steps = job["steps"]
    run = get_step(steps, "Install with both installers and compare")["run"]
    assert "for installer in aqt qgc; do" in run
    assert '--installer "${installer}"' in run
    assert 'install_qt.py compare --version "${QT_VERSION}"' in run
    assert '--reference "${RUNNER_TEMP}/aqt/Qt" --candidate "${RUNNER_TEMP}/qgc/Qt"' in run
    names = [step.get("name") for step in steps]
    # The installer extracts with build-setup's CMake; aqt comes from the locked qt group.
    assert names.index("Setup CMake") < names.index("Install with both installers and compare")
    assert get_step(steps, "Setup Python")["with"] == {"groups": "qt"}


def test_sparse_checkout_holds_the_installer() -> None:
    steps = load_workflow(WORKFLOW)["jobs"]["parity"]["steps"]
    checkout = next(step for step in steps if step.get("uses", "").startswith("actions/checkout@"))
    entries = set(checkout["with"]["sparse-checkout"].split())
    assert {"tools/setup/install_qt.py", "tools/setup/qt_installer", "tools/common"} <= entries
    assert {".github/actions/build-config", ".github/actions/setup-python"} <= entries


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
