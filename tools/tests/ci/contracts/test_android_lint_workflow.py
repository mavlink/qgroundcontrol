"""android.yml runs Android Lint through android_lint.py and keeps its reports after a failure."""

from __future__ import annotations

from ci.android.android_lint import DEFAULT_REPORT, LOG_NAME

from ..._helpers import get_step, get_steps, load_workflow

PROJECT = "${{ runner.temp }}/build/android-build"
REPORTS = f"{PROJECT}/{DEFAULT_REPORT.parent.as_posix()}"
ARTIFACT = "android-${{ matrix.host }}-lint-reports"


def _build_steps() -> list[dict]:
    return get_steps(load_workflow("android.yml"), "build")


def test_lint_runs_the_script_on_the_linux_leg() -> None:
    lint = get_step(_build_steps(), "Android Lint")
    assert lint["id"] == "lint"
    assert lint["if"] == "${{ matrix.host == 'linux' }}"
    assert lint["shell"] == "bash"
    assert lint["run"] == (
        'python3 "${GITHUB_WORKSPACE}/tools/ci/android/android_lint.py" \\\n'
        '  --project-dir "${RUNNER_TEMP}/build/android-build"\n'
    )
    # The script reports a missing project itself instead of the runner failing the step.
    assert "working-directory" not in lint


def test_failed_lint_uploads_its_reports_and_links_them_below_the_summary() -> None:
    steps = _build_steps()
    lint = get_step(steps, "lint")
    upload = get_step(steps, "upload-lint")
    link = get_step(steps, "Link lint reports")
    # Nothing between the Android Lint section and the link writes a job-summary section.
    assert steps.index(lint) + 1 == steps.index(upload) == steps.index(link) - 1

    assert upload["if"] == "${{ failure() && steps.lint.outcome == 'failure' }}"
    assert upload["uses"] == "actions/upload-artifact@v7"
    assert upload["with"] == {
        "name": ARTIFACT,
        "path": (
            f"{REPORTS}/lint-results-release.html\n"
            f"{REPORTS}/{DEFAULT_REPORT.name}\n"
            f"${{{{ runner.temp }}}}/build/{LOG_NAME}\n"
        ),
        "if-no-files-found": "ignore",
        "retention-days": 7,
    }

    assert link["if"] == "${{ failure() && steps.upload-lint.outputs.artifact-url != '' }}"
    assert link["shell"] == "bash"
    assert link["run"] == (
        'python3 "${GITHUB_WORKSPACE}/tools/ci/packaging/artifact_link.py" '
        '--label "Android Lint reports"'
    )
    assert link["env"] == {
        "ARTIFACT_NAME": ARTIFACT,
        "ARTIFACT_URL": "${{ steps.upload-lint.outputs.artifact-url }}",
    }
