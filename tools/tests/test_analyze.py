"""Tests for analyze.py code quality tool."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from analyze import (
    AnalysisResult,
    FileCollector,
    AnalyzerBase,
    ClangFormatAnalyzer,
    validate_path,
    get_analyzer,
)


class TestAnalysisResult:
    """Tests for AnalysisResult dataclass."""

    def test_default_values(self):
        """Test default values for AnalysisResult."""
        result = AnalysisResult(tool="test", passed=True)
        assert result.tool == "test"
        assert result.passed is True
        assert result.issues == 0
        assert result.output == ""
        assert result.files_checked == 0
        assert result.files_with_issues == []

    def test_failed_result(self):
        """Test failed result with issues."""
        result = AnalysisResult(
            tool="clang-format",
            passed=False,
            issues=5,
            files_checked=10,
            files_with_issues=["file1.cpp", "file2.cpp"],
        )
        assert result.passed is False
        assert result.issues == 5
        assert len(result.files_with_issues) == 2


class TestFileCollector:
    """Tests for FileCollector class."""

    def test_init(self, tmp_path):
        """Test FileCollector initialization."""
        collector = FileCollector(tmp_path)
        assert collector.repo_root == tmp_path

    def test_find_files_empty_dir(self, tmp_path):
        """Test finding files in empty directory."""
        collector = FileCollector(tmp_path)
        files = collector._find_files(tmp_path, (".cpp", ".h"))
        assert files == []

    def test_find_files_with_cpp(self, tmp_path):
        """Test finding C++ files."""
        # Create test files
        (tmp_path / "test.cpp").write_text("int main() {}")
        (tmp_path / "test.h").write_text("#pragma once")
        (tmp_path / "test.py").write_text("# python file")

        collector = FileCollector(tmp_path)
        files = collector._find_files(tmp_path, (".cpp", ".h"))

        assert len(files) == 2
        assert any(f.suffix == ".cpp" for f in files)
        assert any(f.suffix == ".h" for f in files)

    def test_find_files_nonexistent_path(self, tmp_path):
        """Test finding files in nonexistent path."""
        collector = FileCollector(tmp_path)
        files = collector._find_files(tmp_path / "nonexistent", (".cpp",))
        assert files == []

    def test_get_cpp_files_all(self, tmp_path):
        """Test getting all C++ files."""
        src_dir = tmp_path / "src"
        src_dir.mkdir()
        (src_dir / "main.cpp").write_text("int main() {}")
        (src_dir / "utils.h").write_text("#pragma once")

        collector = FileCollector(tmp_path)
        files = collector.get_cpp_files(analyze_all=True)

        assert len(files) == 2

    def test_get_qml_files(self, tmp_path):
        """Test getting QML files."""
        src_dir = tmp_path / "src"
        src_dir.mkdir()
        (src_dir / "Main.qml").write_text("import QtQuick")
        (src_dir / "test.cpp").write_text("int main() {}")

        collector = FileCollector(tmp_path)
        files = collector.get_qml_files(analyze_all=True)

        assert len(files) == 1
        assert files[0].suffix == ".qml"

    def test_can_compare_master_with_mock(self, tmp_path):
        """Test can_compare_master with mocked git."""
        collector = FileCollector(tmp_path)

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            assert collector.can_compare_master() is True

    def test_can_compare_master_no_master(self, tmp_path):
        """Test can_compare_master when master doesn't exist."""
        collector = FileCollector(tmp_path)

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=1)
            assert collector.can_compare_master() is False


class TestAnalyzerBase:
    """Tests for AnalyzerBase class."""

    def test_check_tool_exists(self, tmp_path):
        """Test check_tool with existing tool."""
        analyzer = ClangFormatAnalyzer(tmp_path, tmp_path / "build")

        with patch("shutil.which") as mock_which:
            mock_which.return_value = "/usr/bin/clang-format"
            assert analyzer.check_tool("clang-format") is True

    def test_check_tool_missing(self, tmp_path):
        """Test check_tool with missing tool."""
        analyzer = ClangFormatAnalyzer(tmp_path, tmp_path / "build")

        with patch("shutil.which") as mock_which:
            mock_which.return_value = None
            assert analyzer.check_tool("clang-format") is False

    def test_relative_path(self, tmp_path):
        """Test relative_path conversion."""
        analyzer = ClangFormatAnalyzer(tmp_path, tmp_path / "build")
        file_path = tmp_path / "src" / "file.cpp"

        result = analyzer.relative_path(file_path)
        assert result == "src/file.cpp"


class TestValidatePath:
    """Tests for validate_path function."""

    def test_validate_existing_path(self, tmp_path):
        """Test validating existing path."""
        src_dir = tmp_path / "src"
        src_dir.mkdir()

        result = validate_path("src", tmp_path)
        assert result == Path("src")

    def test_validate_nonexistent_path_allowed(self, tmp_path):
        """Test that nonexistent paths are allowed (returns path)."""
        result = validate_path("nonexistent", tmp_path)
        assert result == Path("nonexistent")

    def test_validate_path_parent_traversal(self, tmp_path):
        """Test that parent traversal is rejected."""
        with pytest.raises(ValueError, match="must not contain"):
            validate_path("../outside", tmp_path)

    def test_validate_absolute_path(self, tmp_path):
        """Test that absolute paths are rejected."""
        with pytest.raises(ValueError, match="must be relative"):
            validate_path("/absolute/path", tmp_path)


class TestGetAnalyzer:
    """Tests for get_analyzer function."""

    def test_get_clang_format(self, tmp_path):
        """Test getting clang-format analyzer."""
        analyzer = get_analyzer("clang-format", tmp_path, tmp_path / "build")
        assert analyzer.name == "clang-format"

    def test_get_clang_tidy(self, tmp_path):
        """Test getting clang-tidy analyzer."""
        analyzer = get_analyzer("clang-tidy", tmp_path, tmp_path / "build")
        assert analyzer.name == "clang-tidy"

    def test_get_cppcheck(self, tmp_path):
        """Test getting cppcheck analyzer."""
        analyzer = get_analyzer("cppcheck", tmp_path, tmp_path / "build")
        assert analyzer.name == "cppcheck"

    def test_get_unknown_tool(self, tmp_path):
        """Test getting unknown tool raises error."""
        with pytest.raises(ValueError, match="Unknown tool"):
            get_analyzer("unknown-tool", tmp_path, tmp_path / "build")


class TestClangFormatAnalyzer:
    """Tests for ClangFormatAnalyzer."""

    def test_name(self, tmp_path):
        """Test analyzer name."""
        analyzer = ClangFormatAnalyzer(tmp_path, tmp_path / "build")
        assert analyzer.name == "clang-format"

    def test_run_no_files(self, tmp_path):
        """Test running with no files."""
        analyzer = ClangFormatAnalyzer(tmp_path, tmp_path / "build")

        with patch("shutil.which", return_value="/usr/bin/clang-format"):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(
                    returncode=0,
                    stdout="clang-format version 14.0.0 (Ubuntu 14.0.0-1ubuntu1)"
                )
                result = analyzer.run([], fix=False)

        assert result.passed is True

    def test_run_tool_not_found(self, tmp_path):
        """Test running when tool not found."""
        analyzer = ClangFormatAnalyzer(tmp_path, tmp_path / "build")

        with patch("shutil.which", return_value=None):
            result = analyzer.run([], fix=False)

        assert result.passed is False
        assert "Tool not found" in result.output


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
