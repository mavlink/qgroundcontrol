#!/usr/bin/env python3
"""Configuration contracts for the source GStreamer builder."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ._helpers import load_script_module

if TYPE_CHECKING:
    from pathlib import Path

    import pytest

mod = load_script_module("setup/build-gstreamer.py", "build_gstreamer")


def test_job_and_host_arch_detection(monkeypatch: pytest.MonkeyPatch) -> None:
    assert mod.detect_jobs(override=8) == 8
    assert mod.detect_jobs() >= 1
    monkeypatch.setattr("os.cpu_count", lambda: None)
    assert mod.detect_jobs() == 4

    monkeypatch.delenv("RUNNER_ARCH", raising=False)
    for machine, expected in (
        ("x86_64", "x86_64"),
        ("amd64", "x86_64"),
        ("aarch64", "arm64"),
        ("arm64", "arm64"),
        ("armv7l", "armv7"),
    ):
        monkeypatch.setattr("platform.machine", lambda machine=machine: machine)
        assert mod.detect_host_arch() == expected


def test_build_config_paths_and_archive_names(tmp_path: Path) -> None:
    config = mod.BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    assert config.prefix == tmp_path / "gst-linux-x86_64"
    assert config.source_dir == tmp_path / "gstreamer"
    assert config.build_dir == config.source_dir / "builddir"
    assert config.build_type == "release"
    assert config.archive_name == "gstreamer-1.0-linux-x86_64-1.24.0"

    custom = tmp_path / "custom"
    assert mod.BuildConfig("linux", "x86_64", "1.24.0", prefix=custom).prefix == custom
    ios = mod.BuildConfig("ios", "arm64", "1.24.0", simulator=True)
    assert ios.archive_name == "gstreamer-1.0-ios-arm64-1.24.0-simulator"


def test_meson_arguments_cover_plugins_and_platform_arch_flags(tmp_path: Path) -> None:
    linux = mod.BuildConfig("linux", "x86_64", "1.24.0", work_dir=tmp_path)
    linux_args = mod.MesonBuilder(linux).get_meson_args()
    for value in (
        f"--prefix={linux.prefix}",
        "--buildtype=release",
        "--wrap-mode=forcefallback",
        "-Dgst-plugins-base:gl=enabled",
        "-Dgst-plugins-good:qt6=enabled",
        "-Dgst-plugins-bad:videoparsers=enabled",
        "-Dgst-plugins-ugly:x264=enabled",
    ):
        assert value in linux_args
    assert not any("-Dcpp_args" in arg for arg in linux_args)

    macos = mod.BuildConfig("macos", "arm64", "1.24.0", work_dir=tmp_path)
    macos_args = mod.MesonBuilder(macos).get_meson_args()
    assert "-Dcpp_args=['-arch', 'arm64']" in macos_args
    assert "-Dc_args=['-arch', 'arm64']" in macos_args

    universal = mod.BuildConfig("macos", "universal", "1.24.0", work_dir=tmp_path)
    assert not any("_args" in arg for arg in mod.MesonBuilder(universal).get_meson_args())
