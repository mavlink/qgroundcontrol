"""Boot-test diagnostics reach an artifact that the job summary links."""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

from ci.ios import ios_boot_test
from ci.packaging import verify_executable

from ..._helpers import REPO_ROOT, load_action, load_workflow

if TYPE_CHECKING:
    from pathlib import Path

ARTIFACT_LINK = "tools/ci/packaging/artifact_link.py"


def _steps(steps: list[dict]) -> dict[str, dict]:
    return {step["name"]: step for step in steps}


def test_verify_executable_uploads_failure_diagnostics_and_links_them(
    tmp_path: Path, monkeypatch, gh_output
) -> None:
    action = load_action("verify-executable")
    steps = _steps(action["runs"]["steps"])
    assert list(steps) == [
        "Verify executable",
        "Upload boot test diagnostics",
        "Link boot test diagnostics",
    ]
    verify, upload, link = steps.values()
    assert verify["id"] == "verify"
    assert upload["id"] == "upload"
    assert upload["uses"] == "actions/upload-artifact@v7"
    assert upload["if"] == "failure() && steps.verify.outputs.diagnostics-dir != ''"
    assert upload["with"] == {
        "name": "${{ steps.verify.outputs.diagnostics-name }}",
        "path": "${{ steps.verify.outputs.diagnostics-dir }}",
        "retention-days": 7,
        "if-no-files-found": "ignore",
    }
    assert link["if"] == "failure() && steps.upload.outputs.artifact-url != ''"
    assert ARTIFACT_LINK in link["run"]
    assert link["env"] == {
        "ARTIFACT_NAME": "${{ steps.verify.outputs.diagnostics-name }}",
        "ARTIFACT_URL": "${{ steps.upload.outputs.artifact-url }}",
    }

    # The outputs the action reads are the ones verify_executable.py writes.
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    verify_executable._prepare_diagnostics(tmp_path / "QGroundControl")
    written = {line.split("=", 1)[0] for line in gh_output.read_text().splitlines()}
    assert set(re.findall(r"steps\.verify\.outputs\.([\w-]+)", str(action))) == written


def test_msvc_release_links_write_the_pdb_that_symbolizes_crash_dumps() -> None:
    toolchain = (REPO_ROOT / "cmake" / "Toolchain.cmake").read_text(encoding="utf-8")
    msvc = toolchain.split("elseif(MSVC)", 1)[1].split("endif()", 1)[0]
    # /DEBUG alone would switch the linker to /OPT:NOREF,NOICF and grow the release binary.
    assert 'add_link_options("$<$<CONFIG:Release>:/DEBUG:FULL;/OPT:REF;/OPT:ICF>")' in msvc
    assert 'CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded"' in msvc


def test_android_emulator_diagnostics_are_linked() -> None:
    steps = _steps(load_action("android-emulator-test")["runs"]["steps"])
    upload = steps["Upload Emulator Diagnostics"]
    link = steps["Link Emulator Diagnostics"]
    assert list(steps).index("Link Emulator Diagnostics") == len(steps) - 1
    assert upload["id"] == "upload-diagnostics"
    assert link["if"] == "failure() && steps.upload-diagnostics.outputs.artifact-url != ''"
    assert ARTIFACT_LINK in link["run"]
    assert link["env"] == {
        "ARTIFACT_NAME": upload["with"]["name"],
        "ARTIFACT_URL": "${{ steps.upload-diagnostics.outputs.artifact-url }}",
    }


def test_ios_boot_test_is_bounded_and_keeps_its_diagnostics() -> None:
    steps = _steps(load_workflow("ios.yml")["jobs"]["build"]["steps"])
    boot = steps["Run simulator boot test"]
    upload = steps["Upload simulator boot log"]
    link = steps["Link simulator boot log"]
    names = list(steps)
    assert names.index(boot["name"]) < names.index(upload["name"]) < names.index(link["name"])

    # The script's budget, then the last attempt's diagnostics and cleanup, end before the step
    # timeout, so the error annotation and job summary are written and the upload still runs.
    worst_case = (
        ios_boot_test.DEFAULT_TOTAL_TIMEOUT
        + ios_boot_test.DIAGNOSTIC_TIMEOUT
        + 2 * ios_boot_test.LOG_SHOW_TIMEOUT
        + 2 * ios_boot_test.CLEANUP_TIMEOUT
    )
    assert boot["timeout-minutes"] == 30
    assert worst_case < boot["timeout-minutes"] * 60
    assert "--total-timeout" not in boot["run"]

    # Failed attempts save their diagnostics beside the log, so the artifact holds the directory.
    assert '--log "$RUNNER_TEMP/ios-boot-test/ios-boot-test.log"' in boot["run"]
    assert upload["id"] == "boot-log"
    assert upload["if"] == "${{ !cancelled() && matrix.target == 'simulator' }}"
    assert upload["with"] == {"name": "ios-boot-test", "path": "${{ runner.temp }}/ios-boot-test"}
    assert link["if"] == "${{ !cancelled() && steps.boot-log.outputs.artifact-url != '' }}"
    assert ARTIFACT_LINK in link["run"]
    assert link["env"] == {
        "ARTIFACT_NAME": upload["with"]["name"],
        "ARTIFACT_URL": "${{ steps.boot-log.outputs.artifact-url }}",
    }
