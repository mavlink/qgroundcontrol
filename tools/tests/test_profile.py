"""Tests for profile.py (debuggers/profile.py)."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path
from unittest.mock import patch, MagicMock

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent / "debuggers"))

from profile import ProfileResult, Profiler


class TestProfileResult:
    """Tests for ProfileResult dataclass."""

    def test_creation(self, tmp_path):
        """Test ProfileResult creation."""
        result = ProfileResult(
            output_file=tmp_path / "output.log",
            exit_code=0,
        )
        assert result.exit_code == 0
        assert result.timed_out is False

    def test_timed_out(self, tmp_path):
        """Test ProfileResult with timeout."""
        result = ProfileResult(
            output_file=tmp_path / "output.log",
            exit_code=124,
            timed_out=True,
        )
        assert result.timed_out is True
        assert result.exit_code == 124


class TestProfiler:
    """Tests for Profiler class."""

    @pytest.fixture
    def profiler(self, tmp_path):
        """Create a Profiler instance for testing."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "profile"
        return Profiler(
            build_dir=build_dir,
            output_dir=output_dir,
            timeout=10,
        )

    def test_timestamp_format(self, profiler):
        """Test timestamp generation."""
        ts = profiler._timestamp()
        # Format: YYYYMMDD-HHMMSS
        assert len(ts) == 15
        assert "-" in ts

    def test_check_tool_found(self, profiler):
        """Test tool check when tool exists."""
        with patch("shutil.which", return_value="/usr/bin/valgrind"):
            assert profiler._check_tool("valgrind", "apt install valgrind") is True

    def test_check_tool_not_found(self, profiler, capsys):
        """Test tool check when tool doesn't exist."""
        with patch("shutil.which", return_value=None):
            result = profiler._check_tool("valgrind", "apt install valgrind")
            assert result is False
            captured = capsys.readouterr()
            assert "not found" in captured.err

    def test_check_executable_exists(self, profiler, tmp_path):
        """Test executable check when it exists."""
        exe = profiler.build_dir / "QGroundControl"
        exe.write_text("#!/bin/bash\n")
        exe.chmod(0o755)
        assert profiler._check_executable() is True

    def test_check_executable_missing(self, profiler, capsys):
        """Test executable check when missing."""
        assert profiler._check_executable() is False
        captured = capsys.readouterr()
        assert "not found" in captured.err

    def test_output_dir_created(self, profiler, tmp_path):
        """Test output directory is created."""
        exe = profiler.build_dir / "QGroundControl"
        exe.write_text("#!/bin/bash\n")
        exe.chmod(0o755)

        with patch("shutil.which", return_value="/usr/bin/perf"), \
             patch("subprocess.run") as mock_run, \
             patch.object(Path, "read_text", return_value="1"):
            mock_run.return_value = MagicMock(returncode=0, stdout="", stderr="")
            profiler.run_perf()

        assert profiler.output_dir.exists()

    def test_extra_args_passed(self, profiler, tmp_path):
        """Test extra arguments are passed to executable."""
        profiler.extra_args = ["--arg1", "--arg2"]
        exe = profiler.build_dir / "QGroundControl"
        exe.write_text("#!/bin/bash\n")
        exe.chmod(0o755)

        with patch("shutil.which", return_value="/usr/bin/valgrind"), \
             patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            # Mock the output file
            profiler.output_dir.mkdir(parents=True)
            profiler.run_memcheck()

            # Check that extra args are in the command
            call_args = mock_run.call_args[0][0]
            assert "--arg1" in call_args
            assert "--arg2" in call_args


class TestProfilerModes:
    """Tests for different profiler modes."""

    @pytest.fixture
    def setup_profiler(self, tmp_path):
        """Set up profiler with executable."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        exe = build_dir / "QGroundControl"
        exe.write_text("#!/bin/bash\n")
        exe.chmod(0o755)

        output_dir = tmp_path / "profile"
        return Profiler(
            build_dir=build_dir,
            output_dir=output_dir,
            timeout=10,
        )

    def test_memcheck_uses_valgrind(self, setup_profiler):
        """Test memcheck mode uses valgrind."""
        with patch("shutil.which", return_value="/usr/bin/valgrind"), \
             patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            setup_profiler.run_memcheck()

            call_args = mock_run.call_args[0][0]
            assert "valgrind" in call_args
            assert "--leak-check=full" in call_args

    def test_callgrind_uses_valgrind_tool(self, setup_profiler):
        """Test callgrind mode uses valgrind with callgrind tool."""
        with patch("shutil.which", return_value="/usr/bin/valgrind"), \
             patch("subprocess.run") as mock_run, \
             patch("subprocess.Popen"):
            mock_run.return_value = MagicMock(returncode=0)
            setup_profiler.run_callgrind()

            call_args = mock_run.call_args[0][0]
            assert "--tool=callgrind" in call_args

    def test_massif_uses_valgrind_tool(self, setup_profiler):
        """Test massif mode uses valgrind with massif tool."""
        with patch("shutil.which", return_value="/usr/bin/valgrind"), \
             patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0, stdout="", stderr="")
            setup_profiler.run_massif()

            call_args = mock_run.call_args[0][0]
            assert "--tool=massif" in call_args

    def test_heaptrack_mode(self, setup_profiler):
        """Test heaptrack mode."""
        with patch("shutil.which", return_value="/usr/bin/heaptrack"), \
             patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            setup_profiler.run_heaptrack()

            call_args = mock_run.call_args[0][0]
            assert "heaptrack" in call_args

    def test_perf_mode(self, setup_profiler):
        """Test perf mode."""
        with patch("shutil.which", return_value="/usr/bin/perf"), \
             patch("subprocess.run") as mock_run, \
             patch.object(Path, "exists", return_value=True), \
             patch.object(Path, "read_text", return_value="1"):
            mock_run.return_value = MagicMock(returncode=0, stdout="", stderr="")
            setup_profiler.run_perf()

            # First call is perf record
            call_args = mock_run.call_args_list[0][0][0]
            assert "perf" in call_args
            assert "record" in call_args

    def test_sanitize_mode_builds_project(self, setup_profiler):
        """Test sanitize mode runs cmake build."""
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            setup_profiler.run_sanitize()

            # Should have cmake configure call
            cmake_calls = [c for c in mock_run.call_args_list
                          if "cmake" in str(c)]
            assert len(cmake_calls) >= 1


class TestTimeout:
    """Tests for timeout handling."""

    def test_timeout_returns_124(self, tmp_path):
        """Test timeout returns exit code 124."""
        import subprocess

        build_dir = tmp_path / "build"
        build_dir.mkdir()
        exe = build_dir / "QGroundControl"
        exe.write_text("#!/bin/bash\n")
        exe.chmod(0o755)

        profiler = Profiler(
            build_dir=build_dir,
            output_dir=tmp_path / "profile",
            timeout=1,
        )

        with patch("shutil.which", return_value="/usr/bin/valgrind"), \
             patch("subprocess.run") as mock_run:
            mock_run.side_effect = subprocess.TimeoutExpired(cmd="test", timeout=1)
            result = profiler.run_memcheck()
            assert result == 124
