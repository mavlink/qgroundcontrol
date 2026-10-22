"""Tests for size_analysis.py."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

from ci.reporting import size_analysis
from ci.reporting.size_analysis import BinaryAnalyzer


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
        "ci.reporting.size_analysis.subprocess.run",
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
        "ci.reporting.size_analysis.subprocess.run",
        return_value=subprocess.CompletedProcess(
            args=["nm"], returncode=1, stdout="", stderr="err"
        ),
    ):
        assert analyzer.get_symbol_count() == 0


def test_get_section_sizes_unavailable(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"abcd")
    analyzer = BinaryAnalyzer(binary)

    with patch("ci.reporting.size_analysis.subprocess.run", side_effect=FileNotFoundError):
        assert analyzer.get_section_sizes() == "Section sizes unavailable"


def test_source_build_cleans_temporary_directory(tmp_path: Path, monkeypatch) -> None:
    build_directory = None
    real_temporary_directory = tempfile.TemporaryDirectory

    def temporary_directory(**kwargs):
        nonlocal build_directory
        directory = real_temporary_directory(dir=tmp_path, **kwargs)
        build_directory = Path(directory.name)
        return directory

    commands: list[list[str]] = []

    def run_captured(command, **kwargs):
        commands.append(command)
        return subprocess.CompletedProcess(command, 0, "", "")

    monkeypatch.setattr(size_analysis.tempfile, "TemporaryDirectory", temporary_directory)
    monkeypatch.setattr(size_analysis.shutil, "which", lambda name: None)
    monkeypatch.setattr(size_analysis, "run_captured", run_captured)

    assert size_analysis.install_bloaty(10)
    assert build_directory is not None
    assert not build_directory.exists()
    assert commands[0][1] == str(size_analysis.APT_INSTALL_RETRY)
    assert "libprotobuf-dev" in commands[0]
