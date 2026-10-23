"""Tests for tools/coverage_report.py."""

from __future__ import annotations

import json
from pathlib import Path
from subprocess import CompletedProcess
from unittest.mock import patch

import pytest
from coverage_report import DEFAULT_BUILD_DIR, build_step_summary, main, parse_args, run_tests

from ._helpers import REPO_ROOT, get_step, load_workflow


def test_build_step_summary_reads_gcovr_summary(tmp_path: Path) -> None:
    report = tmp_path / "coverage-summary.json"
    report.write_text(
        json.dumps(
            {
                "line_total": 100,
                "line_covered": 42,
                "line_percent": 42.0,
                "branch_total": 50,
                "branch_covered": 5,
                "branch_percent": 10.0,
            }
        ),
        encoding="utf-8",
    )
    markdown = build_step_summary(report)
    assert markdown.startswith("### Coverage\n\n| Item | Value |")
    assert "| Lines | 42.0% (42 of 100) |" in markdown
    assert "| Branches | 10.0% (5 of 50) |" in markdown
    assert "Functions" not in markdown


def test_build_step_summary_reports_missing_data(tmp_path: Path) -> None:
    assert "Coverage data not available" in build_step_summary(tmp_path / "coverage-summary.json")


def test_default_build_dir_is_the_coverage_preset_binary_dir() -> None:
    assert Path("build/Linux-coverage") == DEFAULT_BUILD_DIR
    assert parse_args([]).build_dir == str(DEFAULT_BUILD_DIR)


def test_report_only_skips_build_and_writes_summary(tmp_path: Path, gh_summary: Path) -> None:
    with (
        patch("coverage_report.shutil.which", return_value="/usr/bin/gcovr"),
        patch("coverage_report.configure_build") as configure_build,
        patch("coverage_report.generate_report") as generate_report,
    ):
        assert main(["--report", "--step-summary", "--build-dir", str(tmp_path)]) == 0
    configure_build.assert_not_called()
    generate_report.assert_called_once_with(tmp_path, log_file=None)
    assert "### Coverage" in gh_summary.read_text(encoding="utf-8")


@pytest.mark.parametrize("ci", [False, True])
def test_report_fails_without_gcda_files(tmp_path, monkeypatch, capsys, ci) -> None:
    """Tests that wrote no counters fail the report instead of publishing zero coverage."""
    if ci:
        monkeypatch.setenv("GITHUB_ACTIONS", "true")
    (tmp_path / "src").mkdir()
    (tmp_path / "src" / "Vehicle.gcno").touch()
    with (
        patch("coverage_report.shutil.which", return_value="/usr/bin/gcovr"),
        patch("coverage_report.run_captured") as run_captured,
    ):
        assert main(["--report", "--build-dir", str(tmp_path)]) == 1
    run_captured.assert_not_called()
    out, err = capsys.readouterr()
    message = "No .gcda files were produced by test execution"
    if ci:
        assert f"::error::{message}" in out
    else:
        assert message in err


def test_report_runs_the_coverage_target_when_gcda_files_exist(tmp_path, capsys) -> None:
    (tmp_path / "src").mkdir()
    (tmp_path / "src" / "Vehicle.gcda").touch()
    with (
        patch("coverage_report.shutil.which", return_value="/usr/bin/gcovr"),
        patch(
            "coverage_report.run_captured",
            return_value=CompletedProcess([], 0, stdout="", stderr=""),
        ) as run_captured,
    ):
        assert main(["--report", "--build-dir", str(tmp_path)]) == 0
    run_captured.assert_called_once_with(
        ["cmake", "--build", str(tmp_path), "--target", "coverage-report"], check=False
    )
    assert "Detected 1 .gcda files" in capsys.readouterr().out


def test_coverage_job_relies_on_the_report_for_missing_gcda_files() -> None:
    steps = load_workflow("linux.yml")["jobs"]["debug-validation"]["steps"]
    assert ".gcda" not in str([step.get("run", "") for step in steps])
    report = get_step(steps, "Generate Coverage Report")
    assert "tools/coverage_report.py" in report["run"]
    assert "--report" in report["run"]


def test_missing_gcovr_is_reported_once(tmp_path, capsys) -> None:
    with patch("coverage_report.shutil.which", return_value=None):
        assert main(["--report", "--build-dir", str(tmp_path)]) == 1
    error = capsys.readouterr().err
    assert "gcovr not found" in error
    assert error.count("Install with:") == 1


def test_clean_does_not_require_gcovr(tmp_path) -> None:
    build_dir = tmp_path / "Linux-coverage"
    build_dir.mkdir()
    with patch("coverage_report.shutil.which", return_value=None):
        assert main(["--clean", "--build-dir", str(build_dir)]) == 0
    assert not build_dir.exists()


@pytest.mark.parametrize("opened", [True, False])
def test_open_shows_the_html_report_in_a_browser(tmp_path: Path, opened: bool) -> None:
    report = tmp_path / "coverage.html"
    report.touch()
    with (
        patch("coverage_report.shutil.which", return_value="/usr/bin/gcovr"),
        patch("coverage_report.generate_report"),
        patch("coverage_report.webbrowser.open", return_value=opened) as browser,
    ):
        assert main(["--report", "--open", "--build-dir", str(tmp_path)]) == int(not opened)
    browser.assert_called_once_with(report.resolve().as_uri())


def test_run_tests_runs_unit_then_integration_like_ci(tmp_path) -> None:
    with patch("coverage_report.subprocess.run", return_value=CompletedProcess([], 0)) as run:
        run_tests(tmp_path, tmp_path / "build")
    phases = []
    for call in run.call_args_list:
        command = call.args[0]
        assert command[command.index("--preset") + 1] == "Linux-coverage"
        assert command[command.index("--test-dir") + 1] == str(tmp_path / "build")
        assert "--label-exclude" not in command
        assert call.kwargs["cwd"] == tmp_path
        phases.append(
            (command[command.index("--label-regex") + 1], command[command.index("--parallel") + 1])
        )
    assert [label for label, _jobs in phases] == ["Unit", "Integration"]
    assert phases[1][1] == "2"


def test_run_tests_runs_integration_after_unit_failures(tmp_path) -> None:
    with (
        patch("coverage_report.subprocess.run", return_value=CompletedProcess([], 8)) as run,
        pytest.raises(RuntimeError, match="Unit and Integration tests failed"),
    ):
        run_tests(tmp_path, tmp_path / "build")
    assert run.call_count == 2


def test_coverage_test_preset_excludes_the_ci_labels() -> None:
    presets = {
        preset["name"]: preset
        for document in ("common", "Linux")
        for preset in json.loads(
            (REPO_ROOT / f"cmake/presets/{document}.json").read_text(encoding="utf-8")
        )["testPresets"]
    }
    preset = presets["Linux-coverage"]
    while "filter" not in preset:
        (parent,) = preset["inherits"]
        preset = presets[parent]
    assert preset["filter"]["exclude"]["label"] == "Flaky|Network"
