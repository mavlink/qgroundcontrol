"""Scripts that run before setup-python import with only the standard library and repository code.

Those steps use the runner's system interpreter, so a third-party import would only fail in CI.
"""

from __future__ import annotations

import re
import subprocess
import sys
from typing import TYPE_CHECKING, Any

import pytest

from ._helpers import (
    REPO_ROOT,
    TOOLS_DIR,
    action_names,
    get_steps,
    load_action,
    load_workflow,
    workflow_names,
)

if TYPE_CHECKING:
    from collections.abc import Iterator

PRE_SETUP_PYTHON_SCRIPTS: tuple[str, ...] = (
    # Run by jobs without setup-python (test_jobs_without_setup_python_run_only_listed_scripts).
    "deploy/multipass/run_multipass.py",
    "deploy/vagrant/run_vagrant.py",
    "tools/ci/cache/gh_cache_cleanup.py",
    "tools/ci/gstreamer/mirror_gstreamer.py",
    "tools/ci/planning/analysis_matrix.py",
    "tools/ci/planning/plan_platform.py",
    "tools/ci/publish/deploy_docs.py",
    "tools/ci/reporting/collect_build_status.py",
    "tools/ci/reporting/download_artifacts.py",
    "tools/ci/reporting/gh_pr_size_label.py",
    "tools/ci/reporting/report_context.py",
    "tools/ci/runner/apt_install_retry.py",
    "tools/ci/runner/enable_kvm.py",
    "tools/ci/runner/install_multipass.py",
    "tools/ci/runner/runner_image.py",
    "tools/setup/read_config.py",
    "tools/update_px4_metadata.py",
    # Run before a later or conditional setup-python.
    "tools/ci/publish/release_branch_check.py",  # release.yml checks the tag before installing
    "tools/ci/gstreamer/gstreamer_archive.py",  # Android and iOS GStreamer builds skip build-setup
    "tools/setup/build_gstreamer.py",
    "tools/setup/gstreamer/ci_plan.py",  # build-gstreamer.yml plans before build-setup
    "tools/ci/runner/install_dependencies_helper.py",  # install-dependencies sets up Python on Windows only
    "tools/ci/runner/build_setup.py",  # build-setup plans before its setup-python
    "tools/setup/install_dependencies/__main__.py",
    "tools/ci/cache/cache_backend.py",  # setup-python's cache backend probe
    "tools/setup/install_python.py",  # setup-python's own installer
    "tools/setup/install_uv.py",  # VM and runner-image provisioning
)

# Composite actions run scripts as `python3`. install_python.py gives the Windows tools/.venv a
# python3.exe so they resolve to the venv; staying stdlib-only keeps them working if a runner's
# PATH still prefers its own interpreter.
_ACTION_PYTHON3_RE = re.compile(
    r'\bpython3\s+"?(?:\$\{GITHUB_WORKSPACE\}/)?((?:tools|deploy)/[\w./-]+\.py)\b'
)
# Third-party imports; only run after setup-python, so `python3` is the venv's on every OS.
_VENV_ACTION_SCRIPTS = frozenset({"tools/ci/reporting/junit_report.py"})  # defusedxml


def _action_python3_scripts() -> set[str]:
    scripts = set()
    for name in action_names():
        for step in get_steps(load_action(name)):
            inputs = step.get("with") or {}
            shell = [step.get("run", ""), inputs.get("command", ""), inputs.get("script", "")]
            scripts.update(_ACTION_PYTHON3_RE.findall("\n".join(shell)))
    return scripts


STDLIB_ONLY_SCRIPTS = sorted(
    {*PRE_SETUP_PYTHON_SCRIPTS, *_action_python3_scripts()} - _VENV_ACTION_SCRIPTS
)

# Packages behind those scripts, under tools/; every module is probed, not only those the script
# imports today.
STDLIB_ONLY_PACKAGES: tuple[str, ...] = (
    "moccache",  # moccache_launcher.py
    "setup.gstreamer",  # build_gstreamer.py
)

STDLIB_ONLY_MODULES = sorted(
    package if path.stem == "__init__" else f"{package}.{path.stem}"
    for package in STDLIB_ONLY_PACKAGES
    for path in (TOOLS_DIR / package.replace(".", "/")).glob("*.py")
)

# Runs the script's module body without its __main__ block, with the script directory on
# sys.path as `python3 path/to/script.py` would have it.
_PROBE = (
    "import os, runpy, sys; "
    "sys.path.insert(0, os.path.dirname(sys.argv[1])); "
    "runpy.run_path(sys.argv[1], run_name='__stdlib_probe__')"
)
_MODULE_PROBE = (
    "import importlib, sys; sys.path.insert(0, sys.argv[1]); importlib.import_module(sys.argv[2])"
)

_SCRIPT_RE = re.compile(r"(?<![\w-])(?:tools|deploy)/[\w./-]+")
_LOCAL_ACTION_PREFIX = "./.github/actions/"


@pytest.mark.parametrize("script", STDLIB_ONLY_SCRIPTS)
def test_imports_without_site_packages(script: str) -> None:
    # -I -S hides site-packages and environment overrides, like a bare runner interpreter.
    result = subprocess.run(
        [sys.executable, "-I", "-S", "-c", _PROBE, str(REPO_ROOT / script)],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("module", STDLIB_ONLY_MODULES)
def test_package_modules_import_without_site_packages(module: str) -> None:
    result = subprocess.run(
        [sys.executable, "-I", "-S", "-c", _MODULE_PROBE, str(TOOLS_DIR), module],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )
    assert result.returncode == 0, result.stderr


def _expanded_steps(
    steps: list[dict[str, Any]] | None, seen: tuple[str, ...] = ()
) -> Iterator[dict[str, Any]]:
    """Yield a job's steps with local composite actions expanded in place."""
    for step in steps or []:
        yield step
        uses = str(step.get("uses", ""))
        if uses.startswith(_LOCAL_ACTION_PREFIX):
            name = uses.removeprefix(_LOCAL_ACTION_PREFIX).strip("/")
            if name not in seen:
                yield from _expanded_steps(get_steps(load_action(name)), (*seen, name))


def _scripts_run_by(step: dict[str, Any]) -> set[str]:
    text = f"{step.get('run', '')}\n{(step.get('with') or {}).get('command', '')}"
    scripts = set()
    for match in _SCRIPT_RE.findall(text):
        if match.endswith(".py"):
            scripts.add(match)
        elif match.rstrip("/") == "tools/setup/install_dependencies":
            scripts.add("tools/setup/install_dependencies/__main__.py")
    return scripts


def test_jobs_without_setup_python_run_only_listed_scripts() -> None:
    unlisted = []
    for workflow in workflow_names():
        for job_name, job in (load_workflow(workflow).get("jobs") or {}).items():
            steps = list(_expanded_steps(job.get("steps")))
            if any("setup-python" in str(step.get("uses", "")) for step in steps):
                continue
            unlisted += [
                f"{workflow}:{job_name} runs {script}"
                for step in steps
                for script in sorted(_scripts_run_by(step))
                if script not in STDLIB_ONLY_SCRIPTS
            ]
    assert not unlisted, "Add these to PRE_SETUP_PYTHON_SCRIPTS:\n" + "\n".join(unlisted)
