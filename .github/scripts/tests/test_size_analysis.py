"""Core contracts for binary size analysis."""

from __future__ import annotations

from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from _helpers import completed
from size_analysis import BinaryAnalyzer

if TYPE_CHECKING:
    from pathlib import Path


def test_binary_analyzer_requires_a_file_and_reports_its_size(tmp_path: Path) -> None:
    with pytest.raises(FileNotFoundError):
        BinaryAnalyzer(tmp_path / "missing")

    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    assert BinaryAnalyzer(binary).get_binary_size() == 4


def test_external_tool_failures_degrade_cleanly(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    for result, expected in ((completed(stdout="a\nb\n"), 2), (completed(returncode=1), 0)):
        with patch("size_analysis.run_captured", return_value=result):
            assert analyzer.get_symbol_count() == expected
    with patch("size_analysis.run_captured", side_effect=FileNotFoundError):
        assert analyzer.get_section_sizes() == "Section sizes unavailable"


def test_metrics_json_preserves_names_units_and_values(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"x")
    metrics = BinaryAnalyzer(binary).generate_metrics_json(4, 3, 42)
    assert metrics == [
        {"name": "Binary Size", "unit": "bytes", "value": 4},
        {"name": "Stripped Size", "unit": "bytes", "value": 3},
        {"name": "Symbol Count", "unit": "symbols", "value": 42},
    ]
