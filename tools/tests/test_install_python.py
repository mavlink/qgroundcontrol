#!/usr/bin/env python3
"""Tests for tools/setup/install_python.py."""

from __future__ import annotations

from pathlib import Path  # noqa: TC003
from subprocess import CalledProcessError
from unittest.mock import patch

import pytest
from common import proc
from setup import install_python
from setup.install_python import get_packages_for_groups, sync_groups_with_uv


def test_test_group_contains_pytest() -> None:
    packages = get_packages_for_groups("test")
    assert "pytest" in packages
    assert "jinja2" in packages
    assert "pyyaml" in packages


def test_scripts_group_contains_defusedxml() -> None:
    packages = get_packages_for_groups("scripts")
    assert "defusedxml>=0.7.1" in packages


def test_multiple_groups_are_merged() -> None:
    packages = get_packages_for_groups("precommit,test")
    assert "pre-commit" in packages
    assert "pytest" in packages
    assert len(packages) == len(set(packages))


def test_all_group_includes_test_and_ci_tools() -> None:
    packages = get_packages_for_groups("all")
    assert "pytest" in packages
    assert "pre-commit" in packages
    assert "meson" in packages
    assert "ninja" in packages


def test_unknown_group_raises() -> None:
    with pytest.raises(ValueError, match="Unknown group"):
        get_packages_for_groups("nope")


def test_sync_groups_with_uv_uses_locked_active_env(tmp_path: Path) -> None:
    venv = tmp_path / ".venv"
    (tmp_path / "tools").mkdir()
    lockfile = tmp_path / "tools" / "uv.lock"
    lockfile.write_text("", encoding="utf-8")
    with (
        patch("setup.install_python.find_repo_root", return_value=tmp_path),
        patch("setup.install_python.run_checked_with_retry") as mock_run,
    ):
        sync_groups_with_uv(venv, "scripts,test")

    args = mock_run.call_args.args[0]
    assert args[:5] == ["uv", "sync", "--project", str(tmp_path / "tools"), "--active"]
    assert "--frozen" in args
    assert args.count("--extra") == 2


def test_list_packages_uses_uv_for_synced_environment(tmp_path: Path) -> None:
    venv = tmp_path / ".venv"
    with (
        patch.object(install_python, "has_uv", return_value=True),
        patch.object(install_python.subprocess, "run") as mock_run,
    ):
        install_python.list_packages(venv)

    mock_run.assert_called_once_with(
        ["uv", "pip", "list", "--python", str(install_python.get_python_executable(venv))],
        check=False,
    )


def test_list_packages_uses_pip_without_uv(tmp_path: Path) -> None:
    venv = tmp_path / ".venv"
    python = install_python.get_python_executable(venv)
    with (
        patch.object(install_python, "has_uv", return_value=False),
        patch.object(install_python.subprocess, "run") as mock_run,
    ):
        install_python.list_packages(venv)

    mock_run.assert_called_once_with([str(python), "-m", "pip", "list"], check=False)


def test_create_venv_removes_partial_environment_before_retry(tmp_path: Path) -> None:
    venv = tmp_path / ".venv"
    command = ["uv", "venv", "--seed", str(venv)]
    attempts = 0

    def create_partial_then_succeed(*_args: object, **_kwargs: object) -> None:
        nonlocal attempts
        attempts += 1
        if attempts == 1:
            venv.mkdir()
            (venv / "partial").touch()
            raise CalledProcessError(1, command)
        assert not venv.exists()
        venv.mkdir()

    with (
        patch.object(install_python, "has_uv", return_value=True),
        patch.object(proc.subprocess, "run", side_effect=create_partial_then_succeed),
        patch.object(proc.time, "sleep"),
    ):
        install_python.create_venv(venv)

    assert attempts == 2
    assert venv.is_dir()
    assert not (venv / "partial").exists()


def test_create_venv_removes_partial_environment_after_final_failure(tmp_path: Path) -> None:
    venv = tmp_path / ".venv"
    command = ["uv", "venv", "--seed", str(venv)]

    def always_fail(*_args: object, **_kwargs: object) -> None:
        venv.mkdir(exist_ok=True)
        raise CalledProcessError(1, command)

    with (
        patch.object(install_python, "has_uv", return_value=True),
        patch.object(proc.subprocess, "run", side_effect=always_fail),
        patch.object(proc.time, "sleep"),
        pytest.raises(CalledProcessError),
    ):
        install_python.create_venv(venv)

    assert not venv.exists()
