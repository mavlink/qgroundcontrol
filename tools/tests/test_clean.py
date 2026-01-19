"""Tests for clean.py."""

from __future__ import annotations

import sys
from pathlib import Path
from unittest.mock import patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from clean import Cleaner, CleanStats, get_size_str


class TestCleanStats:
    """Tests for CleanStats dataclass."""

    def test_empty_stats(self):
        """Test empty stats."""
        stats = CleanStats()
        assert stats.total_items == 0
        assert len(stats.files) == 0
        assert len(stats.directories) == 0

    def test_stats_counting(self):
        """Test item counting."""
        stats = CleanStats()
        stats.files.append(Path("/tmp/file1"))
        stats.files.append(Path("/tmp/file2"))
        stats.directories.append(Path("/tmp/dir1"))
        assert stats.total_items == 3


class TestGetSizeStr:
    """Tests for get_size_str function."""

    def test_size_bytes(self, tmp_path):
        """Test size in bytes."""
        f = tmp_path / "small.txt"
        f.write_text("hello")
        size = get_size_str(f)
        assert "B" in size

    def test_size_kilobytes(self, tmp_path):
        """Test size in kilobytes."""
        f = tmp_path / "medium.txt"
        f.write_text("x" * 2000)
        size = get_size_str(f)
        assert "K" in size or "B" in size  # Depends on exact size

    def test_size_directory(self, tmp_path):
        """Test directory size calculation."""
        d = tmp_path / "subdir"
        d.mkdir()
        (d / "file1.txt").write_text("content1")
        (d / "file2.txt").write_text("content2")
        size = get_size_str(d)
        assert size != "?"

    def test_size_nonexistent(self, tmp_path):
        """Test nonexistent path returns ?."""
        f = tmp_path / "nonexistent"
        size = get_size_str(f)
        assert size == "?"


class TestCleaner:
    """Tests for Cleaner class."""

    def test_dry_run_no_removal(self, tmp_path):
        """Test dry run doesn't remove files."""
        (tmp_path / "build").mkdir()
        (tmp_path / "build" / "test.txt").write_text("test")

        cleaner = Cleaner(tmp_path, dry_run=True)
        cleaner.clean_build()

        # File should still exist
        assert (tmp_path / "build").exists()
        # But should be tracked in stats
        assert cleaner.stats.total_items > 0

    def test_actual_removal(self, tmp_path):
        """Test actual removal of build directory."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        (build_dir / "test.txt").write_text("test")

        cleaner = Cleaner(tmp_path, dry_run=False)
        cleaner.clean_build()

        # Build directory should be gone
        assert not build_dir.exists()

    def test_clean_user_files(self, tmp_path):
        """Test cleaning Qt Creator .user files."""
        user_file = tmp_path / "CMakeLists.txt.user"
        user_file.write_text("user config")

        cleaner = Cleaner(tmp_path, dry_run=False)
        cleaner.clean_build()

        assert not user_file.exists()

    def test_clean_cmake_presets(self, tmp_path):
        """Test cleaning CMakeUserPresets.json."""
        presets = tmp_path / "CMakeUserPresets.json"
        presets.write_text("{}")

        cleaner = Cleaner(tmp_path, dry_run=False)
        cleaner.clean_build()

        assert not presets.exists()

    def test_clean_cache(self, tmp_path):
        """Test cleaning cache directories."""
        cache_dir = tmp_path / ".cache"
        cache_dir.mkdir()
        (cache_dir / "test.txt").write_text("cache")

        cleaner = Cleaner(tmp_path, dry_run=False)
        cleaner.clean_cache()

        assert not cache_dir.exists()

    def test_clean_clangd_index(self, tmp_path):
        """Test cleaning .clangd directory."""
        clangd_dir = tmp_path / ".clangd"
        clangd_dir.mkdir()

        cleaner = Cleaner(tmp_path, dry_run=False)
        cleaner.clean_cache()

        assert not clangd_dir.exists()

    def test_clean_pycache(self, tmp_path):
        """Test cleaning __pycache__ directories."""
        pycache = tmp_path / "src" / "__pycache__"
        pycache.mkdir(parents=True)
        (pycache / "module.cpython-39.pyc").write_bytes(b"bytecode")

        cleaner = Cleaner(tmp_path, dry_run=False)
        cleaner.clean_generated()

        assert not pycache.exists()

    def test_nonexistent_paths_skipped(self, tmp_path):
        """Test that nonexistent paths are silently skipped."""
        cleaner = Cleaner(tmp_path, dry_run=False)
        # Should not raise
        cleaner.clean_build()
        cleaner.clean_cache()
        cleaner.clean_generated()

    def test_ccache_stats_cleared(self, tmp_path):
        """Test ccache stats are cleared when ccache is available."""
        with patch("shutil.which") as mock_which, \
             patch("subprocess.run") as mock_run:
            mock_which.return_value = "/usr/bin/ccache"

            cleaner = Cleaner(tmp_path, dry_run=False)
            cleaner.clean_cache()

            mock_run.assert_called_once()
            assert "ccache" in mock_run.call_args[0][0]
