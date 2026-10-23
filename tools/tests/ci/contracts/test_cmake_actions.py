"""cmake-configure, cmake-build, and run-unit-tests keep failure logs and link their artifacts."""

from __future__ import annotations

from typing import Any

import pytest
from ci.compile.cmake_helper import CONFIGURE_LOG

from ..._helpers import (
    action_names,
    get_step,
    get_steps,
    load_action,
    load_workflow,
    workflow_names,
)

CMAKE_HELPER = 'python3 "${GITHUB_WORKSPACE}/tools/ci/compile/cmake_helper.py"'
ARTIFACT_LINK = 'python3 "${GITHUB_WORKSPACE}/tools/ci/packaging/artifact_link.py"'


def _named(steps: list[dict[str, Any]], name: str) -> dict[str, Any]:
    return next(step for step in steps if step.get("name") == name)


def test_failed_configure_uploads_cmake_4_logs_and_links_them() -> None:
    steps = get_steps(load_action("cmake-configure"))
    configure, upload, link = steps
    assert configure["id"] == "configure"
    assert configure["run"] == f"{CMAKE_HELPER} configure"
    assert "if" not in configure

    assert upload["id"] == "upload-logs"
    assert upload["uses"] == "actions/upload-artifact@v7"
    assert upload["if"] == "failure() && steps.configure.outputs.logs_artifact != ''"
    assert upload["with"] == {
        "name": "${{ steps.configure.outputs.logs_artifact }}",
        "path": (
            f"${{{{ inputs.build-dir }}}}/{CONFIGURE_LOG}\n"
            "${{ inputs.build-dir }}/CMakeCache.txt\n"
            "${{ inputs.build-dir }}/CMakeFiles/CMakeConfigureLog.yaml\n"
        ),
        "retention-days": 7,
        "if-no-files-found": "ignore",
    }

    assert link["if"] == "failure() && steps.upload-logs.outputs.artifact-url != ''"
    assert link["run"] == f'{ARTIFACT_LINK} --label "Configure logs"'
    assert link["env"] == {
        "ARTIFACT_NAME": "${{ steps.configure.outputs.logs_artifact }}",
        "ARTIFACT_URL": "${{ steps.upload-logs.outputs.artifact-url }}",
    }


def test_build_timings_are_linked_below_the_build_section() -> None:
    steps = get_steps(load_action("cmake-build"))
    build = get_step(steps, "compile")
    upload = get_step(steps, "upload-timings")
    link = _named(steps, "Link build timings")
    assert steps.index(build) < steps.index(upload) == steps.index(link) - 1
    assert upload["if"] == "always() && steps.compile.outputs.profile_path != ''"
    assert link["if"] == "${{ !cancelled() && steps.upload-timings.outputs.artifact-url != '' }}"
    assert link["run"] == f'{ARTIFACT_LINK} --label "Ninja build timings"'
    assert link["env"] == {
        "ARTIFACT_NAME": "${{ steps.compile.outputs.profile_name }}",
        "ARTIFACT_URL": "${{ steps.upload-timings.outputs.artifact-url }}",
    }
    # Nothing between the Build section and the link writes a job-summary section.
    between = steps[steps.index(build) + 1 : steps.index(link)]
    assert [step.get("uses") for step in between] == [
        "./.github/actions/save-build-cache",
        "actions/upload-artifact@v7",
    ]


def test_test_results_are_linked_below_their_report() -> None:
    steps = get_steps(load_action("run-unit-tests"))
    report = _named(steps, "Report test results")
    upload = get_step(steps, "upload-results")
    link = _named(steps, "Link test results")
    assert steps.index(report) < steps.index(upload) == steps.index(link) - 1
    assert link["if"] == "${{ !cancelled() && steps.upload-results.outputs.artifact-url != '' }}"
    assert link["run"] == f'{ARTIFACT_LINK} --label "Test results"'
    assert link["env"] == {
        "ARTIFACT_NAME": "${{ inputs.artifact-name }}",
        "ARTIFACT_URL": "${{ steps.upload-results.outputs.artifact-url }}",
    }


@pytest.mark.parametrize("action", ["cmake-configure", "cmake-build", "run-unit-tests"])
def test_artifact_links_follow_the_upload_they_name(action: str) -> None:
    steps = get_steps(load_action(action))
    for index, step in enumerate(steps):
        url = (step.get("env") or {}).get("ARTIFACT_URL")
        if url is None:
            continue
        upload_id = url.removeprefix("${{ steps.").removesuffix(".outputs.artifact-url }}")
        upload = get_step(steps, upload_id)
        assert upload["uses"].startswith("actions/upload-artifact@")
        assert steps.index(upload) < index
        assert step["run"].startswith(f"{ARTIFACT_LINK} --label ")


def test_android_build_log_upload_names_the_cmake_4_configure_log() -> None:
    steps = load_workflow("android.yml")["jobs"]["build"]["steps"]
    paths = _named(steps, "Upload build log on failure")["with"]["path"].splitlines()
    assert "${{ runner.temp }}/build/CMakeFiles/CMakeConfigureLog.yaml" in paths


def test_no_upload_names_the_logs_cmake_4_stopped_writing() -> None:
    documents = [(name, load_workflow(name)) for name in workflow_names()]
    documents += [(f"actions/{name}", load_action(name)) for name in action_names()]
    stale = []
    for source, document in documents:
        jobs = (document.get("jobs") or {}).values() if "jobs" in document else [document["runs"]]
        for job in jobs:
            for step in job.get("steps") or []:
                path = str((step.get("with") or {}).get("path", ""))
                if "CMakeError.log" in path or "CMakeOutput.log" in path:
                    stale.append(f"{source}: {step.get('name')}")
    assert stale == []


def test_android_drops_the_javac_warning_matcher_before_building() -> None:
    steps = load_workflow("android.yml")["jobs"]["build"]["steps"]
    drop = _named(steps, "Drop javac warning annotations")
    assert drop["run"] == 'echo "::remove-matcher owner=javac::"'
    assert "if" not in drop
    # setup-java (in build-setup) registers the matcher; the Gradle build runs in cmake-build.
    setup = next(step for step in steps if step.get("uses") == "./.github/actions/build-setup")
    build = next(step for step in steps if step.get("uses") == "./.github/actions/cmake-build")
    assert steps.index(setup) < steps.index(drop) < steps.index(build)
