"""The build actions keep the logs CMake and cmake_helper.py write, and link what they upload."""

from __future__ import annotations

from ci.compile.cmake_helper import CONFIGURE_LOG

from ..._helpers import get_step, get_steps, load_action
from .._workflows import step_groups, uses_action


def test_failed_configure_uploads_the_log_cmake_helper_writes() -> None:
    uploaded = [
        path
        for step in get_steps(load_action("cmake-configure"))
        if uses_action(step, "actions/upload-artifact")
        for path in step["with"]["path"].splitlines()
    ]
    assert any(path.endswith(f"/{CONFIGURE_LOG}") for path in uploaded)


def test_artifact_links_follow_the_upload_they_name() -> None:
    linked = 0
    for label, _, steps in step_groups():
        for index, step in enumerate(steps):
            url = (step.get("env") or {}).get("ARTIFACT_URL")
            if url is None:
                continue
            upload_id = url.removeprefix("${{ steps.").removesuffix(".outputs.artifact-url }}")
            upload = get_step(steps, upload_id)
            assert uses_action(upload, "actions/upload-artifact"), label
            assert steps.index(upload) < index, label
            linked += 1
    assert linked


def test_no_upload_names_the_logs_cmake_4_stopped_writing() -> None:
    # CMake 4 records its try_compile and find_* results in CMakeConfigureLog.yaml.
    stale = [
        f"{label}: {step.get('name')}"
        for label, _, steps in step_groups()
        for step in steps
        if any(
            log in str((step.get("with") or {}).get("path", ""))
            for log in ("CMakeError.log", "CMakeOutput.log")
        )
    ]
    assert stale == []
