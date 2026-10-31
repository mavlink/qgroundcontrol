"""GStreamer SDK package layout shared by the installers, the S3 mirror and dependency checks.

Mirrors ``qgc_gst_get_package_url`` / ``qgc_gst_get_s3_mirror_url`` in
cmake/GStreamer/Packages.cmake. A package key is the ``gstreamer.checksums.<version>.<key>``
entry in .github/build-config.json that pins it.
"""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

from .build_config import load_build_config

if TYPE_CHECKING:
    from pathlib import Path

UPSTREAM_BASE_URL = "https://gstreamer.freedesktop.org/data/pkg"
S3_MIRROR_BASE_URL = "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer"

# Package key -> (platform directory, path under <platform>/<version>/ with {version} placeholders)
_PACKAGES: dict[str, tuple[str, str]] = {
    "android": ("android", "gstreamer-1.0-android-universal-{version}.tar.xz"),
    "ios": ("ios", "gstreamer-1.0-devel-{version}-ios-universal.pkg"),
    "macos": ("macos", "gstreamer-1.0-{version}-universal.pkg"),
    "macos_devel": ("macos", "gstreamer-1.0-devel-{version}-universal.pkg"),
    "windows_msvc_x64": ("windows", "msvc/gstreamer-1.0-msvc-x86_64-{version}.exe"),
    "windows_msvc_arm64": ("windows", "msvc/gstreamer-1.0-msvc-arm64-{version}.exe"),
}

# From 1.28.8 on, the iOS SDK is an xcframework tarball instead of a devel .pkg.
_IOS_XCFRAMEWORK_SINCE = (1, 28, 8)
_IOS_XCFRAMEWORK_PATH = "gstreamer-{version}-xcframework.tar.xz"

# Platforms with prebuilt upstream SDKs, and their packages.
PLATFORM_PACKAGES: dict[str, tuple[str, ...]] = {
    "android": ("android",),
    "ios": ("ios",),
    "macos": ("macos", "macos_devel"),
    "windows": ("windows_msvc_x64", "windows_msvc_arm64"),
}


def _package(package: str) -> tuple[str, str]:
    try:
        return _PACKAGES[package]
    except KeyError:
        raise ValueError(f"Unknown GStreamer package: {package!r}") from None


def package_platform(package: str) -> str:
    """Return the platform directory *package* is published under."""
    return _package(package)[0]


def package_filename(package: str, version: str) -> str:
    """Return the file name of *package* at *version*."""
    return package_url(package, version).rsplit("/", 1)[-1]


def _version_key(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in re.findall(r"\d+", version)[:3])


def package_url(package: str, version: str) -> str:
    """Return the gstreamer.freedesktop.org URL of *package* at *version*."""
    platform, path = _package(package)
    if package == "ios" and _version_key(version) >= _IOS_XCFRAMEWORK_SINCE:
        path = _IOS_XCFRAMEWORK_PATH
    return f"{UPSTREAM_BASE_URL}/{platform}/{version}/{path.format(version=version)}"


def mirror_url(package: str, version: str) -> str:
    """Return the QGC S3 mirror URL of *package* at *version*."""
    return f"{S3_MIRROR_BASE_URL}/{package_platform(package)}/{package_filename(package, version)}"


def release_index_url(platform: str) -> str:
    """Return the upstream directory listing the released versions for *platform*."""
    if platform not in PLATFORM_PACKAGES:
        raise ValueError(f"Unknown GStreamer platform: {platform!r}")
    return f"{UPSTREAM_BASE_URL}/{platform}/"


def pinned_checksum(version: str, package: str, *, config_file: Path | None = None) -> str | None:
    """Return the SHA-256 pinned for *package* at *version* in build-config.json, if any."""
    try:
        checksums = load_build_config(config_file).get("gstreamer", {}).get("checksums", {})
    except (AttributeError, FileNotFoundError, OSError):
        return None
    value = checksums.get(version, {}).get(package)
    return str(value) if value else None
