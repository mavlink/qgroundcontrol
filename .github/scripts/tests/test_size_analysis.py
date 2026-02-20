"""Tests for size_analysis.py."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path
from unittest.mock import patch

from size_analysis import BinaryAnalyzer, write_github_output


def test_binary_analyzer_requires_existing_file(tmp_path: Path) -> None:
    missing = tmp_path / "missing.bin"
    try:
        BinaryAnalyzer(missing)
        assert False, "Expected FileNotFoundError"
    except FileNotFoundError:
        pass


def test_get_binary_size(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)
    assert analyzer.get_binary_size() == 4


def test_get_symbol_count_success(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch(
        "size_analysis.subprocess.run",
        return_value=subprocess.CompletedProcess(args=["nm"], returncode=0, stdout="a\nb\n", stderr=""),
    ):
        assert analyzer.get_symbol_count() == 2


def test_get_symbol_count_failure_returns_zero(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch(
        "size_analysis.subprocess.run",
        return_value=subprocess.CompletedProcess(args=["nm"], returncode=1, stdout="", stderr="err"),
    ):
        assert analyzer.get_symbol_count() == 0


def test_get_section_sizes_unavailable(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch("size_analysis.subprocess.run", side_effect=FileNotFoundError):
        assert analyzer.get_section_sizes() == "Section sizes unavailable"


def test_generate_metrics_json_with_patched_methods(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch.object(BinaryAnalyzer, "get_stripped_size", return_value=3), patch.object(
        BinaryAnalyzer, "get_symbol_count", return_value=42,
    ):
        metrics = analyzer.generate_metrics_json()

    names = [entry["name"] for entry in metrics]
    assert names == ["Binary Size", "Stripped Size", "Symbol Count"]
    assert metrics[0]["value"] == 4
    assert metrics[1]["value"] == 3
    assert metrics[2]["value"] == 42


def test_write_github_output(tmp_path: Path) -> None:
    output_file = tmp_path / "github_output.txt"
    with patch.dict(os.environ, {"GITHUB_OUTPUT": str(output_file)}):
        write_github_output(10, 8, 100)

    content = output_file.read_text(encoding="utf-8")
    assert "binary_size=10" in content
    assert "stripped_size=8" in content
    assert "symbol_count=100" in content
