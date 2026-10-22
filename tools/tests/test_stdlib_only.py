"""Scripts that run before setup-python import with only the standard library and repository code.

Those steps use the runner's system interpreter, so a third-party import would only fail in CI.
"""

from __future__ import annotations

import re
import subprocess
import sys
from typing import TYPE_CHECKING, Any

import pytest

from ._helpers import REPO_ROOT, get_steps, load_action, load_workflow

if TYPE_CHECKING:
    from collections.abc import Iterator

PRE_SETUP_PYTHON_SCRIPTS: tuple[str, ...] = (
    # Run by jobs without setup-python (test_jobs_without_setup_python_run_only_listed_scripts).
    "deploy/multipass/run_multipass.py",
    "deploy/vagrant/run_vagrant.py",
    "tools/ci/cache/gh_cache_cleanup.py",
    "tools/ci/gstreamer/mirror_gstreamer.py",
    "tools/ci/planning/analysis_matrix.py",
    "tools/ci/planning/android_matrix.py",
    "tools/ci/planning/detect_changes.py",
    "tools/ci/planning/plan_docker_builds.py",
    "tools/ci/publish/deploy_docs.py",
    "tools/ci/publish/release_assets.py",
    "tools/ci/publish/release_builds.py",
    "tools/ci/reporting/collect_build_status.py",
    "tools/ci/reporting/download_artifacts.py",
    "tools/ci/reporting/gh_pr_size_label.py",
    "tools/ci/reporting/report_context.py",
    "tools/ci/runner/apt_install_retry.py",
    "tools/ci/runner/enable_kvm.py",
    "tools/ci/runner/install_multipass.py",
    "tools/ci/runner/runner_image.py",
    "tools/release.py",
    "tools/setup/read_config.py",
    # Run before a later or conditional setup-python.
    "tools/ci/compile/docker_helper.py",  # docker.yml resolves the push target first
    "tools/ci/gstreamer/gstreamer_archive.py",  # Android and iOS GStreamer builds skip build-setup
    "tools/setup/build_gstreamer.py",
    "tools/ci/runner/install_dependencies_helper.py",  # install-dependencies sets up Python on Windows only
    "tools/setup/install_dependencies/__main__.py",
    "tools/setup/install_python.py",  # setup-python's own installer
    "tools/setup/install_uv.py",  # VM and runner-image provisioning
    # Run as `python3` from actions' bash steps, which on Windows resolves to the runner's interpreter
    # rather than tools/.venv (the venv only provides python.exe).
    "tools/ci/android/android_sdk_helper.py",
    "tools/ci/cache/ccache_helper.py",
    "tools/ci/compile/cmake_helper.py",
    "tools/moccache.py",
)

# Runs the script's module body without its __main__ block, with the script directory on
# sys.path as `python3 path/to/script.py` would have it.
_PROBE = (
    "import os, runpy, sys; "
    "sys.path.insert(0, os.path.dirname(sys.argv[1])); "
    "runpy.run_path(sys.argv[1], run_name='__stdlib_probe__')"
)

_SCRIPT_RE = re.compile(r"(?<![\w-])(?:tools|deploy)/[\w./-]+")
_LOCAL_ACTION_PREFIXES = ("./.github/actions/", "./.github/actions/")


@pytest.mark.parametrize("script", PRE_SETUP_PYTHON_SCRIPTS)
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


def _expanded_steps(
    steps: list[dict[str, Any]] | None, seen: tuple[str, ...] = ()
) -> Iterator[dict[str, Any]]:
    """Yield a job's steps with local composite actions expanded in place."""
    for step in steps or []:
        yield step
        uses = str(step.get("uses", ""))
        if uses.startswith(_LOCAL_ACTION_PREFIXES):
            name = uses.split("/.github/actions/", 1)[1].strip("/")
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
    for workflow in sorted((REPO_ROOT / ".github" / "workflows").glob("*.yml")):
        for job_name, job in (load_workflow(workflow.name).get("jobs") or {}).items():
            steps = list(_expanded_steps(job.get("steps")))
            if any("setup-python" in str(step.get("uses", "")) for step in steps):
                continue
            unlisted += [
                f"{workflow.name}:{job_name} runs {script}"
                for step in steps
                for script in sorted(_scripts_run_by(step))
                if script not in PRE_SETUP_PYTHON_SCRIPTS
            ]
    assert not unlisted, "Add these to PRE_SETUP_PYTHON_SCRIPTS:\n" + "\n".join(unlisted)
