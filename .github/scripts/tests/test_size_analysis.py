"""Tests for size_analysis.py."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

import size_analysis
from size_analysis import BinaryAnalyzer


def test_binary_analyzer_requires_existing_file(tmp_path: Path) -> None:
    missing = tmp_path / "missing.bin"
    try:
        BinaryAnalyzer(missing)
        raise AssertionError("Expected FileNotFoundError")
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
        return_value=subprocess.CompletedProcess(
            args=["nm"], returncode=0, stdout="a\nb\n", stderr=""
        ),
    ):
        assert analyzer.get_symbol_count() == 2


def test_get_symbol_count_failure_returns_zero(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch(
        "size_analysis.subprocess.run",
        return_value=subprocess.CompletedProcess(
            args=["nm"], returncode=1, stdout="", stderr="err"
        ),
    ):
        assert analyzer.get_symbol_count() == 0


def test_get_section_sizes_unavailable(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch("size_analysis.subprocess.run", side_effect=FileNotFoundError):
        assert analyzer.get_section_sizes() == "Section sizes unavailable"


def test_source_build_cleans_temporary_directory(tmp_path: Path, monkeypatch) -> None:
    build_directory = None
    real_temporary_directory = tempfile.TemporaryDirectory

    def temporary_directory(**kwargs):
        nonlocal build_directory
        directory = real_temporary_directory(dir=tmp_path, **kwargs)
        build_directory = Path(directory.name)
        return directory

    monkeypatch.setattr(size_analysis.tempfile, "TemporaryDirectory", temporary_directory)
    monkeypatch.setattr(
        size_analysis,
        "run_captured",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 0, "", ""),
    )

    assert size_analysis._install_bloaty_from_source(10)
    assert build_directory is not None
    assert not build_directory.exists()


def test_generate_metrics_json_with_explicit_values(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    metrics = analyzer.generate_metrics_json(binary_size=4, stripped_size=3, symbol_count=42)

    names = [entry["name"] for entry in metrics]
    assert names == ["Binary Size", "Stripped Size", "Symbol Count"]
    assert metrics[0]["value"] == 4
    assert metrics[1]["value"] == 3
    assert metrics[2]["value"] == 42
