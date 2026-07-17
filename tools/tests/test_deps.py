"""Contracts for external-tool discovery."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

import common.deps as deps
import pytest
from common.deps import check_dependencies, pip_install, require_tool
from common.errors import ToolNotFoundError


def test_dependency_checks_report_only_missing_tools(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(shutil, "which", lambda name: "/usr/bin/cmake" if name == "cmake" else None)
    assert check_dependencies([]) == []
    assert check_dependencies(["cmake", "ninja", "gcovr"]) == ["ninja", "gcovr"]


def test_require_tool_returns_path_or_actionable_error(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(shutil, "which", lambda _name: "/usr/bin/cmake")
    assert require_tool("cmake") == Path("/usr/bin/cmake")

    monkeypatch.setattr(shutil, "which", lambda _name: None)
    with pytest.raises(ToolNotFoundError, match="cmake"):
        require_tool("cmake")
    with pytest.raises(ToolNotFoundError, match="pip install"):
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
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda command, **_kwargs: calls.append(command),
    )

    pip_install(["pre-commit"])
    assert calls == [["uv", "pip", "install", "--python", str(python), "pre-commit"]]


def test_pip_install_falls_back_to_current_interpreter(monkeypatch: pytest.MonkeyPatch) -> None:
    calls: list[list[str]] = []
    monkeypatch.setattr(shutil, "which", lambda _name: None)
    monkeypatch.setattr(deps.subprocess, "run", lambda command, **_kwargs: calls.append(command))
    pip_install(["gcovr"], quiet=False)
    assert calls == [[sys.executable, "-m", "pip", "install", "gcovr"]]
