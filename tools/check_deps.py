#!/usr/bin/env python3
"""Check dependency and tool versions used by QGroundControl development."""

from __future__ import annotations

import argparse
import json
import re
import sys
from http.client import HTTPException
from pathlib import Path
from typing import TYPE_CHECKING

import _bootstrap as _bootstrap
from common.build_config import get_build_config_value
from common.file_traversal import find_repo_root
from common.gstreamer import PLATFORM_PACKAGES, release_index_url
from common.logging import log_info, log_ok, log_warn
from common.net import read_url_text
from common.proc import run_captured
from common.tool_version import probe_version
from packaging.version import Version

if TYPE_CHECKING:
    from collections.abc import Callable

QT_RELEASES_URL = "https://download.qt.io/official_releases/qt/"
GSTREAMER_PACKAGE_INDEX_URLS = {
    platform: release_index_url(platform) for platform in PLATFORM_PACKAGES
}
GSTREAMER_VERSION_PATTERN = re.compile(r'href=["\'](\d+)\.(\d+)\.(\d+)/["\']')
# Three components so `qmake --version` skips its "QMake version 3.1" line
FULL_VERSION_PATTERN = re.compile(r"(\d+)\.(\d+)\.(\d+)")
FETCH_TIMEOUT_SECONDS = 15
PYTHON_MANIFEST = Path("tools/pyproject.toml")
BUILD_TOOLS = ["cmake", "ninja", "ccache", "clang-format", "clang-tidy"]


def fetch_text(url: str) -> str | None:
    """Return the body of *url*, or None on any network, HTTP, or decoding error."""
    try:
        return read_url_text(url, timeout=FETCH_TIMEOUT_SECONDS)
    except (HTTPException, OSError, ValueError):
        return None


def format_version(version: tuple[int, ...]) -> str:
    return ".".join(str(part) for part in version)


def fetch_latest_qt_minor() -> str | None:
    """Return the latest Qt 6 minor version available on download.qt.io."""
    body = fetch_text(QT_RELEASES_URL)
    if body is None:
        return None

    versions = re.findall(r">6\.(\d+)/<", body)
    if not versions:
        versions = re.findall(r"6\.(\d+)", body)
    if not versions:
        return None
    latest_minor = max(int(version) for version in versions)
    return f"6.{latest_minor}"


def check_qt_version() -> None:
    """Check configured and installed Qt versions."""
    log_info("Checking Qt version...")
    current_version = get_build_config_value(
        "qt.version", "unknown", start=Path(__file__).resolve()
    )
    print(f"  Current: Qt {current_version}")

    latest_minor = fetch_latest_qt_minor()
    if latest_minor:
        print(f"  Latest minor: Qt {latest_minor}.x")
        if current_version and current_version != "unknown":
            current_minor = ".".join(current_version.split(".")[:2])
            if current_minor != latest_minor:
                log_warn(f"Newer Qt minor version available: {latest_minor}")
            else:
                log_ok("Using latest Qt minor version")

    for tool, args in (("qmake", ("--version",)), ("qtpaths", ("--qt-version",))):
        installed = probe_version(tool, args=args, pattern=FULL_VERSION_PATTERN)
        if installed:
            print(f"  Installed: Qt {format_version(installed)}")
            return


def parse_gstreamer_package_versions(index_html: str) -> set[tuple[int, int, int]]:
    """Extract semantic versions from an official GStreamer package index."""
    return {
        (int(major), int(minor), int(patch))
        for major, minor, patch in GSTREAMER_VERSION_PATTERN.findall(index_html)
    }


def latest_common_gstreamer_patch(
    configured_version: str,
    platform_versions: dict[str, set[tuple[int, int, int]]],
) -> str | None:
    """Return the latest patch in the configured minor line available on every platform."""
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", configured_version)
    required_platforms = set(GSTREAMER_PACKAGE_INDEX_URLS)
    if not match or not required_platforms.issubset(platform_versions):
        return None

    major, minor, _patch = (int(part) for part in match.groups())
    matching_sets = [
        {version for version in versions if version[:2] == (major, minor)}
        for platform, versions in platform_versions.items()
        if platform in required_platforms
    ]
    if any(not versions for versions in matching_sets):
        return None

    common_versions = set.intersection(*matching_sets)
    if not common_versions:
        return None
    latest = max(common_versions)
    return ".".join(str(part) for part in latest)


