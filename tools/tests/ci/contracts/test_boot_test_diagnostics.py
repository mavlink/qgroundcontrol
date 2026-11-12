"""Boot tests keep their diagnostics: the outputs verify-executable reads, and the iOS time budget."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

from ci.ios import ios_boot_test
from ci.packaging import verify_executable

from ..._helpers import get_steps, load_action, load_workflow, read_github_file

if TYPE_CHECKING:
    from pathlib import Path


def test_verify_executable_reads_the_outputs_the_script_writes(
    tmp_path: Path, monkeypatch, gh_output
) -> None:
    action = load_action("verify-executable")
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    verify_executable._prepare_diagnostics(tmp_path / "QGroundControl")
    written = set(read_github_file(gh_output))
    assert set(re.findall(r"steps\.verify\.outputs\.([\w-]+)", str(action))) == written


def test_ios_boot_test_ends_before_its_step_timeout() -> None:
    steps = get_steps(load_workflow("ios.yml"), "build")
    (boot,) = (step for step in steps if "ios_boot_test.py" in step.get("run", ""))
    # The script's budget, then the last attempt's diagnostics and cleanup, end before the step
    # timeout, so the error annotation and job summary are written and the upload still runs.
    worst_case = (
        ios_boot_test.DEFAULT_TOTAL_TIMEOUT
        + ios_boot_test.DIAGNOSTIC_TIMEOUT
        + 2 * ios_boot_test.LOG_SHOW_TIMEOUT
        + 2 * ios_boot_test.CLEANUP_TIMEOUT
    )
    assert "--total-timeout" not in boot["run"]
    assert worst_case < boot["timeout-minutes"] * 60
