"""Static contracts for the Build GStreamer workflow; build_gstreamer.py is unit-tested elsewhere."""

from __future__ import annotations

import re
from typing import Any

from setup.install_dependencies._packages import get_debian_packages

from ..._helpers import get_step, get_steps, load_action, load_workflow


def _steps() -> list[dict[str, Any]]:
    return get_steps(load_workflow("build-gstreamer.yml"), "build")


def test_linux_dependencies_come_from_an_install_dependencies_package_set() -> None:
    step = get_step(_steps(), "Install dependencies (Linux)")
    assert step["uses"] == "./.github/actions/install-dependencies"
    assert get_debian_packages(step["with"]["category"])


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


def test_arm64_builds_install_qt_from_the_matching_host_repository() -> None:
    """Qt's linux_x64 and windows_x86 repositories have no native ARM64 desktop packages."""
    qt_host = " ".join(get_step(_steps(), "Build Setup")["with"]["qt-host"].split())
    assert "inputs.arch == 'aarch64' && 'linux_arm64'" in qt_host
    assert "inputs.arch == 'arm64' && 'windows_arm64'" in qt_host
