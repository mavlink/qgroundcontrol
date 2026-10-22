"""Tests for tools/coverage.py."""

from __future__ import annotations

import json
from pathlib import Path
from unittest.mock import patch

from coverage import DEFAULT_BUILD_DIR, build_step_summary, main, parse_args, run_tests


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
    assert "| Lines | 42.0% (42 of 100) |" in markdown
    assert "| Branches | 10.0% (5 of 50) |" in markdown


def test_build_step_summary_reports_missing_data(tmp_path: Path) -> None:
    assert "Coverage data not available" in build_step_summary(tmp_path / "coverage-summary.json")


def test_default_build_dir_is_the_coverage_preset_binary_dir() -> None:
    assert Path("build/Linux-coverage") == DEFAULT_BUILD_DIR
    assert parse_args([]).build_dir == str(DEFAULT_BUILD_DIR)


def test_report_only_skips_build_and_writes_summary(tmp_path: Path, monkeypatch) -> None:
    summary = tmp_path / "summary.md"
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    with (
        patch("coverage.shutil.which", return_value="/usr/bin/gcovr"),
        patch("coverage.configure_build") as configure_build,
        patch("coverage.generate_report") as generate_report,
    ):
        assert main(["--report", "--step-summary", "--build-dir", str(tmp_path)]) == 0
    configure_build.assert_not_called()
    generate_report.assert_called_once_with(tmp_path, log_file=None)
    assert "## Code Coverage" in summary.read_text(encoding="utf-8")


def test_missing_gcovr_is_reported_once(tmp_path, capsys) -> None:
    with patch("coverage.shutil.which", return_value=None):
        assert main(["--report", "--build-dir", str(tmp_path)]) == 1
    error = capsys.readouterr().err
    assert "gcovr not found" in error
    assert error.count("Install with:") == 1


def test_clean_does_not_require_gcovr(tmp_path) -> None:
    build_dir = tmp_path / "Linux-coverage"
    build_dir.mkdir()
    with patch("coverage.shutil.which", return_value=None):
        assert main(["--clean", "--build-dir", str(build_dir)]) == 0
    assert not build_dir.exists()


def test_run_tests_uses_the_ci_label_selection(tmp_path) -> None:
    with patch("coverage.subprocess.run") as run:
        run_tests(tmp_path)
    command = run.call_args.args[0]
    assert command[command.index("--label-regex") + 1] == "Unit|Integration"
    assert command[command.index("--label-exclude") + 1] == "Flaky|Network"
    assert "--parallel" in command
