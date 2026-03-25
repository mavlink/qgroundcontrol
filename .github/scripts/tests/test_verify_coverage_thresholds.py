"""Tests for verify_coverage_thresholds.py."""

from __future__ import annotations

from pathlib import Path

from verify_coverage_thresholds import main


def _write_xml(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def test_missing_file_returns_zero(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--coverage-xml", str(tmp_path / "missing.xml")],
    )
    assert main() == 0


def test_above_thresholds(tmp_path: Path, monkeypatch) -> None:
    xml = tmp_path / "coverage.xml"
    _write_xml(xml, '<coverage lines-valid="100" lines-covered="80" line-rate="0.80" branch-rate="0.60"/>')
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--coverage-xml", str(xml), "--line-threshold", "30", "--branch-threshold", "20"],
    )
    assert main() == 0


def test_below_line_threshold(tmp_path: Path, monkeypatch) -> None:
    xml = tmp_path / "coverage.xml"
    _write_xml(xml, '<coverage lines-valid="100" lines-covered="10" line-rate="0.10" branch-rate="0.50"/>')
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--coverage-xml", str(xml), "--line-threshold", "30", "--branch-threshold", "20"],
    )
    assert main() == 1


def test_below_branch_threshold(tmp_path: Path, monkeypatch) -> None:
    xml = tmp_path / "coverage.xml"
    _write_xml(xml, '<coverage lines-valid="100" lines-covered="80" line-rate="0.80" branch-rate="0.10"/>')
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--coverage-xml", str(xml), "--line-threshold", "30", "--branch-threshold", "20"],
    )
    assert main() == 1


def test_zero_lines_valid(tmp_path: Path, monkeypatch) -> None:
    xml = tmp_path / "coverage.xml"
    _write_xml(xml, '<coverage lines-valid="0" lines-covered="0" line-rate="0" branch-rate="0"/>')
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--coverage-xml", str(xml)],
    )
    assert main() == 1
