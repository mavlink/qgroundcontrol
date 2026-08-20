"""Tests for common.deps module."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

import pytest
from common.deps import check_dependencies, pip_install, require_tool
from common.errors import ToolNotFoundError


class TestCheckDependencies:
    """Tests for check_dependencies()."""

    def test_all_found(self):
        """Returns empty list when all tools exist."""
        with patch.object(shutil, "which", return_value="/usr/bin/python3"):
            assert check_dependencies(["python3", "cmake"]) == []

    def test_some_missing(self):
        """Returns names of missing tools."""

        def fake_which(name):
            return "/usr/bin/cmake" if name == "cmake" else None

        with patch.object(shutil, "which", side_effect=fake_which):
            result = check_dependencies(["cmake", "ninja", "gcovr"])
            assert result == ["ninja", "gcovr"]

    def test_empty_list(self):
        """Empty input returns empty output."""
        assert check_dependencies([]) == []


class TestRequireTool:
    """Tests for require_tool()."""

    def test_found(self):
        """Returns Path when tool exists."""
        with patch.object(shutil, "which", return_value="/usr/bin/cmake"):
            result = require_tool("cmake")
            assert isinstance(result, Path)
            assert result == Path("/usr/bin/cmake")

    def test_not_found_raises(self):
        """Raises ToolNotFoundError when tool is missing."""
        with (
            patch.object(shutil, "which", return_value=None),
            pytest.raises(ToolNotFoundError, match="cmake"),
        ):
            require_tool("cmake")

    def test_hint_in_message(self):
        """Hint text appears in the error message."""
        with (
            patch.object(shutil, "which", return_value=None),
            pytest.raises(ToolNotFoundError, match="pip install"),
        ):
            require_tool("gcovr", hint="pip install gcovr")


def test_pip_install_prefers_project_venv_with_uv(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    python = (
        tmp_path / ".venv" / ("Scripts/python.exe" if sys.platform == "win32" else "bin/python")
    )
    python.parent.mkdir(parents=True)
    python.touch()
    calls: list[list[str]] = []
    monkeypatch.setattr(shutil, "which", lambda name: "/usr/bin/uv" if name == "uv" else None)
    monkeypatch.setattr("common.file_traversal.find_repo_root", lambda: tmp_path)
    monkeypatch.setattr(subprocess, "run", lambda command, **_kwargs: calls.append(command))

    pip_install(["pre-commit"])
    assert calls == [["uv", "pip", "install", "--python", str(python), "pre-commit"]]


def test_pip_install_falls_back_to_current_interpreter(monkeypatch: pytest.MonkeyPatch) -> None:
    calls: list[list[str]] = []
    monkeypatch.setattr(shutil, "which", lambda _name: None)
    monkeypatch.setattr(subprocess, "run", lambda command, **_kwargs: calls.append(command))
    pip_install(["gcovr"], quiet=False)
    assert calls == [[sys.executable, "-m", "pip", "install", "gcovr"]]
