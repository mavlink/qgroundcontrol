"""Tests for android_lint.py, on lint XML reports in AGP's format."""

from __future__ import annotations

import os
import re
import textwrap
from pathlib import Path

import pytest
from ci.android import android_lint
from qgc_tools.build_errors import MAX_ERRORS

# lint 9.2.1's XML report for a QGC tree, as `./gradlew lintRelease` writes it; {project} is the
# Gradle project (RUNNER_TEMP/build/android-build) and {qt} the Qt for Android kit's Java sources.
LINT_REPORT = """\
<?xml version="1.0" encoding="UTF-8"?>
<issues format="6" by="lint 9.2.1">

    <issue
        id="DuplicateActivity"
        severity="Fatal"
        message="Duplicate registration for activity `org.mavlink.qgroundcontrol.QGCActivity`"
        category="Correctness"
        priority="5"
        summary="Activity registered more than once"
        explanation="An activity should only be registered once in the manifest. If it is accidentally registered more than once, then subtle errors can occur, since attribute declarations from the two elements are not merged, so you may accidentally remove previous declarations."
        errorLine1="        &lt;activity"
        errorLine2="         ~~~~~~~~">
        <location
            file="{project}/AndroidManifest.xml"
            line="96"
            column="10"/>
        <location
            file="{project}/AndroidManifest.xml"
            line="49"
            column="10"
            message="Previously registered here"/>
    </issue>

    <issue
        id="NewApi"
        severity="Error"
        message="Call requires API level 33 (current min is 28): `android.content.Context#registerReceiver`"
        category="Correctness"
        priority="6"
        summary="Calling new methods on older versions"
        explanation="This check scans through all the Android API calls in the application and warns about any calls that are not available on **all** versions targeted by this application (according to its minimum SDK attribute in the manifest).&#xA;&#xA;If you really want to use this API and don&apos;t need to support older devices just set the `minSdkVersion` in your `build.gradle` or `AndroidManifest.xml` files."
        url="https://developer.android.com/reference/android/os/Build.VERSION_CODES"
        urls="https://developer.android.com/reference/android/os/Build.VERSION_CODES"
        errorLine1="            context.registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED);"
        errorLine2="                    ~~~~~~~~~~~~~~~~">
        <location
            file="{project}/src/org/mavlink/qgroundcontrol/QGCUsbSerialManager.java"
            line="358"
            column="21"/>
    </issue>

    <issue
        id="NewApi"
        severity="Error"
        message="Call requires API level 30 (current min is 28): `android.view.Display#getMode`"
        category="Correctness"
        priority="6"
        summary="Calling new methods on older versions"
        explanation="This check scans through all the Android API calls in the application."
        errorLine1="        Display.Mode mode = display.getMode();"
        errorLine2="                                    ~~~~~~~">
        <location
            file="{qt}/QtActivity.java"
            line="120"
            column="37"/>
    </issue>

    <issue
        id="GradleDependency"
        severity="Warning"
        message="A newer version of com.github.mik3y:usb-serial-for-android than 3.10.0 is available: 3.11.0"
        category="Correctness"
        priority="4"
        summary="Obsolete Gradle Dependency"
        explanation="This detector looks for usages of libraries where the version you are using is not the current stable release."
        errorLine1="    implementation &apos;com.github.mik3y:usb-serial-for-android:3.10.0&apos;"
        errorLine2="                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~">
        <location
            file="{project}/build.gradle"
            line="25"
            column="20"/>
    </issue>

    <issue
        id="UnusedResources"
        severity="Warning"
        message="The resource `R.array.unused_baud_rates` appears to be unused"
        category="Performance"
        priority="3"
        summary="Unused resources"
        explanation="Unused resources make applications larger and slow down builds."
        errorLine1="    &lt;string-array name=&quot;unused_baud_rates&quot;>"
        errorLine2="                  ~~~~~~~~~~~~~~~~~~~~~~~~">
        <location
            file="{project}/res/values/arrays.xml"
            line="3"
            column="19"/>
    </issue>

    <issue
        id="LintBaseline"
        severity="Information"
        message="1 warning was filtered out because it is listed in the baseline file, lint-baseline.xml"
        category="Lint"
        priority="10"
        summary="Baseline Applied"
        explanation="Lint can be configured with a &quot;baseline&quot;; a set of current issues found in a codebase, which future runs of lint will silently ignore.">
        <location
            file="{project}/lint-baseline.xml"/>
    </issue>

</issues>
"""

