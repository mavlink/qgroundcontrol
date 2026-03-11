#!/usr/bin/env python3
"""Tests for tools/setup/install_python.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import patch

import pytest

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
    with patch("setup.install_python.find_repo_root", return_value=tmp_path), \
         patch("setup.install_python.subprocess.run") as mock_run:
        sync_groups_with_uv(venv, "scripts,test")

    args = mock_run.call_args.args[0]
    assert args[:5] == ["uv", "sync", "--project", str(tmp_path / "tools"), "--active"]
    assert "--frozen" in args
    assert args.count("--extra") == 2
