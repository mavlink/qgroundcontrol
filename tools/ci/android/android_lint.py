#!/usr/bin/env python3
"""Run Android Lint on the Gradle project that androiddeployqt generated and report its errors.

Runs ``gradlew lintRelease``, teeing Gradle's output to a log, then reads lint's XML report: the
first errors (severity Error or Fatal) become error annotations titled with the lint issue id, at
the repository's Android package sources that androiddeployqt copied into the project. Without a
report, the errors in Gradle's output are annotated instead. Every run writes the job summary's
Android Lint section: result, duration, error and warning counts, and the first errors. Exits with
Gradle's exit code.
"""

from __future__ import annotations

import argparse
import filecmp
import os
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.cmake import read_cache_dict
from common.format import format_count, format_duration
from common.gh_actions import gh_error, gh_warning, write_step_summary
from common.markdown import code_block, code_span, summary_section
from common.paths import REPO_ROOT
from common.proc import run_tee
from common.xml import XMLParseError, xml_parse
from qgc_tools.build_errors import MAX_ERRORS, parse_build_failure, repo_path

LINT_TASK = "lintRelease"
REPORT_NAME = "lint-results-release.xml"
# Where AGP writes the report, relative to the Gradle project.
DEFAULT_REPORT = Path("build", "reports", REPORT_NAME)
LOG_NAME = "android-lint.log"
ERROR_SEVERITIES = frozenset({"Error", "Fatal"})
# Some filesystems store modification times with up to 2 s granularity.
MTIME_SLACK_SECONDS = 2.0


@dataclass(frozen=True)
class LintIssue:
    """One lint finding at its primary location, as lint's XML report gives it."""

    id: str
    severity: str
    message: str
    file: str = ""
    line: int | None = None
    col: int | None = None


@dataclass
class LintReport:
    """A lint report's errors (severity Error or Fatal), in report order, and its warning count."""

    errors: list[LintIssue] = field(default_factory=list)
    warnings: int = 0


def _number(value: str | None) -> int | None:
    return int(value) if value and value.isdigit() else None


def _location(file: str, line: int | None, col: int | None) -> str:
    numbers = [str(number) for number in (line, col) if number is not None]
    return ":".join([file, *numbers]) if file else ""


def parse_report(path: Path) -> LintReport:
    """Read the errors and the warning count from lint's XML report at *path*."""
    report = LintReport()
    for issue in xml_parse(path).iter("issue"):
        severity = issue.get("severity", "")
        if severity == "Warning":
            report.warnings += 1
        elif severity in ERROR_SEVERITIES:
            location = issue.find("location")
            attributes = location.attrib if location is not None else {}
            report.errors.append(
                LintIssue(
                    id=issue.get("id", ""),
                    severity=severity,
                    message=issue.get("message", ""),
                    file=attributes.get("file", ""),
                    line=_number(attributes.get("line")),
                    col=_number(attributes.get("column")),
                )
            )
    return report


def find_report(project_dir: Path, since: float) -> Path | None:
    """Return the lint XML report written since *since*, or None.

    AGP writes it to the linted module's build/reports/; a report of that name anywhere else in
    the project also counts, the newest first. An older report is stale.
    """

    def fresh(path: Path) -> bool:
        return path.is_file() and path.stat().st_mtime >= since - MTIME_SLACK_SECONDS

    default = project_dir / DEFAULT_REPORT
    if fresh(default):
        return default
    reports = [path for path in project_dir.rglob(REPORT_NAME) if fresh(path)]
    return max(reports, key=lambda path: path.stat().st_mtime, default=None)


def package_source_dir(project_dir: Path) -> Path:
    """Return the package source directory that configure recorded beside the project.

    CMake passes QGC_ANDROID_PACKAGE_SOURCE_DIR (default: the checkout's android/) to Qt as the
    target's QT_ANDROID_PACKAGE_SOURCE_DIR, which androiddeployqt copies into the project.
    """
    cache = read_cache_dict(project_dir.parent / "CMakeCache.txt")
    return Path(cache.get("QGC_ANDROID_PACKAGE_SOURCE_DIR") or REPO_ROOT / "android")


def checkout_location(file: str, *, project_dir: Path, package_dir: Path) -> tuple[str, bool]:
    """Return the checkout path of the source that *file* was copied from, and whether lines match.

    androiddeployqt rewrites placeholders in its copy of AndroidManifest.xml, so line numbers hold
    only for an unchanged copy. A file already in the checkout maps to itself; anything else, such
    as Qt's own Java sources, maps to ``("", False)``.
    """
    path = Path(file)
    if not path.is_absolute():
        path = project_dir / path
    path = path.resolve()
    try:
        relative = path.relative_to(project_dir.resolve())
    except ValueError:
        checkout = repo_path(str(path), bases=(REPO_ROOT,), repo_root=REPO_ROOT)
        return checkout, bool(checkout)
    source = package_dir / relative
    if not (path.is_file() and source.is_file()):
        return "", False
    checkout = repo_path(str(source), bases=(REPO_ROOT,), repo_root=REPO_ROOT)
    return checkout, bool(checkout) and filecmp.cmp(path, source, shallow=False)


