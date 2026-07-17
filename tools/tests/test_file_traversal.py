"""Contracts for repository traversal filters."""

from pathlib import Path

import pytest
from common.file_traversal import DEFAULT_SKIP_DIRS, find_repo_root, should_skip_path


def test_find_repo_root() -> None:
    assert (find_repo_root(Path(__file__)) / ".git").exists()


def test_find_repo_root_reports_missing_marker(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(Path, "exists", lambda _path: False)
    with pytest.raises(RuntimeError, match="Could not find repository root"):
        find_repo_root(tmp_path)


def test_default_skip_directories_filter_nested_paths() -> None:
    for directory in ("build", "libs", ".cache", ".ccache", "node_modules", ".git"):
        assert directory in DEFAULT_SKIP_DIRS
        assert should_skip_path(Path("/project") / directory / "nested" / "file.cpp")
    for path in (Path("/project/src/file.cpp"), Path("/project/src/Vehicle/Vehicle.h")):
        assert not should_skip_path(path)
