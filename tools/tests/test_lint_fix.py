"""Tests for lint_fix.py linting and formatting tool."""

from __future__ import annotations

import os
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from lint_fix import (
    FixResult,
    LintFixer,
    parse_args,
)


class TestFixResult:
    """Tests for FixResult dataclass."""

    def test_default_values(self):
        """Test default FixResult values."""
        result = FixResult(formatter="test")
        assert result.formatter == "test"
        assert result.files_modified == []
        assert result.output == ""
        assert result.success is True
        assert result.skipped is False

    def test_failed_result(self):
        """Test failed FixResult."""
        result = FixResult(
            formatter="clang-format",
            success=False,
            output="Error 1\nError 2",
            files_modified=["file1.cpp"],
        )
        assert result.success is False
        assert len(result.files_modified) == 1


class TestParseArgs:
    """Tests for parse_args function."""

    def test_default_args(self):
        """Test default arguments."""
        args = parse_args([])
        assert args.check is False
        assert args.stage is False

    def test_check_flag(self):
        """Test --check flag."""
        args = parse_args(["--check"])
        assert args.check is True

    def test_stage_flag(self):
        """Test --stage flag."""
        args = parse_args(["--stage"])
        assert args.stage is True

    def test_changed_flag(self):
        """Test --changed flag."""
        args = parse_args(["--changed"])
        assert args.changed is True

    def test_all_files_default(self):
        """Test --all flag (default)."""
        args = parse_args([])
        assert args.all_files is True


class TestLintFixer:
    """Tests for LintFixer class."""

    def test_init(self, mock_repo):
        """Test LintFixer initialization."""
        fixer = LintFixer(mock_repo)
        assert fixer.repo_root == mock_repo

    def test_check_pre_commit_installed(self, mock_repo):
        """Test checking if pre-commit is installed."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value="/usr/bin/pre-commit"):
            assert fixer._check_pre_commit() is True

    def test_check_pre_commit_not_installed(self, mock_repo):
        """Test checking when pre-commit not installed."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value=None):
            assert fixer._check_pre_commit() is False

    def test_get_default_branch(self, mock_repo_with_git):
        """Test getting default branch."""
        fixer = LintFixer(mock_repo_with_git)
        branch = fixer._get_default_branch()
        # Should return some branch name
        assert isinstance(branch, str)


class TestLintFixerRuffFormat:
    """Tests for LintFixer ruff format."""

    def test_run_ruff_format_not_found(self, mock_repo):
        """Test ruff format when ruff not installed."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value=None):
            result = fixer._run_ruff_format()

        assert result.skipped is True

    def test_run_ruff_format_success(self, mock_repo):
        """Test ruff format success."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value="/usr/bin/ruff"):
            with patch.object(fixer, "_find_files", return_value=[]):
                result = fixer._run_ruff_format()

        # No files to format
        assert result.success is True


class TestLintFixerClangFormat:
    """Tests for LintFixer clang-format."""

    def test_run_clang_format_not_found(self, mock_repo):
        """Test clang-format when not installed."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value=None):
            result = fixer._run_clang_format()

        assert result.skipped is True

    def test_run_clang_format_no_files(self, mock_repo):
        """Test clang-format with no files."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value="/usr/bin/clang-format"):
            with patch.object(fixer, "_find_files", return_value=[]):
                result = fixer._run_clang_format()

        assert result.success is True
        assert result.files_modified == []


class TestLintFixerCmakeFormat:
    """Tests for LintFixer cmake-format."""

    def test_run_cmake_format_not_found(self, mock_repo):
        """Test cmake-format when not installed."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value=None):
            result = fixer._run_cmake_format()

        assert result.skipped is True


class TestLintFixerRun:
    """Tests for LintFixer.run_formatters method."""

    def test_run_all_formatters(self, mock_repo):
        """Test running all formatters."""
        fixer = LintFixer(mock_repo)

        # Mock all tools to be missing and pre-commit hooks to skip
        with patch("shutil.which", return_value=None):
            with patch.object(fixer, "_run_precommit_hooks") as mock_hooks:
                mock_hooks.return_value = FixResult(
                    formatter="pre-commit", skipped=True
                )
                results = fixer.run_formatters()

        # All formatters should be skipped when tools not found
        assert all(r.skipped for r in results)

    def test_run_formatters_returns_list(self, mock_repo):
        """Test run_formatters returns list of FixResult."""
        fixer = LintFixer(mock_repo)

        with patch("shutil.which", return_value=None):
            results = fixer.run_formatters()

        assert isinstance(results, list)
        assert all(isinstance(r, FixResult) for r in results)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