def fetch_latest_common_gstreamer_patch(configured_version: str) -> str | None:
    """Query official SDK indexes for the latest common patch release."""
    platform_versions: dict[str, set[tuple[int, int, int]]] = {}
    for platform, url in GSTREAMER_PACKAGE_INDEX_URLS.items():
        index_html = fetch_text(url)
        if index_html is None:
            log_warn(f"Could not fetch the official GStreamer {platform} SDK index")
            return None
        platform_versions[platform] = parse_gstreamer_package_versions(index_html)

    return latest_common_gstreamer_patch(configured_version, platform_versions)


def check_gstreamer_version() -> None:
    """Check configured and installed GStreamer versions."""
    log_info("Checking GStreamer version...")
    current_version = get_build_config_value(
        "gstreamer.version.default",
        "unknown",
        start=Path(__file__).resolve(),
    )
    print(f"  Configured: GStreamer {current_version}")

    latest_patch = fetch_latest_common_gstreamer_patch(str(current_version))
    if latest_patch:
        print(f"  Latest common patch: GStreamer {latest_patch}")
        current, latest = Version(str(current_version)), Version(latest_patch)
        if current < latest:
            log_warn(f"Newer GStreamer patch available in the same minor line: {latest_patch}")
        elif current == latest:
            log_ok("Using latest GStreamer patch available on every SDK platform")
        else:
            log_warn(
                "Configured GStreamer is newer than the latest patch common to every SDK platform"
            )
    else:
        log_warn("Could not resolve a common GStreamer patch from the official SDK indexes")

    installed = probe_version("gst-launch-1.0", pattern=FULL_VERSION_PATTERN)
    if installed:
        print(f"  Installed: GStreamer {format_version(installed)}")


def check_python_deps(repo_root: Path) -> None:
    """Check installed Python packages for available updates."""
    log_info("Checking Python dependencies...")
    if not (repo_root / PYTHON_MANIFEST).is_file():
        log_warn(f"Python dependency manifest not found: {PYTHON_MANIFEST.as_posix()}")
        return
    print(f"  Manifest: {PYTHON_MANIFEST.as_posix()}")

    try:
        result = run_captured(
            ["uv", "pip", "list", "--python", sys.executable, "--outdated", "--format=json"],
        )
    except OSError:
        log_warn("uv not found; cannot query installed Python package updates")
        return
    if result.returncode != 0:
        log_warn("Could not query installed Python package updates")
        return

    try:
        outdated = json.loads(result.stdout or "[]")
    except json.JSONDecodeError:
        log_warn("Could not parse uv outdated output")
        return

    if not outdated:
        log_ok("No outdated installed Python packages reported")
        return

    print("  Outdated installed packages:")
    for package in outdated:
        name = package.get("name", "unknown")
        current = package.get("version", "?")
        latest = package.get("latest_version", "?")
        print(f"    - {name}: {current} -> {latest}")


def check_build_tools() -> None:
    """Print installed build tool versions."""
    log_info("Checking build tools...")
    for tool in BUILD_TOOLS:
        installed = probe_version(tool)
        print(f"  - {tool}: {format_version(installed) if installed else 'not installed'}")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Check dependency versions and tool availability.")
    parser.add_argument("--qt", action="store_true", help="Check the Qt version")
    parser.add_argument("--gstreamer", action="store_true", help="Check the GStreamer version")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Run the requested dependency checks."""
    args = parse_args(argv)
    repo_root = find_repo_root(Path(__file__))

    checks: list[Callable[[], None]] = []
    if args.qt:
        checks.append(check_qt_version)
    if args.gstreamer:
        checks.append(check_gstreamer_version)
    if not checks:
        checks = [
            check_qt_version,
            check_gstreamer_version,
            lambda: check_python_deps(repo_root),
            check_build_tools,
        ]
    for check in checks:
        check()
        print()
    log_ok("Dependency check complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
