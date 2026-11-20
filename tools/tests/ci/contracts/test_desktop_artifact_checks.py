"""windows.yml audits the install its installer test leaves, and runs the documented GStreamer tests."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_steps, load_workflow
from .._workflows import local_action

GSTREAMER_README = REPO_ROOT / "src/VideoManager/GStreamer/README.md"


def test_dll_audit_reads_the_install_directory_the_installer_test_exports() -> None:
    steps = get_steps(load_workflow("windows.yml"), "build")
    (audit,) = (step for step in steps if "pe_dependency_audit.py" in step.get("run", ""))
    (variable,) = re.findall(r'--install-dir "\$\{(\w+)\}"', audit["run"])
    installer_test = (REPO_ROOT / "deploy/windows/verify-installer.ps1").read_text(encoding="utf-8")
    assert f'"{variable}=$installDir" | Out-File -FilePath $env:GITHUB_ENV' in installer_test


def test_windows_gstreamer_job_selects_the_test_classes_the_readme_lists() -> None:
    steps = get_steps(load_workflow("windows.yml"), "gstreamer-tests")
    (tests,) = (step for step in steps if local_action(step) == "run-unit-tests")
    pattern = re.compile(tests["with"]["tests-regex"])
    documented = re.findall(
        r"^\| `(\w+Test)` \|", GSTREAMER_README.read_text(encoding="utf-8"), re.M
    )
    assert documented
    assert all(pattern.match(name) for name in documented)
    assert not pattern.match("ParameterManagerTest")
