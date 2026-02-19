#!/usr/bin/env python3
"""Tests for tools/setup/gstreamer/build-gstreamer.py."""

from __future__ import annotations

import importlib.util
from pathlib import Path
from unittest.mock import patch

import pytest

# Import module with a hyphen in the filename
import sys as _sys

_SCRIPT = Path(__file__).parent.parent / "setup" / "gstreamer" / "build-gstreamer.py"
_spec = importlib.util.spec_from_file_location("build_gstreamer", _SCRIPT)
_mod = importlib.util.module_from_spec(_spec)
_sys.modules["build_gstreamer"] = _mod
_spec.loader.exec_module(_mod)

BuildConfig = _mod.BuildConfig
MesonBuilder = _mod.MesonBuilder
detect_host_arch = _mod.detect_host_arch
detect_host_platform = _mod.detect_host_platform
detect_jobs = _mod.detect_jobs


# ── detect_jobs ──────────────────────────────────────────────────────────


def test_detect_jobs_with_override() -> None:
    assert detect_jobs(override=8) == 8


def test_detect_jobs_without_override() -> None:
    result = detect_jobs()
    assert result >= 1


def test_detect_jobs_fallback_when_cpu_count_none() -> None:
    with patch("os.cpu_count", return_value=None):
        assert detect_jobs() == 4


# ── detect_host_platform ─────────────────────────────────────────────────


def test_detect_host_platform_linux() -> None:
    with patch("platform.system", return_value="Linux"):
        assert detect_host_platform() == "linux"


def test_detect_host_platform_macos() -> None:
    with patch("platform.system", return_value="Darwin"):
        assert detect_host_platform() == "macos"


def test_detect_host_platform_windows() -> None:
    with patch("platform.system", return_value="Windows"):
        assert detect_host_platform() == "windows"


# ── detect_host_arch ──────────────────────────────────────────────────────


@pytest.mark.parametrize(
    "machine, expected",
    [
        ("x86_64", "x86_64"),
        ("amd64", "x86_64"),
        ("aarch64", "arm64"),
        ("arm64", "arm64"),
        ("armv7l", "armv7"),
    ],
)
def test_detect_host_arch(machine: str, expected: str) -> None:
    with patch("platform.machine", return_value=machine):
        assert detect_host_arch() == expected


# ── BuildConfig ───────────────────────────────────────────────────────────


def test_build_config_defaults(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    assert cfg.prefix == tmp_path / "gst-linux-x86_64"
    assert cfg.source_dir == tmp_path / "gstreamer"
    assert cfg.build_dir == tmp_path / "gstreamer" / "builddir"
    assert cfg.build_type == "release"


def test_build_config_custom_prefix(tmp_path: Path) -> None:
    custom = tmp_path / "custom"
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", prefix=custom)
    assert cfg.prefix == custom


def test_build_config_archive_name() -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0")
    assert cfg.archive_name == "gstreamer-1.0-linux-x86_64-1.24.0"


def test_build_config_archive_name_simulator() -> None:
    cfg = BuildConfig(platform="ios", arch="arm64", version="1.24.0", simulator=True)
    assert cfg.archive_name == "gstreamer-1.0-ios-arm64-1.24.0-simulator"


# ── MesonBuilder.get_meson_args ──────────────────────────────────────────


def test_get_meson_args_contains_prefix(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert f"--prefix={cfg.prefix}" in args
    assert "--buildtype=release" in args
    assert "--wrap-mode=forcefallback" in args


def test_get_meson_args_macos_arch_flags(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="macos", arch="arm64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert "-Dcpp_args=['-arch', 'arm64']" in args
    assert "-Dc_args=['-arch', 'arm64']" in args


def test_get_meson_args_macos_universal_no_arch_flags(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="macos", arch="universal", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert not any("-Dcpp_args" in a for a in args)
    assert not any("-Dc_args" in a for a in args)


def test_get_meson_args_linux_no_arch_flags(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert not any("-Dcpp_args" in a for a in args)


def test_get_meson_args_includes_plugins(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert "-Dgst-plugins-base:gl=enabled" in args
    assert "-Dgst-plugins-good:qt6=enabled" in args
    assert "-Dgst-plugins-bad:videoparsers=enabled" in args
    assert "-Dgst-plugins-ugly:x264=enabled" in args
