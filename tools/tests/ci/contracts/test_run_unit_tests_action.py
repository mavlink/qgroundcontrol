"""run-unit-tests must report results after failed tests without hiding the failure."""

from __future__ import annotations

from typing import Any

from ..._helpers import action_names, get_steps, load_action, load_workflow, workflow_names


def _steps(name: str) -> list[dict[str, Any]]:
    return get_steps(load_action(name))


def test_reports_follow_failed_tests_and_keep_the_failure() -> None:
    ctest, *reports = _steps("run-unit-tests")
    assert ctest["name"] == "Run CTest"
    assert "if" not in ctest
    assert "continue-on-error" not in ctest
    assert [step.get("uses", "").split("@")[0] for step in reports] == [
        "",
        "codecov/codecov-action",
        "actions/upload-artifact",
        "",
    ]
    for step in reports:
        assert step["if"].startswith("${{ !cancelled() && ")
        assert "continue-on-error" not in step


def test_ctest_reads_every_selection_input_from_its_environment() -> None:
    ctest = _steps("run-unit-tests")[0]
    assert ctest["run"] == 'python3 "${GITHUB_WORKSPACE}/tools/ci/compile/cmake_helper.py" ctest'
    assert ctest["env"] == {
        f"INPUT_{name.upper().replace('-', '_')}": f"${{{{ inputs.{name} }}}}"
        for name in ("junit-output", "ctest-output", "parallel", "include-labels", "exclude-labels")
    }


def test_codecov_upload_tolerates_missing_junit_without_a_probe() -> None:
    steps = _steps("run-unit-tests")
    assert not any("GITHUB_OUTPUT" in step.get("run", "") for step in steps)
    upload = next(step for step in steps if step.get("uses") == "codecov/codecov-action@v7")
    assert upload["if"] == "${{ !cancelled() && inputs.upload-codecov == 'true' }}"
    assert upload["with"] == {
        "files": "${{ inputs.build-dir }}/${{ inputs.junit-output }}",
        "report_type": "test_results",
        "flags": "${{ inputs.codecov-flags }}",
        "name": "${{ inputs.report-name }}",
        "use_oidc": True,
        "disable_search": True,
        "handle_no_reports_found": True,
        "fail_ci_if_error": False,
    }


def test_test_phase_reports_each_suite_from_its_own_run() -> None:
    steps = _steps("test-phase")
    runs = [step for step in steps if step.get("uses") == "./.github/actions/run-unit-tests"]
    assert [step["with"]["include-labels"] for step in runs] == ["Unit", "Integration"]
    declared = set(load_action("run-unit-tests")["inputs"])
    assert all(set(step["with"]) <= declared for step in runs)
    assert runs[1]["if"] == "${{ !cancelled() }}"
    assert [step["with"]["exclude-labels"] for step in runs] == ["${{ inputs.exclude-labels }}"] * 2
    # Test summaries go to the job summary; the JUnit XML in test-results-* holds the details.
    assert steps == runs
    (report,) = [
        step for step in _steps("run-unit-tests") if "junit_report.py" in step.get("run", "")
    ]
    assert report["if"] == "${{ !cancelled() && inputs.report-name != '' }}"
    assert report["env"] == {
        "INPUT_JUNIT_PATH": "${{ inputs.junit-output }}",
        "INPUT_TITLE": "${{ inputs.report-name }}",
        "INPUT_SLOW_WARNINGS": "${{ inputs.duration-report }}",
    }
    for step, kind in zip(runs, ("unit", "integration"), strict=True):
        options = step["with"]
        suite = f"${{{{ inputs.suite }}}}-{kind}"
        assert options["junit-output"] == f"junit-results-{suite}.xml"
        assert options["ctest-output"] == f"test-output-{suite}.txt"
        assert options["artifact-name"] == f"test-results-{suite}"
        assert options["retention-days"] == 7
        assert options["upload-codecov"] == "${{ inputs.upload-codecov }}"
        assert options["codecov-flags"] == suite
        assert options["duration-report"] == "true"


def _codecov_uploads() -> list[tuple[str, dict[str, Any]]]:
    documents = [(name, load_workflow(name)) for name in workflow_names()]
    documents += [(f"actions/{name}", load_action(name)) for name in action_names()]
    uploads = []
    for source, document in documents:
        jobs = (document.get("jobs") or {}).values() if "jobs" in document else [document["runs"]]
        for job in jobs:
            for step in job.get("steps") or []:
                if str(step.get("uses", "")).startswith("codecov/codecov-action@"):
                    uploads.append((source, step))
    return uploads


def test_codecov_uploads_use_oidc_and_only_the_named_reports() -> None:
    # The CODECOV_TOKEN secret was rejected (404 "Repository not found"), so master coverage never
    # reached Codecov; the default search also uploaded Qt's and CPM packages' coverage files.
    uploads = _codecov_uploads()
    assert {source for source, _ in uploads} >= {"linux.yml", "actions/run-unit-tests"}
    for source, step in uploads:
        options = step["with"]
        assert options["use_oidc"] is True, source
        assert options["disable_search"] is True, source
        assert "token" not in options, source
        assert options["files"], source
