"""Cobertura parsing and report-generation contracts."""

from __future__ import annotations

import sys
from typing import TYPE_CHECKING

from coverage_comment import coverage_badge, delta_str, main, parse_coverage

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_parse_coverage_supports_root_and_package_rates(tmp_path: Path) -> None:
    coverage = tmp_path / "coverage.xml"
    cases = [
        ('<coverage line-rate="0.75" branch-rate="0.50"/>', {"line": 75.0, "branch": 50.0}),
        (
            '<coverage><packages><package line-rate="0.62" branch-rate="0.40"/></packages></coverage>',
            {"line": 62.0, "branch": 40.0},
        ),
    ]
    for content, expected in cases:
        coverage.write_text(content)
        assert parse_coverage(str(coverage)) == expected


def test_parse_coverage_rejects_invalid_or_unsafe_xml(tmp_path: Path) -> None:
    coverage = tmp_path / "coverage.xml"
    for content in (
        "<coverage",
        '<!DOCTYPE coverage [<!ENTITY xxe SYSTEM "file:///etc/passwd">]><coverage line-rate="0.75"/>',
    ):
        coverage.write_text(content)
        assert parse_coverage(str(coverage)) is None


def test_coverage_labels_and_deltas() -> None:
    for coverage, expected in ((85.0, "Good"), (70.0, "Fair"), (30.0, "Low")):
        assert coverage_badge(coverage) == expected
    for old, new, expected in (
        (None, 70.0, "N/A"),
        (50.0, 55.0, "+5.00%"),
        (55.0, 50.0, "-5.00%"),
        (55.0, 55.0, "No change"),
    ):
        assert delta_str(old, new) == expected


def _run_report(
    monkeypatch: pytest.MonkeyPatch,
    coverage: Path,
    output: Path,
    baseline: Path | None = None,
) -> str:
    argv = ["coverage_comment.py", "--coverage-xml", str(coverage), "--output", str(output)]
    if baseline is not None:
        argv.extend(["--baseline-xml", str(baseline)])
    monkeypatch.setattr(sys, "argv", argv)
    assert main() == 0
    return output.read_text()


def test_report_handles_missing_coverage(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    text = _run_report(monkeypatch, tmp_path / "missing.xml", tmp_path / "comment.md")
    assert "Coverage data not available" in text


def test_report_includes_available_baseline_deltas(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    coverage = tmp_path / "coverage.xml"
    baseline = tmp_path / "baseline.xml"
    output = tmp_path / "comment.md"
    baseline.write_text('<coverage line-rate="0.70" branch-rate="0.50"/>')

    cases = [
        (
            '<coverage line-rate="0.80" branch-rate="0.60"/>',
            ["| Good Lines | 80.00% | +10.00% |", "| Fair Branches | 60.00% | +10.00% |"],
            [],
        ),
        ('<coverage line-rate="0.80"/>', ["Lines"], ["Branches"]),
        (
            '<coverage line-rate="0.80" branch-rate="0.00"/>',
            ["| Low Branches | 0.00% | -50.00% |"],
            [],
        ),
    ]
    for xml, expected, absent in cases:
        coverage.write_text(xml)
        text = _run_report(monkeypatch, coverage, output, baseline)
        for value in expected:
            assert value in text
        for value in absent:
            assert value not in text