def annotate(
    title: str,
    message: str,
    file: str,
    line: int | None,
    col: int | None,
    *,
    project_dir: Path,
    package_dir: Path,
) -> str:
    """Annotate one error at its checkout source; return the location to show in the summary.

    An error whose source is outside the checkout names its location in the message instead, and
    one in a changed copy is annotated on its source file without a line.
    """
    checkout, exact = (
        checkout_location(file, project_dir=project_dir, package_dir=package_dir)
        if file
        else ("", False)
    )
    location = _location(checkout, line, col) if exact else _location(file, line, col)
    gh_error(
        message if exact or not file else f"{location}: {message}",
        title=title,
        file=checkout,
        line=line if exact else None,
        col=col if exact else None,
    )
    return location


def _read_log(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def _first_errors(entries: list[str], total: int) -> str:
    if not entries:
        return ""
    separator = "\n\n" if any("\n" in entry for entry in entries) else "\n"
    heading = f"First {len(entries)} of {total} errors:\n\n" if total > len(entries) else ""
    return heading + code_block(separator.join(entries))


def report_results(
    exit_code: int, log: Path, report_path: Path | None, *, project_dir: Path, package_dir: Path
) -> tuple[LintReport | None, str]:
    """Annotate the first errors; return the parsed report and the summary's first errors.

    Without a readable report, a failure annotates the errors in Gradle's output instead.
    """
    report = None
    if report_path:
        try:
            report = parse_report(report_path)
        except (OSError, XMLParseError) as error:
            gh_warning(f"Cannot read the lint report {report_path}: {error}")
    if report and report.errors:
        entries = []
        for issue in report.errors[:MAX_ERRORS]:
            location = annotate(
                issue.id,
                issue.message,
                issue.file,
                issue.line,
                issue.col,
                project_dir=project_dir,
                package_dir=package_dir,
            )
            text = f"{issue.severity}: {issue.message} [{issue.id}]"
            entries.append(f"{location}: {text}" if location else text)
        return report, _first_errors(entries, len(report.errors))
    if exit_code == 0:
        if report is None:
            gh_warning(f"Android Lint passed but wrote no readable {REPORT_NAME} in {project_dir}")
        return report, ""
    failure = parse_build_failure(_read_log(log))
    for diagnostic in failure.errors:
        annotate(
            diagnostic.title,
            diagnostic.message,
            diagnostic.file,
            diagnostic.line,
            diagnostic.col,
            project_dir=project_dir,
            package_dir=package_dir,
        )
    if not failure.errors:
        gh_error(
            f"Android Lint failed with exit code {exit_code}; no error recognized in {log}",
            title="Android Lint failed",
        )
    entries = [diagnostic.excerpt or diagnostic.message for diagnostic in failure.errors]
    return report, _first_errors(entries, failure.error_count)


def write_summary(
    exit_code: int,
    seconds: float | None,
    report: LintReport | None,
    first_errors: str,
    *,
    note: str = "",
) -> None:
    """Write the job summary's Android Lint section."""
    section = summary_section(
        "Android Lint",
        [
            ("Result", "passed" if exit_code == 0 else "failed"),
            ("Duration", format_duration(seconds) if seconds is not None else ""),
            ("Errors", format_count(len(report.errors)) if report else ""),
            ("Warnings", format_count(report.warnings) if report else ""),
        ],
        body=note,
        collapsed=[("First errors", first_errors)],
    )
    print(section)
    write_step_summary(section)


def _print_listing(directory: Path) -> None:
    try:
        entries = sorted(directory.iterdir())
    except OSError as error:
        print(f"Cannot list {directory}: {error}", flush=True)
        return
    for entry in entries:
        print(f"{entry.name}/" if entry.is_dir() else entry.name, flush=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--project-dir",
        type=Path,
        required=True,
        help="Gradle project that androiddeployqt generated (the build directory's android-build)",
    )
    parser.add_argument(
        "--package-source-dir",
        type=Path,
        help="Android package source directory copied into the project; default: "
        "QGC_ANDROID_PACKAGE_SOURCE_DIR from the CMakeCache.txt beside the project",
    )
    parser.add_argument(
        "--output-file",
        type=Path,
        help=f"Log that captures Gradle's output; default: {LOG_NAME} beside the project",
    )
    args = parser.parse_args(argv)
    project_dir: Path = args.project_dir

    gradlew = project_dir / "gradlew"
    if not (gradlew.is_file() and os.access(gradlew, os.X_OK)):
        gh_error(f"Gradle wrapper not found in {project_dir}")
        _print_listing(project_dir)
        write_summary(
            1, None, None, "", note=f"Gradle wrapper not found in {code_span(project_dir)}"
        )
        return 1

    log = args.output_file or project_dir.parent / LOG_NAME
    since = time.time()
    start = time.monotonic()
    exit_code = run_tee([str(gradlew), LINT_TASK, "--no-daemon"], log, cwd=project_dir)
    seconds = time.monotonic() - start

    # Reporting problems only warn: the step's result is Gradle's.
    try:
        report, first_errors = report_results(
            exit_code,
            log,
            find_report(project_dir, since),
            project_dir=project_dir,
            package_dir=args.package_source_dir or package_source_dir(project_dir),
        )
        write_summary(exit_code, seconds, report, first_errors)
    except Exception as error:
        gh_warning(f"Cannot report the Android Lint results: {error}")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
