"""Contracts for shared Cobertura metrics parsing."""

from __future__ import annotations

import pytest
from common.cobertura import CoberturaError, CoberturaMetrics, read_cobertura


def test_reads_root_and_package_metrics(tmp_path) -> None:
    report = tmp_path / "coverage.xml"
    cases = [
        (
            '<coverage line-rate="0.75" branch-rate="0.50" lines-valid="100" lines-covered="75"/>',
            CoberturaMetrics(75.0, 50.0, 100, 75),
        ),
        (
            '<coverage><packages><package line-rate="0.62" branch-rate="0.40"/>'
            "</packages></coverage>",
            CoberturaMetrics(62.0, 40.0, 0, 0),
        ),
        ('<coverage line-rate="0.80"/>', CoberturaMetrics(80.0, None, 0, 0)),
    ]
    for content, expected in cases:
        report.write_text(content)
        assert read_cobertura(report) == expected


def test_rejects_invalid_unsafe_or_metric_free_reports(tmp_path) -> None:
    report = tmp_path / "coverage.xml"
    for content in (
        "<coverage",
        '<!DOCTYPE coverage [<!ENTITY xxe SYSTEM "file:///etc/passwd">]>'
        '<coverage line-rate="0.75"/>',
        "<coverage/>",
    ):
        report.write_text(content)
        with pytest.raises(CoberturaError):
            read_cobertura(report)
