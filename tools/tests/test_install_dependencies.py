#!/usr/bin/env python3
"""Tests for tools/setup/install_dependencies.py."""

from __future__ import annotations

import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

TOOLS_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(TOOLS_DIR))

from setup.install_dependencies import (
    DEBIAN_PACKAGES,
    MACOS_PACKAGES,
    PIPX_PACKAGES,
    detect_platform,
    get_debian_packages,
    get_macos_packages,
    parse_args,
)


# ---------------------------------------------------------------------------
# Package list integrity
# ---------------------------------------------------------------------------

def test_debian_packages_not_empty() -> None:
    assert DEBIAN_PACKAGES
    for category, pkgs in DEBIAN_PACKAGES.items():
        assert pkgs, f"Category '{category}' is empty"


def test_debian_packages_no_duplicates_within_category() -> None:
    for category, pkgs in DEBIAN_PACKAGES.items():
        assert len(pkgs) == len(set(pkgs)), f"Duplicates in category '{category}'"


def test_macos_packages_not_empty() -> None:
    assert MACOS_PACKAGES


def test_pipx_packages_not_empty() -> None:
    assert PIPX_PACKAGES


def test_get_debian_packages_all_returns_no_optional() -> None:
    pkgs = get_debian_packages()
    assert "gstreamer1.0-qt6" not in pkgs, "Optional gstreamer pkg should be excluded from default list"


def test_get_debian_packages_category_core() -> None:
    pkgs = get_debian_packages("core")
    assert "cmake" in pkgs
    assert "git" in pkgs
    assert "ninja-build" in pkgs


def test_get_debian_packages_category_qt() -> None:
    pkgs = get_debian_packages("qt")
    assert any("libxcb" in p for p in pkgs)


def test_get_debian_packages_unknown_category_returns_empty() -> None:
    assert get_debian_packages("nonexistent_category") == []


def test_get_debian_packages_no_duplicates() -> None:
    pkgs = get_debian_packages()
    assert len(pkgs) == len(set(pkgs))


def test_get_macos_packages() -> None:
    pkgs = get_macos_packages()
    assert "cmake" in pkgs
    assert "ninja" in pkgs
    assert "ccache" in pkgs


# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------

def test_detect_platform_linux_debian() -> None:
    with patch("sys.platform", "linux"), \
         patch("pathlib.Path.exists", return_value=True):
        assert detect_platform() == "debian"


def test_detect_platform_macos() -> None:
    with patch("sys.platform", "darwin"):
        assert detect_platform() == "macos"


def test_detect_platform_windows() -> None:
    with patch("sys.platform", "win32"):
        assert detect_platform() == "windows"


def test_detect_platform_unknown_linux() -> None:
    with patch("sys.platform", "linux"), \
         patch("pathlib.Path.exists", return_value=False):
        assert detect_platform() == "linux"


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def test_parse_args_defaults() -> None:
    args = parse_args([])
    assert args.platform is None
    assert args.dry_run is False
    assert args.list_packages is False
    assert args.category is None


def test_parse_args_platform_debian() -> None:
    args = parse_args(["--platform", "debian"])
    assert args.platform == "debian"


def test_parse_args_platform_windows() -> None:
    args = parse_args(["--platform", "windows"])
    assert args.platform == "windows"


def test_parse_args_dry_run() -> None:
    args = parse_args(["--dry-run"])
    assert args.dry_run is True


def test_parse_args_list() -> None:
    args = parse_args(["--list"])
    assert args.list_packages is True


def test_parse_args_category() -> None:
    args = parse_args(["--category", "qt"])
    assert args.category == "qt"


def test_parse_args_gstreamer_version() -> None:
    args = parse_args(["--platform", "windows", "--gstreamer-version", "1.24.0"])
    assert args.gstreamer_version == "1.24.0"


def test_parse_args_skip_gstreamer() -> None:
    args = parse_args(["--platform", "windows", "--skip-gstreamer"])
    assert args.skip_gstreamer is True


def test_parse_args_vulkan() -> None:
    args = parse_args(["--platform", "windows", "--vulkan"])
    assert args.vulkan is True


# ---------------------------------------------------------------------------
# download_file (network call mocked)
# ---------------------------------------------------------------------------

def test_download_file_dry_run(tmp_path: Path) -> None:
    from setup.install_dependencies import download_file

    dest = tmp_path / "test.bin"
    result = download_file("https://example.com/test.bin", dest, dry_run=True)
    assert result is True
    assert not dest.exists()


def test_download_file_network_error(tmp_path: Path) -> None:
    import urllib.error
    from setup.install_dependencies import download_file

    dest = tmp_path / "test.bin"
    with patch("urllib.request.urlretrieve", side_effect=urllib.error.URLError("unreachable")):
        result = download_file("https://example.com/test.bin", dest, dry_run=False)
    assert result is False
