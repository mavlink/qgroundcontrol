"""Tests for tools/coverage.py."""

from __future__ import annotations

import subprocess

import coverage
import pytest
from coverage import build_step_summary


def test_build_step_summary_includes_metrics() -> None:
    markdown = build_step_summary(
        "lines: 42.0% (42 out of 100)\nbranches: 10.0% (1 out of 10)\n", "report-only"
    )
    assert "| Lines | 42.0% (42 out of 100) |" in markdown
    assert "| Branches | 10.0% (1 out of 10) |" in markdown


def test_generate_report_preserves_failed_command_output(monkeypatch, tmp_path, capsys) -> None:
    result = subprocess.CompletedProcess(
        args=["cmake"], returncode=64, stdout="gcov stdout\n", stderr="gcov stderr\n"
    )
    monkeypatch.setattr(coverage, "run_captured", lambda _command: result)
    log_file = tmp_path / "coverage-output.txt"

    with pytest.raises(subprocess.CalledProcessError):
        coverage.generate_report(tmp_path, xml_only=False, log_file=log_file)

    captured = capsys.readouterr()
    assert captured.out.endswith("gcov stdout\n")
    assert captured.err == "gcov stderr\n"
    assert log_file.read_text(encoding="utf-8") == "gcov stdout\ngcov stderr\n"


def test_configure_build_delegates_to_linux_coverage_preset(monkeypatch, tmp_path) -> None:
    configured = []
    monkeypatch.setattr(coverage, "configure", lambda config: configured.append(config) or 0)

    coverage.configure_build(tmp_path, tmp_path / "build-coverage")

    assert len(configured) == 1
    assert configured[0].preset == "Linux-coverage"
    assert configured[0].coverage is True
    assert configured[0].source_dir == tmp_path
