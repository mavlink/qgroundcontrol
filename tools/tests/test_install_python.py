#!/usr/bin/env python3
"""Tests for tools/setup/install_python.py."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

TOOLS_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(TOOLS_DIR))

from setup.install_python import get_packages_for_groups


def test_test_group_contains_pytest() -> None:
    packages = get_packages_for_groups("test")
    assert packages == ["pytest"]


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
