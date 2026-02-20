"""Tests for coverage_comment.py."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from coverage_comment import coverage_badge, delta_str, main, parse_coverage


def _write_xml(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def test_parse_coverage_root_rates(tmp_path: Path) -> None:
    xml = tmp_path / "coverage.xml"
    _write_xml(xml, '<coverage line-rate="0.75" branch-rate="0.50"></coverage>')

    cov = parse_coverage(str(xml))
    assert cov is not None
    assert cov["line"] == 75.0
    assert cov["branch"] == 50.0


def test_parse_coverage_package_fallback(tmp_path: Path) -> None:
    xml = tmp_path / "coverage.xml"
    _write_xml(
        xml,
        "<coverage><packages><package line-rate=\"0.62\" branch-rate=\"0.40\"/></packages></coverage>",
    )

    cov = parse_coverage(str(xml))
    assert cov is not None
    assert cov["line"] == 62.0
    assert cov["branch"] == 40.0


def test_parse_coverage_invalid_xml_returns_none(tmp_path: Path) -> None:
    xml = tmp_path / "bad.xml"
    _write_xml(xml, "<coverage")
    assert parse_coverage(str(xml)) is None


def test_coverage_badge() -> None:
    assert coverage_badge(85.0) == "Good"
    assert coverage_badge(70.0) == "Fair"
    assert coverage_badge(30.0) == "Low"


def test_delta_str() -> None:
    assert delta_str(None, 70.0) == "N/A"
    assert delta_str(50.0, 55.0) == "+5.00%"
    assert delta_str(55.0, 50.0) == "-5.00%"
    assert delta_str(55.0, 55.0) == "No change"


def test_main_missing_coverage_writes_placeholder(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output = tmp_path / "coverage-comment.md"
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "coverage_comment.py",
            "--coverage-xml",
            str(tmp_path / "missing.xml"),
            "--output",
            str(output),
        ],
    )

    with pytest.raises(SystemExit) as exc:
        main()

    assert exc.value.code == 0
    text = output.read_text(encoding="utf-8")
    assert "Coverage data not available" in text


def test_main_with_baseline_writes_delta_table(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    coverage_xml = tmp_path / "coverage.xml"
    baseline_xml = tmp_path / "baseline.xml"
    output = tmp_path / "coverage-comment.md"

    _write_xml(coverage_xml, '<coverage line-rate="0.80" branch-rate="0.60"></coverage>')
    _write_xml(baseline_xml, '<coverage line-rate="0.70" branch-rate="0.50"></coverage>')

    monkeypatch.setattr(
        sys,
        "argv",
        [
            "coverage_comment.py",
            "--coverage-xml",
            str(coverage_xml),
            "--baseline-xml",
            str(baseline_xml),
            "--output",
            str(output),
        ],
    )

    main()

    text = output.read_text(encoding="utf-8")
    assert "| Good Lines | 80.00% | +10.00% |" in text
    assert "| Fair Branches | 60.00% | +10.00% |" in text