# androiddeployqt rewrites the placeholders in its copy of AndroidManifest.xml.
SOURCE_MANIFEST = "<manifest>\n    <!-- %%INSERT_PERMISSIONS -->\n    <application/>\n</manifest>\n"
DEPLOYED_MANIFEST = (
    "<manifest>\n"
    '    <uses-permission android:name="android.permission.INTERNET" />\n'
    '    <uses-permission android:name="android.permission.CAMERA" />\n'
    "    <application/>\n"
    "</manifest>\n"
)
JAVA_SOURCE = "package org.mavlink.qgroundcontrol;\n\nclass QGCUsbSerialManager {}\n"
GRADLE_SOURCE = (
    "dependencies {\n    implementation 'com.github.mik3y:usb-serial-for-android:3.10.0'\n}\n"
)

GRADLE_LINT_FAILURE = textwrap.dedent("""\
    > Task :lintAnalyzeRelease
    > Task :lintReportRelease
    Wrote HTML report to file:///home/runner/_work/_temp/build/android-build/build/reports/lint-results-release.html
    > Task :lintRelease FAILED

    FAILURE: Build failed with an exception.

    * What went wrong:
    Execution failed for task ':lintRelease'.
    > Lint found errors in the project; aborting build.

      Fix the issues identified by lint, or create a baseline to see only new errors.

    * Try:
    > Run with --stacktrace option to get the stack trace.

    BUILD FAILED in 1m 12s
""")

# A Gradle failure before lint ran, so no report exists.
GRADLE_RESOLVE_FAILURE = textwrap.dedent("""\
    > Task :lintVitalAnalyzeRelease FAILED

    FAILURE: Build failed with an exception.

    * What went wrong:
    Execution failed for task ':lintVitalAnalyzeRelease'.
    > Could not resolve all files for configuration ':releaseRuntimeClasspath'.
       > Could not find com.github.mik3y:usb-serial-for-android:3.10.0.

    * Try:
    > Run with --info option to get more log output.

    BUILD FAILED in 9s
""")


class Tree:
    """A checkout, its CMake build directory, and the Gradle project androiddeployqt generated."""

    def __init__(self, root: Path) -> None:
        self.repo = root / "repo"
        self.package = self.repo / "android"
        self.build = root / "build"
        self.project = self.build / "android-build"
        self.qt = root / "Qt" / "6.10.1" / "android_arm64_v8a" / "src" / "android" / "java"
        self.java = Path("src", "org", "mavlink", "qgroundcontrol", "QGCUsbSerialManager.java")
        for base in (self.package, self.project):
            self.write(base / self.java, JAVA_SOURCE)
            self.write(base / "build.gradle", GRADLE_SOURCE)
        self.write(self.package / "AndroidManifest.xml", SOURCE_MANIFEST)
        self.write(self.project / "AndroidManifest.xml", DEPLOYED_MANIFEST)
        self.write(self.qt / "QtActivity.java", "class QtActivity {}\n")
        self.gradlew = self.project / "gradlew"
        self.write(self.gradlew, "#!/bin/sh\n")
        self.gradlew.chmod(0o755)
        self.write(
            self.build / "CMakeCache.txt",
            f"QGC_ANDROID_PACKAGE_SOURCE_DIR:PATH={self.package}\n",
        )

    @staticmethod
    def write(path: Path, text: str) -> Path:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def report(self, text: str = LINT_REPORT) -> str:
        return text.replace("{project}", str(self.project)).replace("{qt}", str(self.qt))


