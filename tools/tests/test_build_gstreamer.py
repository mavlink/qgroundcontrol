"""Tests for tools/setup/build_gstreamer.py."""

from __future__ import annotations

import tempfile
from pathlib import Path
from unittest.mock import patch

import pytest
from setup import build_gstreamer
from setup.build_gstreamer import BuildConfig, MesonBuilder, detect_host_arch, detect_jobs


def test_detect_jobs_with_override() -> None:
    assert detect_jobs(override=8) == 8


def test_detect_jobs_without_override() -> None:
    result = detect_jobs()
    assert result >= 1


def test_detect_jobs_fallback_when_cpu_count_none() -> None:
    with patch("os.cpu_count", return_value=None):
        assert detect_jobs() == 4


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


@pytest.mark.parametrize("work_dir_args", [[], ["--work-dir", ""]])
def test_main_without_work_dir_uses_system_temp(work_dir_args: list[str]) -> None:
    argv = ["build_gstreamer.py", "--platform", "linux", "--version", "1.24.0", *work_dir_args]
    with patch("sys.argv", argv), patch.object(build_gstreamer, "MesonBuilder") as builder:
        assert build_gstreamer.main() == 0
    builder.return_value.run.assert_called_once_with()
    assert builder.call_args.args[0].work_dir == Path(tempfile.gettempdir())


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
