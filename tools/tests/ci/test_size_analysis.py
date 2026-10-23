"""Tests for size_analysis.py."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

from ci.reporting import size_analysis
from ci.reporting.size_analysis import BinaryAnalyzer
from common.build_config import load_build_config


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

    assert size_analysis.install_bloaty()
    assert build_directory is not None
    assert not build_directory.exists()
    assert commands[0][1] == str(size_analysis.APT_INSTALL_RETRY)
    assert "libprotobuf-dev" in commands[0]
    fetch = next(command for command in commands if "fetch" in command)
    assert fetch[-1] == load_build_config()["tools"]["bloaty"]["revision"]


def test_summary_lists_sizes_and_collapses_the_breakdowns(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_bytes(b"x")
    analyzer = BinaryAnalyzer(binary)
    with (
        patch.object(analyzer, "get_section_sizes", return_value=".text 1000"),
        patch.object(size_analysis.shutil, "which", return_value=None),
    ):
        summary = analyzer.generate_summary(150 * 1048576, 120 * 1048576, 98765)
    assert summary == (
        "### Binary size\n\n"
        "| Item | Value |\n| --- | --- |\n"
        "| Binary | `QGroundControl` |\n"
        "| Size | 150.00 MB (157,286,400 bytes) |\n"
        "| Stripped | 120.00 MB (125,829,120 bytes) |\n"
        "| Symbols | 98,765 |\n\n"
        "<details><summary>Section sizes</summary>\n\n```\n.text 1000\n```\n\n</details>\n\n"
    )
