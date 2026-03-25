#!/usr/bin/env python3
"""Tests for tools/analyze.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock, patch

from analyze import FileCollector, get_analyzer, validate_path


class TestFileCollector:
    def test_get_compare_ref_symbolic(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        mock_result = MagicMock(returncode=0, stdout="origin/main\n")
        with patch("analyze.subprocess.run", return_value=mock_result):
            ref = collector.get_compare_ref()
        assert ref == "main"

    def test_get_compare_ref_fallback_master(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        symbolic_fail = MagicMock(returncode=1, stdout="")
        master_ok = MagicMock(returncode=0)

        def side_effect(cmd, **kw):
            if "symbolic-ref" in cmd:
                return symbolic_fail
            if cmd[-1] == "master":
                return master_ok
            return MagicMock(returncode=1)

        with patch("analyze.subprocess.run", side_effect=side_effect):
            ref = collector.get_compare_ref()
        assert ref == "master"

    def test_get_compare_ref_none(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        fail = MagicMock(returncode=1, stdout="")
        with patch("analyze.subprocess.run", return_value=fail):
            ref = collector.get_compare_ref()
        assert ref is None

    def test_find_files(self, tmp_path: Path) -> None:
        src = tmp_path / "src"
        src.mkdir()
        (src / "foo.cpp").touch()
        (src / "bar.h").touch()
        (src / "readme.txt").touch()

        collector = FileCollector(tmp_path)
        files = collector._find_files(src, (".cpp", ".h"))
        names = [f.name for f in files]
        assert "foo.cpp" in names
        assert "bar.h" in names
        assert "readme.txt" not in names


class TestValidatePath:
    def test_valid_relative_path(self, tmp_path: Path) -> None:
        (tmp_path / "src").mkdir()
        result = validate_path("src", tmp_path)
        assert result == Path("src")

    def test_rejects_parent_traversal(self, tmp_path: Path) -> None:
        import pytest
        with pytest.raises(ValueError, match="must not contain"):
            validate_path("../etc/passwd", tmp_path)

    def test_rejects_absolute_path(self, tmp_path: Path) -> None:
        import pytest
        with pytest.raises(ValueError, match="must be relative"):
            validate_path("/etc/passwd", tmp_path)


class TestGetAnalyzer:
    def test_returns_clang_tidy(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("clang-tidy", tmp_path, tmp_path / "build", jobs=4)
        assert analyzer.name == "clang-tidy"
        assert analyzer.jobs == 4  # type: ignore[attr-defined]

    def test_returns_clazy_with_jobs(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("clazy", tmp_path, tmp_path / "build", jobs=2)
        assert analyzer.name == "clazy"
        assert analyzer.jobs == 2  # type: ignore[attr-defined]

    def test_returns_cppcheck(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("cppcheck", tmp_path, tmp_path / "build")
        assert analyzer.name == "cppcheck"

    def test_unknown_tool_raises(self, tmp_path: Path) -> None:
        import pytest
        with pytest.raises(ValueError, match="Unknown tool"):
            get_analyzer("nonexistent", tmp_path, tmp_path / "build")
