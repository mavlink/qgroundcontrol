"""Tests for pre_commit.py pre-commit hook runner."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pre_commit import (
    PreCommitResult,
    check_pre_commit,
    run_pre_commit,
    parse_output,
    strip_ansi,
    parse_args,
)


class TestPreCommitResult:
    """Tests for PreCommitResult dataclass."""

    def test_default_values(self):
        """Test default PreCommitResult values."""
        result = PreCommitResult()
        assert result.passed == 0
        assert result.failed == 0
        assert result.skipped == 0
        assert result.output == ""
        assert result.exit_code == 0
        assert result.modified_files == []

    def test_with_values(self):
        """Test PreCommitResult with values."""
        result = PreCommitResult(
            passed=5,
            failed=2,
            skipped=1,
            exit_code=1,
            modified_files=["file1.py", "file2.cpp"],
        )
        assert result.passed == 5
        assert result.failed == 2
        assert len(result.modified_files) == 2


class TestParseArgs:
    """Tests for parse_args function."""

    def test_default_args(self):
        """Test default arguments."""
        with patch("sys.argv", ["pre_commit.py"]):
            args = parse_args()
        # --all defaults to True (run on all files)
        assert args.all is True
        assert args.install is False
        assert args.changed is False

    def test_changed_flag(self):
        """Test --changed flag."""
        with patch("sys.argv", ["pre_commit.py", "--changed"]):
            args = parse_args()
        assert args.changed is True

    def test_install_flag(self):
        """Test --install flag."""
        with patch("sys.argv", ["pre_commit.py", "--install"]):
            args = parse_args()
        assert args.install is True


class TestCheckPreCommit:
    """Tests for check_pre_commit function."""

    def test_check_pre_commit_installed(self):
        """Test checking if pre-commit is installed."""
        with patch("shutil.which", return_value="/usr/bin/pre-commit"):
            assert check_pre_commit() is True

    def test_check_pre_commit_not_installed(self):
        """Test checking when pre-commit not installed."""
        with patch("shutil.which", return_value=None):
            assert check_pre_commit() is False


class TestParseOutput:
    """Tests for parse_output function."""

    def test_parse_output_all_passed(self):
        """Test parsing output when all hooks pass."""
        output = """
check yaml...............................................................Passed
check json...............................................................Passed
trim trailing whitespace.................................................Passed
"""
        passed, failed, skipped = parse_output(output)
        assert passed == 3
        assert failed == 0
        assert skipped == 0

    def test_parse_output_with_failures(self):
        """Test parsing output with some failures."""
        output = """
check yaml...............................................................Passed
clang-format.............................................................Failed
trailing-whitespace......................................................Passed
"""
        passed, failed, skipped = parse_output(output)
        assert passed == 2
        assert failed == 1
        assert skipped == 0

    def test_parse_output_with_skipped(self):
        """Test parsing output with skipped hooks."""
        output = """
check yaml...............................................................Passed
clang-format.............................................................Skipped
trailing-whitespace......................................................Passed
"""
        passed, failed, skipped = parse_output(output)
        assert passed == 2
        assert failed == 0
        assert skipped == 1


class TestStripAnsi:
    """Tests for strip_ansi function."""

    def test_strip_ansi_codes(self):
        """Test stripping ANSI escape codes."""
        text = "\033[0;31m[ERROR]\033[0m Something went wrong"
        result = strip_ansi(text)
        assert "\033" not in result
        assert "[ERROR]" in result
        assert "Something went wrong" in result

    def test_strip_ansi_no_codes(self):
        """Test stripping when no ANSI codes present."""
        text = "Plain text without colors"
        result = strip_ansi(text)
        assert result == text


class TestRunPreCommit:
    """Tests for run_pre_commit function."""

    def test_run_pre_commit_all_passed(self):
        """Test successful pre-commit run."""
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(
                returncode=0,
                stdout="check yaml...Passed\ntrailing-whitespace...Passed\n",
                stderr="",
            )
            result = run_pre_commit()

        assert result.exit_code == 0
        assert result.passed == 2

    def test_run_pre_commit_with_failures(self):
        """Test pre-commit run with failures."""
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(
                returncode=1,
                stdout="check yaml...Passed\nclang-format...Failed\n",
                stderr="",
            )
            result = run_pre_commit()

        assert result.exit_code == 1
        assert result.passed == 1
        assert result.failed == 1

    def test_run_pre_commit_changed_only(self):
        """Test running on changed files only."""
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(
                returncode=0,
                stdout="check yaml...Passed\n",
                stderr="",
            )
            with patch("pre_commit.has_master_branch", return_value=True):
                result = run_pre_commit(changed_only=True)

        # Should have called subprocess.run with pre-commit command
        # Look for the pre-commit call (not git diff call)
        all_calls = mock_run.call_args_list
        pre_commit_calls = [c for c in all_calls if c[0][0][0] == "pre-commit"]
        assert len(pre_commit_calls) >= 1
        assert result.exit_code == 0


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
