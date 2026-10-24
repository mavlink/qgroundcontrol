"""Static contracts for the Build GStreamer workflow; build_gstreamer.py is unit-tested elsewhere."""

from __future__ import annotations

import re
from typing import Any

from setup.gstreamer.ci_plan import APT_CATEGORIES, DESKTOP_QT, PLATFORMS
from setup.install_dependencies._packages import get_debian_packages

from ..._helpers import get_step, get_steps, load_action, load_workflow


def _steps() -> list[dict[str, Any]]:
    return get_steps(load_workflow("build-gstreamer.yml"), "build")


def test_plan_runs_first_and_drives_the_setup_steps() -> None:
    steps = _steps()
    plan = get_step(steps, "plan")
    assert " ".join(plan["run"].replace("\\\n", " ").split()) == (
        'python3 ./tools/setup/gstreamer/ci_plan.py --platform "$PLATFORM" --arch "$ARCH" '
        '--simulator "$SIMULATOR"'
    )
    assert plan["env"] == {
        "PLATFORM": "${{ inputs.platform }}",
        "ARCH": "${{ inputs.arch }}",
        "SIMULATOR": "${{ inputs.simulator }}",
    }
    names = [step.get("id") or step["name"] for step in steps]
    assert names.index("plan") < names.index("Install dependencies (apt)")
    assert names.index("plan") < names.index("Build Setup") < names.index("build")
    setup = get_step(steps, "Build Setup")
    assert setup["if"] == "steps.plan.outputs.qt-host != ''"
    assert setup["with"]["qt-host"] == "${{ steps.plan.outputs.qt-host }}"
    assert setup["with"]["qt-arch"] == "${{ steps.plan.outputs.qt-arch }}"
    for step_id in ("build", "archive"):
        assert get_step(steps, step_id)["env"]["SIMULATOR"] == "${{ steps.plan.outputs.simulator }}"
    # build_gstreamer.py resolves the Windows work directory and the macOS/Windows Qt prefix itself.
    assert {"QT_PREFIX", "WORK_DIR"}.isdisjoint(get_step(steps, "build")["env"])


def test_android_installs_the_pinned_cmake_and_ninja_build_setup_would() -> None:
    """Android skips build-setup, so it installs the same get-cmake release with the plan's pins."""
    step = get_step(_steps(), "Install pinned CMake and Ninja (Android)")
    (setup_step,) = [
        step
        for step in load_action("build-setup")["runs"]["steps"]
        if "lukka/get-cmake@" in step.get("uses", "")
    ]
    assert step["uses"] == setup_step["uses"]
    assert step["if"] == "steps.plan.outputs.cmake-version != ''"
    assert step["with"] == {
        **setup_step["with"],
        "cmakeVersion": "${{ steps.plan.outputs.cmake-version }}",
        "ninjaVersion": "${{ steps.plan.outputs.ninja-version }}",
        # build-setup skips the cloud cache only in Copilot sessions, which never run this.
        "useCloudCache": True,
    }


def test_apt_dependencies_come_from_an_install_dependencies_package_set() -> None:
    step = get_step(_steps(), "Install dependencies (apt)")
    assert step["uses"] == "./.github/actions/install-dependencies"
    assert step["if"] == "steps.plan.outputs.apt-category != ''"
    assert step["with"]["category"] == "${{ steps.plan.outputs.apt-category }}"
    assert all(get_debian_packages(category) for category in APT_CATEGORIES.values())
    assert not any("apt-get" in step.get("run", "") for step in _steps())


def test_build_step_outputs_the_arch_and_prefix_the_archive_step_reads() -> None:
    steps = _steps()
    build = get_step(steps, "build")["run"]
    assert "python3 ./tools/setup/build_gstreamer.py" in build
    assert "--prefix" not in build
    assert "GITHUB_OUTPUT" not in build
    archive = get_step(steps, "archive")["env"]
    assert archive["ARCH"] == "${{ steps.build.outputs.arch }}"
    assert archive["PREFIX"] == "${{ steps.build.outputs.prefix }}"


def test_sparse_checkout_covers_every_script_the_job_runs() -> None:
    steps = _steps()
    roots = get_step(steps, "Checkout")["with"]["sparse-checkout"].split()
    runs = [step.get("run", "") for step in steps]
    runs += [step.get("run", "") for step in get_steps(load_action("install-dependencies"))]
    scripts = set(re.findall(r"(?<![\w-])tools/[\w./-]+\.py", "\n".join(runs)))
    assert "tools/setup/build_gstreamer.py" in scripts
    assert all(any(script.startswith(f"{root}/") for root in roots) for script in scripts)


def test_runs_on_picks_the_arm64_runners_the_plan_accepts() -> None:
    """ci_plan.py accepts Linux aarch64 and Windows arm64, which need native arm64 runners."""
    runs_on = " ".join(load_workflow("build-gstreamer.yml")["jobs"]["build"]["runs-on"].split())
    assert (
        "inputs.platform == 'linux' && (inputs.arch == 'aarch64' && 'ubuntu-24.04-arm'" in runs_on
    )
    assert "inputs.platform == 'windows' && (inputs.arch == 'arm64' && 'windows-11-arm'" in runs_on
    assert set(DESKTOP_QT["linux"]) == {"x86_64", "aarch64"}
    assert set(DESKTOP_QT["windows"]) == {"x64", "arm64"}
    inputs = load_workflow("build-gstreamer.yml")["on"]["workflow_dispatch"]["inputs"]
    assert inputs["platform"]["options"] == list(PLATFORMS)
