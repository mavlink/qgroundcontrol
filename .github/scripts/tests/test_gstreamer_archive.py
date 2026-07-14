"""Archive naming and path contracts for GStreamer packages."""

from __future__ import annotations

from pathlib import Path

import pytest
from gstreamer_archive import GStreamerArchiver


def _archiver(platform: str, arch: str = "x86_64", **kwargs) -> GStreamerArchiver:
    return GStreamerArchiver(platform=platform, arch=arch, version="1.24.0", **kwargs)


def test_supported_platforms_and_validation() -> None:
    for platform in ("linux", "macos", "windows", "android", "ios"):
        assert _archiver(platform).platform == platform
    with pytest.raises(ValueError, match="Unknown platform"):
        _archiver("freebsd")


def test_prefix_and_output_path_precedence(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    assert _archiver("linux").prefix == tmp_path / "gstreamer"
    assert "gstreamer" in str(_archiver("windows").prefix).lower()

    monkeypatch.setenv("GSTREAMER_PREFIX", "/env/gstreamer")
    assert _archiver("linux").prefix == Path("/env/gstreamer")
    custom_output = Path("/custom/output")
    custom_prefix = Path("/opt/gstreamer")
    configured = _archiver("linux", prefix=custom_prefix, output_dir=custom_output)
    assert (configured.prefix, configured.output_dir) == (custom_prefix, custom_output)


def test_archive_names_cover_platform_and_simulator_variants() -> None:
    cases = [
        ("linux", "x86_64", False, "gstreamer-1.0-linux-x86_64-1.24.0"),
        ("windows", "x86_64", False, "gstreamer-1.0-msvc-x64-1.24.0"),
        ("macos", "arm64", False, "gstreamer-1.0-macos-arm64-1.24.0"),
        ("ios", "arm64", False, "gstreamer-1.0-ios-arm64-1.24.0"),
        ("ios", "arm64", True, "gstreamer-1.0-ios-arm64-simulator-1.24.0"),
    ]
    for platform, arch, simulator, expected in cases:
        assert _archiver(platform, arch, simulator=simulator).get_archive_name() == expected


def test_archive_extension_and_windows_arch_normalization() -> None:
    assert _archiver("windows")._normalize_arch() == "x64"
    assert _archiver("linux")._normalize_arch() == "x86_64"
    assert _archiver("windows")._get_extension() == "zip"
    assert _archiver("linux")._get_extension() == "tar.xz"
