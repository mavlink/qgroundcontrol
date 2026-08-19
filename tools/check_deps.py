#!/usr/bin/env python3
"""Check dependency and tool versions used by QGroundControl development."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.build_config import get_build_config_value
from common.file_traversal import find_repo_root
from common.git import run_git
from common.logging import log_error, log_info, log_ok, log_warn
from common.proc import run_captured

QT_RELEASES_URL = "https://download.qt.io/official_releases/qt/"
GSTREAMER_PACKAGE_INDEX_URLS = {
    "android": "https://gstreamer.freedesktop.org/data/pkg/android/",
    "ios": "https://gstreamer.freedesktop.org/data/pkg/ios/",
    "macos": "https://gstreamer.freedesktop.org/data/pkg/macos/",
    "windows": "https://gstreamer.freedesktop.org/data/pkg/windows/",
}
GSTREAMER_VERSION_PATTERN = re.compile(r'href=["\'](\d+)\.(\d+)\.(\d+)/["\']')
REQ_FILES = [
    Path("requirements.txt"),
    Path("docs/requirements.txt"),
    Path("tools/pyproject.toml"),
]
BUILD_TOOLS = ["cmake", "ninja", "ccache", "clang-format", "clang-tidy"]


def parse_submodule_paths(repo_root: Path) -> list[Path]:
    """Return existing submodule paths from ``git submodule status``."""
    result = run_git("submodule", "status", "--recursive", cwd=repo_root)
    if result.returncode != 0:
        return []

    paths: list[Path] = []
    for line in result.stdout.splitlines():
        parts = line.strip().split()
        if len(parts) < 2:
            continue
        path = repo_root / parts[1]
        if path.is_dir():
            paths.append(path)
    return paths


def check_submodules(repo_root: Path, *, update: bool) -> None:
    """Check whether git submodules are behind their upstream branch."""
    log_info("Checking git submodules...")
    run_git("submodule", "update", "--init", "--recursive", cwd=repo_root)

    outdated: list[tuple[Path, int]] = []
    for submodule in parse_submodule_paths(repo_root):
        run_git("fetch", "--quiet", cwd=submodule)
        upstream = run_git("rev-parse", "@{u}", cwd=submodule)
        if upstream.returncode != 0:
            print(f"  - {submodule.relative_to(repo_root)} (no upstream)")
            continue

        behind_result = run_git("rev-list", "--count", "HEAD..@{u}", cwd=submodule)
        behind = int(behind_result.stdout.strip() or "0") if behind_result.returncode == 0 else 0
        if behind > 0:
            outdated.append((submodule, behind))
            log_warn(f"{submodule.relative_to(repo_root)}: {behind} commits behind upstream")
        else:
            print(f"  - {submodule.relative_to(repo_root)} (up to date)")

    if not outdated:
        log_ok("All submodules up to date")
        return

    log_warn(f"{len(outdated)} submodule(s) have updates available")
    if update:
        log_info("Updating submodules...")
        result = run_git("submodule", "update", "--remote", "--merge", cwd=repo_root)
        if result.returncode == 0:
            log_ok("Submodules updated")
        else:
            log_error("Submodule update failed")
            if result.stderr:
                print(result.stderr.strip(), file=sys.stderr)
    else:
        log_info("Run with --update to update submodules")


def fetch_latest_qt_minor() -> str | None:
    """Return the latest Qt 6 minor version available on download.qt.io."""
    try:
        import httpx
    except ImportError:
        return None

    try:
        with httpx.Client(timeout=15) as client:
            response = client.get(QT_RELEASES_URL)
            response.raise_for_status()
            body = response.text
    except (httpx.HTTPError, OSError):
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

    for candidate in (["qmake", "--version"], ["qtpaths", "--qt-version"]):
        result = run_captured(candidate)
        if result.returncode != 0:
            continue
        match = re.search(r"\d+\.\d+\.\d+", result.stdout)
        if match:
            print(f"  Installed: Qt {match.group(0)}")
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


def fetch_gstreamer_package_index(url: str) -> str | None:
    """Fetch an SDK index, falling back to curl when Python networking is constrained."""
    try:
        import httpx
    except ImportError:
        httpx = None  # type: ignore[assignment]

    if httpx is not None:
        try:
            response = httpx.get(url, timeout=15, follow_redirects=True)
            response.raise_for_status()
            return response.text
        except (httpx.HTTPError, OSError):
            pass

    try:
        result = run_captured(
            [
                "curl",
                "--fail",
                "--location",
                "--silent",
                "--show-error",
                "--max-time",
                "15",
                "--retry",
                "2",
                "--retry-all-errors",
                url,
            ]
        )
    except FileNotFoundError:
        return None
    return result.stdout if result.returncode == 0 else None


def fetch_latest_common_gstreamer_patch(configured_version: str) -> str | None:
    """Query official SDK indexes for the latest common patch release."""
    platform_versions: dict[str, set[tuple[int, int, int]]] = {}
    for platform, url in GSTREAMER_PACKAGE_INDEX_URLS.items():
        index_html = fetch_gstreamer_package_index(url)
        if index_html is None:
            log_warn(f"Could not fetch the official GStreamer {platform} SDK index")
            return None
        platform_versions[platform] = parse_gstreamer_package_versions(index_html)

    return latest_common_gstreamer_patch(configured_version, platform_versions)


def check_gstreamer_version() -> bool | None:
    """Check configured and installed GStreamer versions."""
    log_info("Checking GStreamer version...")
    current_version = get_build_config_value(
        "gstreamer.version.default",
        "unknown",
        start=Path(__file__).resolve(),
    )
    print(f"  Configured: GStreamer {current_version}")

    latest_patch = fetch_latest_common_gstreamer_patch(str(current_version))
    is_current: bool | None = None
    if latest_patch:
        print(f"  Latest common patch: GStreamer {latest_patch}")
        current_parts = tuple(int(part) for part in str(current_version).split("."))
        latest_parts = tuple(int(part) for part in latest_patch.split("."))
        is_current = current_parts == latest_parts
        if current_parts < latest_parts:
            log_warn(f"Newer GStreamer patch available in the same minor line: {latest_patch}")
        elif current_parts == latest_parts:
            log_ok("Using latest GStreamer patch available on every SDK platform")
        else:
            log_warn(
                "Configured GStreamer is newer than the latest patch common to every SDK platform"
            )
    else:
        log_warn("Could not resolve a common GStreamer patch from the official SDK indexes")

    try:
        result = run_captured(["gst-launch-1.0", "--version"])
    except FileNotFoundError:
        return is_current
    if result.returncode == 0:
        match = re.search(r"\d+\.\d+\.\d+", result.stdout)
        if match:
            print(f"  Installed: GStreamer {match.group(0)}")
    return is_current


def find_python_manifests(repo_root: Path) -> list[Path]:
    """Return dependency manifest files present in the repo."""
    return [path for path in REQ_FILES if (repo_root / path).exists()]


def check_python_deps(repo_root: Path) -> None:
    """Check installed Python packages for available updates."""
    log_info("Checking Python dependencies...")
    manifests = find_python_manifests(repo_root)
    if not manifests:
        log_warn("No Python dependency manifests found")
        return

    print("  Manifests:")
    for manifest in manifests:
        print(f"    - {manifest}")

    result = run_captured(
        [sys.executable, "-m", "pip", "list", "--outdated", "--format=json"],
    )
    if result.returncode != 0:
        log_warn("Could not query installed Python package updates")
        return

    try:
        outdated = json.loads(result.stdout or "[]")
    except json.JSONDecodeError:
        log_warn("Could not parse pip outdated output")
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
        result = run_captured([tool, "--version"])
        if result.returncode != 0:
            print(f"  - {tool}: not installed")
            continue
        first_line = result.stdout.splitlines()[0] if result.stdout else "unknown"
        print(f"  - {tool}: {first_line}")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Check dependency versions and tool availability.")
    parser.add_argument("--submodules", action="store_true", help="Check only git submodules")
    parser.add_argument("--qt", action="store_true", help="Check only Qt version")
    parser.add_argument("--gstreamer", action="store_true", help="Check only GStreamer version")
    parser.add_argument(
        "--fail-if-outdated",
        action="store_true",
        help="Fail unless --gstreamer resolves the configured version as the latest common patch",
    )
    parser.add_argument(
        "--update", action="store_true", help="Update submodules to latest upstream"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Run the requested dependency checks."""
    args = parse_args(argv)
    repo_root = find_repo_root(Path(__file__))
    if args.fail_if_outdated and not args.gstreamer:
        log_error("--fail-if-outdated requires --gstreamer")
        return 2

    check_all = not args.submodules and not args.qt and not args.gstreamer
    gstreamer_is_current: bool | None = None
    if check_all or args.submodules:
        check_submodules(repo_root, update=args.update)
    if check_all:
        print()
        check_qt_version()
        print()
        gstreamer_is_current = check_gstreamer_version()
        print()
        check_python_deps(repo_root)
        print()
        check_build_tools()
    elif args.qt:
        check_qt_version()
    elif args.gstreamer:
        gstreamer_is_current = check_gstreamer_version()

    if args.fail_if_outdated and gstreamer_is_current is not True:
        log_error("Configured GStreamer is not the latest patch available on every SDK platform")
        return 1

    print()
    log_ok("Dependency check complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
