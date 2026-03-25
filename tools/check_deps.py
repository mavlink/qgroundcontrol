#!/usr/bin/env python3
"""Check dependency and tool versions used by QGroundControl development."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.build_config import get_build_config_value
from common.logging import log_error, log_info, log_ok, log_warn

QT_RELEASES_URL = "https://download.qt.io/official_releases/qt/"
REQ_FILES = [
    Path("requirements.txt"),
    Path("docs/requirements.txt"),
    Path("tools/pyproject.toml"),
]
BUILD_TOOLS = ["cmake", "ninja", "ccache", "clang-format", "clang-tidy"]


def run_git(args: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    """Run a git command and capture stdout/stderr."""
    return subprocess.run(
        ["git", *args],
        cwd=cwd,
        capture_output=True,
        text=True,
        check=False,
    )


def parse_submodule_paths(repo_root: Path) -> list[Path]:
    """Return existing submodule paths from ``git submodule status``."""
    result = run_git(["submodule", "status", "--recursive"], repo_root)
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
    run_git(["submodule", "update", "--init", "--recursive"], repo_root)

    outdated: list[tuple[Path, int]] = []
    for submodule in parse_submodule_paths(repo_root):
        run_git(["fetch", "--quiet"], submodule)
        upstream = run_git(["rev-parse", "@{u}"], submodule)
        if upstream.returncode != 0:
            print(f"  - {submodule.relative_to(repo_root)} (no upstream)")
            continue

        behind_result = run_git(["rev-list", "--count", "HEAD..@{u}"], submodule)
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
        result = run_git(["submodule", "update", "--remote", "--merge"], repo_root)
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
        with httpx.Client(timeout=15) as client:
            response = client.get(QT_RELEASES_URL)
            response.raise_for_status()
            body = response.text
    except Exception:
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
    current_version = get_build_config_value("qt_version", "unknown", start=Path(__file__).resolve())
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
        result = subprocess.run(candidate, capture_output=True, text=True, check=False)
        if result.returncode != 0:
            continue
        match = re.search(r"\d+\.\d+\.\d+", result.stdout)
        if match:
            print(f"  Installed: Qt {match.group(0)}")
            return


def check_gstreamer_version() -> None:
    """Check configured and installed GStreamer versions."""
    log_info("Checking GStreamer version...")
    current_version = get_build_config_value(
        "gstreamer_version",
        "unknown",
        start=Path(__file__).resolve(),
    )
    print(f"  Configured: GStreamer {current_version}")

    result = subprocess.run(
        ["gst-launch-1.0", "--version"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        return
    match = re.search(r"\d+\.\d+\.\d+", result.stdout)
    if match:
        print(f"  Installed: GStreamer {match.group(0)}")


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

    result = subprocess.run(
        [sys.executable, "-m", "pip", "list", "--outdated", "--format=json"],
        capture_output=True,
        text=True,
        check=False,
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
        result = subprocess.run([tool, "--version"], capture_output=True, text=True, check=False)
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
    parser.add_argument("--update", action="store_true", help="Update submodules to latest upstream")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Run the requested dependency checks."""
    args = parse_args(argv)
    repo_root = Path(__file__).resolve().parent.parent

    check_all = not args.submodules and not args.qt
    if check_all or args.submodules:
        check_submodules(repo_root, update=args.update)
    if check_all:
        print()
        check_qt_version()
        print()
        check_gstreamer_version()
        print()
        check_python_deps(repo_root)
        print()
        check_build_tools()
    elif args.qt:
        check_qt_version()

    print()
    log_ok("Dependency check complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
