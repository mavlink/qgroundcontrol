"""Tests for tools/common/file_traversal.py."""

from __future__ import annotations

from pathlib import Path

import pytest
from common.file_traversal import (
    DEFAULT_SKIP_DIRS,
    find_repo_root,
    should_skip_path,
)


class TestFindRepoRoot:
    def test_find_repo_root(self) -> None:
        root = find_repo_root(Path(__file__))
        assert (root / ".git").exists()

    def test_source_archive_without_git_uses_markers(self, tmp_path: Path) -> None:
        (tmp_path / "CMakeLists.txt").touch()
        (tmp_path / "tools").mkdir()
        (tmp_path / "tools" / "_bootstrap.py").touch()
        script = tmp_path / "tools" / "clean.py"
        script.touch()
        assert find_repo_root(script) == tmp_path

    def test_custom_dir_cmakelists_is_not_a_root(self, tmp_path: Path) -> None:
        (tmp_path / ".git").mkdir()
        custom = tmp_path / "custom"
        custom.mkdir()
        (custom / "CMakeLists.txt").touch()
        assert find_repo_root(custom / "CMakeLists.txt") == tmp_path

    def test_outside_any_checkout_raises(self, tmp_path: Path) -> None:
        if any((parent / ".git").exists() for parent in tmp_path.parents):
            pytest.skip("temporary directory is inside a git checkout")
        with pytest.raises(RuntimeError, match="repository root"):
            find_repo_root(tmp_path / "file.py")


class TestShouldSkipPath:
    def test_skip_build(self) -> None:
        assert should_skip_path(Path("/project/build/file.cpp"))
        assert should_skip_path(Path("/project/src/build/nested.h"))

    def test_skip_libs(self) -> None:
        assert should_skip_path(Path("/project/libs/external/file.h"))

    def test_skip_cache(self) -> None:
        assert should_skip_path(Path("/project/.cache/file.cpp"))
        assert should_skip_path(Path("/project/.cache/ccache/file.cpp"))
        assert should_skip_path(Path("/project/.ccache/file.cpp"))

    def test_no_skip_src(self) -> None:
        assert not should_skip_path(Path("/project/src/file.cpp"))
        assert not should_skip_path(Path("/project/src/Vehicle/Vehicle.h"))


class TestDefaultSkipDirs:
    def test_contents(self) -> None:
        for entry in ("build", "libs", "node_modules", ".git", ".ccache"):
            assert entry in DEFAULT_SKIP_DIRS