@pytest.fixture
def tree(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Tree:
    layout = Tree(tmp_path)
    monkeypatch.setattr(android_lint, "REPO_ROOT", layout.repo)
    return layout


@pytest.fixture
def gradle(tree: Tree, monkeypatch: pytest.MonkeyPatch):
    """Replace Gradle: write *output* to the log and *report* where AGP would, then exit."""
    calls: list[tuple[list[str], Path, Path]] = []

    def configure(
        *, exit_code: int = 1, output: str = GRADLE_LINT_FAILURE, report: str | None = LINT_REPORT
    ) -> list[tuple[list[str], Path, Path]]:
        def run_tee(cmd, output_file, *, cwd, **_):
            calls.append((list(cmd), Path(output_file), Path(cwd)))
            Path(output_file).write_text(output, encoding="utf-8")
            if report is not None:
                tree.write(tree.project / android_lint.DEFAULT_REPORT, tree.report(report))
            return exit_code

        monkeypatch.setattr(android_lint, "run_tee", run_tee)
        return calls

    return configure


def _annotations(out: str) -> list[str]:
    return [line for line in out.splitlines() if line.startswith(("::error", "::warning"))]


def _summary(path: Path) -> str:
    return re.sub(r"\| Duration \| [^|]+ \|", "| Duration | <duration> |", path.read_text())


def test_parse_report_reads_errors_at_their_primary_location_and_counts_warnings(tree: Tree):
    path = tree.write(tree.project / android_lint.DEFAULT_REPORT, tree.report())

    report = android_lint.parse_report(path)

    assert report.errors == [
        android_lint.LintIssue(
            id="DuplicateActivity",
            severity="Fatal",
            message="Duplicate registration for activity `org.mavlink.qgroundcontrol.QGCActivity`",
            file=f"{tree.project}/AndroidManifest.xml",
            line=96,
            col=10,
        ),
        android_lint.LintIssue(
            id="NewApi",
            severity="Error",
            message=(
                "Call requires API level 33 (current min is 28): "
                "`android.content.Context#registerReceiver`"
            ),
            file=f"{tree.project}/{tree.java.as_posix()}",
            line=358,
            col=21,
        ),
        android_lint.LintIssue(
            id="NewApi",
            severity="Error",
            message="Call requires API level 30 (current min is 28): `android.view.Display#getMode`",
            file=f"{tree.qt}/QtActivity.java",
            line=120,
            col=37,
        ),
    ]
    # Information findings such as LintBaseline count as neither.
    assert report.warnings == 2


def test_parse_report_keeps_an_error_without_a_location(tmp_path: Path):
    path = Tree.write(
        tmp_path / "lint.xml",
        '<issues format="6"><issue id="LintError" severity="Error" message="Lint crashed"/></issues>',
    )

    assert android_lint.parse_report(path).errors == [
        android_lint.LintIssue(id="LintError", severity="Error", message="Lint crashed")
    ]


def test_parse_report_rejects_entity_declarations(tmp_path: Path):
    path = Tree.write(
        tmp_path / "lint.xml",
        '<!DOCTYPE issues [<!ENTITY x "y">]><issues format="6">&x;</issues>',
    )

    with pytest.raises(android_lint.XMLParseError):
        android_lint.parse_report(path)


def test_find_report_prefers_agps_default_location(tree: Tree):
    nested = tree.write(tree.project / "app" / android_lint.DEFAULT_REPORT, "<issues/>")
    default = tree.write(tree.project / android_lint.DEFAULT_REPORT, "<issues/>")
    os.utime(nested, (nested.stat().st_atime, default.stat().st_mtime + 5))

    assert android_lint.find_report(tree.project, since=0) == default


def test_find_report_falls_back_to_the_newest_report_in_the_project(tree: Tree):
    older = tree.write(tree.project / "lib" / android_lint.DEFAULT_REPORT, "<issues/>")
    newer = tree.write(tree.project / "app" / android_lint.DEFAULT_REPORT, "<issues/>")
    os.utime(older, (older.stat().st_atime, newer.stat().st_mtime - 5))

    assert android_lint.find_report(tree.project, since=0) == newer


def test_find_report_ignores_reports_older_than_the_run(tree: Tree):
    stale = tree.write(tree.project / android_lint.DEFAULT_REPORT, "<issues/>")

    since = stale.stat().st_mtime + android_lint.MTIME_SLACK_SECONDS + 1
    assert android_lint.find_report(tree.project, since=since) is None
    # Coarse filesystem timestamps can predate the run's start slightly.
    since = stale.stat().st_mtime + android_lint.MTIME_SLACK_SECONDS - 1
    assert android_lint.find_report(tree.project, since=since) == stale


def test_package_source_dir_comes_from_the_cmake_cache(tree: Tree):
    # A custom build's merged package template lives in the build directory.
    merged = tree.build / "custom" / "android"
    tree.write(tree.build / "CMakeCache.txt", f"QGC_ANDROID_PACKAGE_SOURCE_DIR:PATH={merged}\n")

    assert android_lint.package_source_dir(tree.project) == merged


def test_package_source_dir_defaults_to_the_checkouts_android_directory(tmp_path: Path, tree: Tree):
    assert android_lint.package_source_dir(tmp_path / "elsewhere" / "android-build") == (
        tree.repo / "android"
    )


@pytest.mark.parametrize("relative", [False, True])
def test_unchanged_copies_map_to_their_source_with_lines(tree: Tree, relative: bool):
    file = tree.java.as_posix() if relative else str(tree.project / tree.java)

    assert android_lint.checkout_location(
        file, project_dir=tree.project, package_dir=tree.package
    ) == (f"android/{tree.java.as_posix()}", True)


def test_changed_copies_map_to_their_source_without_lines(tree: Tree):
    assert android_lint.checkout_location(
        str(tree.project / "AndroidManifest.xml"),
        project_dir=tree.project,
        package_dir=tree.package,
    ) == ("android/AndroidManifest.xml", False)


def test_files_without_a_checkout_source_do_not_map(tree: Tree):
    generated = tree.write(tree.project / "res" / "values" / "libs.xml", "<resources/>\n")
    for file in (generated, tree.qt / "QtActivity.java", tree.project):
        assert android_lint.checkout_location(
            str(file), project_dir=tree.project, package_dir=tree.package
        ) == ("", False)


def test_files_already_in_the_checkout_map_to_themselves(tree: Tree):
    file = tree.write(tree.repo / "custom" / "android" / "Custom.java", "class Custom {}\n")

    assert android_lint.checkout_location(
        str(file), project_dir=tree.project, package_dir=tree.package
    ) == ("custom/android/Custom.java", True)


def test_lint_errors_become_annotations_and_the_summary(tree: Tree, gradle, gh_summary, capsys):
    calls = gradle()

    assert android_lint.main(["--project-dir", str(tree.project)]) == 1

    assert calls == [
        (
            [str(tree.gradlew), "lintRelease", "--no-daemon"],
            tree.build / android_lint.LOG_NAME,
            tree.project,
        )
    ]
    assert _annotations(capsys.readouterr().out) == [
        f"::error file=android/AndroidManifest.xml,title=DuplicateActivity::{tree.project}"
        "/AndroidManifest.xml:96:10: Duplicate registration for activity "
        "`org.mavlink.qgroundcontrol.QGCActivity`",
        f"::error file=android/{tree.java.as_posix()},line=358,col=21,title=NewApi::"
        "Call requires API level 33 (current min is 28): `android.content.Context#registerReceiver`",
        f"::error title=NewApi::{tree.qt}/QtActivity.java:120:37: "
        "Call requires API level 30 (current min is 28): `android.view.Display#getMode`",
    ]
    assert _summary(gh_summary) == textwrap.dedent(f"""\
        ### Android Lint

        | Item | Value |
        | --- | --- |
        | Result | failed |
        | Duration | <duration> |
        | Errors | 3 |
        | Warnings | 2 |

        <details><summary>First errors</summary>

        ```
        {tree.project}/AndroidManifest.xml:96:10: Fatal: Duplicate registration for activity `org.mavlink.qgroundcontrol.QGCActivity` [DuplicateActivity]
        android/{tree.java.as_posix()}:358:21: Error: Call requires API level 33 (current min is 28): `android.content.Context#registerReceiver` [NewApi]
        {tree.qt}/QtActivity.java:120:37: Error: Call requires API level 30 (current min is 28): `android.view.Display#getMode` [NewApi]
        ```

        </details>

    """)


def test_only_the_first_errors_are_annotated(tree: Tree, gradle, gh_summary, capsys):
    count = MAX_ERRORS + 2
    issues = "".join(
        f'<issue id="NewApi" severity="Error" message="Call {index}">'
        f'<location file="{{project}}/{tree.java.as_posix()}" line="{index + 1}"/></issue>'
        for index in range(count)
    )
    gradle(report=f'<issues format="6">{issues}</issues>')

    assert android_lint.main(["--project-dir", str(tree.project)]) == 1

    annotations = _annotations(capsys.readouterr().out)
    assert len(annotations) == MAX_ERRORS
    assert annotations[-1].endswith(f"line={MAX_ERRORS},title=NewApi::Call {MAX_ERRORS - 1}")
    summary = gh_summary.read_text()
    assert f"| Errors | {count} |" in summary
    assert f"First {MAX_ERRORS} of {count} errors:" in summary
    assert f"Call {MAX_ERRORS - 1} [NewApi]" in summary
    assert f"Call {MAX_ERRORS} [NewApi]" not in summary


def test_a_clean_run_summarizes_warnings_without_annotations(
    tree: Tree, gradle, gh_summary, capsys
):
    report = re.sub(r'severity="(Fatal|Error)"', 'severity="Warning"', LINT_REPORT)
    gradle(exit_code=0, output="BUILD SUCCESSFUL in 58s\n", report=report)

    assert android_lint.main(["--project-dir", str(tree.project)]) == 0

    assert _annotations(capsys.readouterr().out) == []
    assert _summary(gh_summary) == textwrap.dedent("""\
        ### Android Lint

        | Item | Value |
        | --- | --- |
        | Result | passed |
        | Duration | <duration> |
        | Errors | 0 |
        | Warnings | 5 |

    """)


def test_a_clean_run_without_a_report_warns(tree: Tree, gradle, gh_summary, capsys):
    gradle(exit_code=0, output="BUILD SUCCESSFUL in 58s\n", report=None)

    assert android_lint.main(["--project-dir", str(tree.project)]) == 0

    assert _annotations(capsys.readouterr().out) == [
        "::warning::Android Lint passed but wrote no readable lint-results-release.xml in "
        f"{tree.project}"
    ]
    assert "| Result | passed |" in gh_summary.read_text()


def test_without_a_report_gradles_errors_are_annotated(tree: Tree, gradle, gh_summary, capsys):
    gradle(output=GRADLE_RESOLVE_FAILURE, report=None)

    assert android_lint.main(["--project-dir", str(tree.project)]) == 1

    message = (
        "Execution failed for task ':lintVitalAnalyzeRelease'.%0A"
        "> Could not resolve all files for configuration ':releaseRuntimeClasspath'.%0A"
        "   > Could not find com.github.mik3y:usb-serial-for-android:3.10.0."
    )
    assert _annotations(capsys.readouterr().out) == [
        f"::error title=Gradle build failed::{message}"
    ]
    summary = _summary(gh_summary)
    assert "| Errors |" not in summary
    assert "| Warnings |" not in summary
    assert "* What went wrong:\nExecution failed for task ':lintVitalAnalyzeRelease'." in summary


def test_a_stale_report_is_not_mistaken_for_this_runs(tree: Tree, gradle, gh_summary, capsys):
    stale = tree.write(tree.project / android_lint.DEFAULT_REPORT, tree.report())
    os.utime(stale, (0, 0))
    gradle(output=GRADLE_RESOLVE_FAILURE, report=None)

    assert android_lint.main(["--project-dir", str(tree.project)]) == 1

    assert [line.split("::")[1] for line in _annotations(capsys.readouterr().out)] == [
        "error title=Gradle build failed"
    ]
    assert "DuplicateActivity" not in gh_summary.read_text()


def test_an_unreadable_report_falls_back_to_gradles_errors(tree: Tree, gradle, capsys):
    gradle(report="<issues><issue")

    assert android_lint.main(["--project-dir", str(tree.project)]) == 1

    warning, error = _annotations(capsys.readouterr().out)
    assert warning.startswith("::warning::Cannot read the lint report ")
    assert error.startswith(
        "::error title=Gradle build failed::Execution failed for task ':lintRelease'."
    )


def test_a_failure_without_a_recognized_error_still_annotates(tree: Tree, gradle, capsys):
    gradle(exit_code=137, output="Killed\n", report=None)

    assert android_lint.main(["--project-dir", str(tree.project)]) == 137

    assert _annotations(capsys.readouterr().out) == [
        "::error title=Android Lint failed::Android Lint failed with exit code 137; no error "
        f"recognized in {tree.build / android_lint.LOG_NAME}"
    ]


def test_options_override_the_log_and_package_source(tree: Tree, gradle, tmp_path: Path, capsys):
    calls = gradle()
    log = tmp_path / "logs" / "lint.log"
    log.parent.mkdir()

    assert (
        android_lint.main(
            [
                "--project-dir",
                str(tree.project),
                "--output-file",
                str(log),
                "--package-source-dir",
                str(tmp_path / "elsewhere"),
            ]
        )
        == 1
    )

    assert calls[0][1] == log
    assert not any("file=android/" in line for line in _annotations(capsys.readouterr().out))


def test_a_reporting_failure_keeps_gradles_exit_code(tree: Tree, gradle, monkeypatch, capsys):
    gradle(exit_code=1)

    def broken(*_args, **_kwargs):
        raise RuntimeError("boom")

    monkeypatch.setattr(android_lint, "report_results", broken)

    assert android_lint.main(["--project-dir", str(tree.project)]) == 1
    assert _annotations(capsys.readouterr().out) == [
        "::warning::Cannot report the Android Lint results: boom"
    ]


@pytest.mark.parametrize("problem", ["missing", "not executable", "no project"])
def test_a_missing_gradle_wrapper_fails_like_the_inline_check(
    tree: Tree, gradle, gh_summary, capsys, problem: str
):
    calls = gradle()
    project = tree.project
    if problem == "missing":
        tree.gradlew.unlink()
    elif problem == "not executable":
        tree.gradlew.chmod(0o644)
    else:
        project = tree.build / "android-build-QGroundControl"

    assert android_lint.main(["--project-dir", str(project)]) == 1

    out = capsys.readouterr().out
    assert _annotations(out) == [f"::error::Gradle wrapper not found in {project}"]
    if problem != "no project":
        assert "AndroidManifest.xml\nbuild.gradle\n" in out
    assert calls == []
    assert gh_summary.read_text() == (
        "### Android Lint\n\n| Item | Value |\n| --- | --- |\n| Result | failed |\n\n"
        f"Gradle wrapper not found in `{project}`\n\n"
    )


def test_gradles_output_streams_and_its_exit_code_is_returned(tree: Tree, gh_summary, capfd):
    """The real run_tee runs the wrapper in the project and tees its output to the log."""
    tree.gradlew.write_text(
        '#!/bin/sh\necho "args: $*"\necho "cwd: $(pwd)"\nexit 3\n', encoding="utf-8"
    )

    assert android_lint.main(["--project-dir", str(tree.project)]) == 3

    out = capfd.readouterr().out
    log = (tree.build / android_lint.LOG_NAME).read_text(encoding="utf-8")
    expected = f"args: lintRelease --no-daemon\ncwd: {tree.project.resolve()}\n"
    assert log == expected
    assert expected in out
    assert "| Result | failed |" in gh_summary.read_text()
