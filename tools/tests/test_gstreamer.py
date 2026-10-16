"""Tests for the shared GStreamer SDK package layout."""

from __future__ import annotations

import json
import shutil
import subprocess
from typing import TYPE_CHECKING

import pytest
from common.gstreamer import (
    PLATFORM_PACKAGES,
    mirror_url,
    package_filename,
    package_url,
    pinned_checksum,
    release_index_url,
)

from ._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

PACKAGES = [package for packages in PLATFORM_PACKAGES.values() for package in packages]


def test_package_urls() -> None:
    assert package_url("macos_devel", "1.28.4") == (
        "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.4/"
        "gstreamer-1.0-devel-1.28.4-universal.pkg"
    )
    assert package_filename("windows_msvc_arm64", "1.28.4") == (
        "gstreamer-1.0-msvc-arm64-1.28.4.exe"
    )
    assert mirror_url("windows_msvc_x64", "1.28.4") == (
        "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows/"
        "gstreamer-1.0-msvc-x86_64-1.28.4.exe"
    )
    assert release_index_url("ios") == "https://gstreamer.freedesktop.org/data/pkg/ios/"


@pytest.mark.parametrize(
    "call", [lambda: package_url("linux", "1.28.4"), lambda: release_index_url("linux")]
)
def test_unknown_names_are_rejected(call) -> None:
    with pytest.raises(ValueError, match="Unknown GStreamer"):
        call()


def test_pinned_checksum(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    config.write_text(json.dumps({"gstreamer": {"checksums": {"1.28.4": {"ios": "ab" * 32}}}}))

    assert pinned_checksum("1.28.4", "ios", config_file=config) == "ab" * 32
    assert pinned_checksum("1.28.4", "android", config_file=config) is None
    assert pinned_checksum("1.28.5", "ios", config_file=config) is None


@pytest.mark.skipif(shutil.which("cmake") is None, reason="cmake is not installed")
def test_layout_matches_cmake(tmp_path: Path) -> None:
    """The configure-time download (cmake/GStreamer/Download.cmake) must fetch the same files."""
    script = tmp_path / "urls.cmake"
    script.write_text(
        f'include("{REPO_ROOT}/cmake/GStreamer/Download.cmake")\n'
        f"foreach(_package IN ITEMS {' '.join(PACKAGES)})\n"
        "    gstreamer_get_package_url(${_package} 1.28.4 _url)\n"
        "    gstreamer_get_s3_mirror_url(${_package} 1.28.4 _mirror)\n"
        '    message(STATUS "${_package} ${_url} ${_mirror}")\n'
        "endforeach()\n"
    )
    result = subprocess.run(
        ["cmake", "-P", str(script)], check=True, capture_output=True, text=True
    )

    cmake_urls = [line.split()[1:] for line in result.stdout.splitlines()]
    assert cmake_urls == [
        [package, package_url(package, "1.28.4"), mirror_url(package, "1.28.4")]
        for package in PACKAGES
    ]
