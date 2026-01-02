#!/usr/bin/env python3
"""
Check for outdated dependencies and submodules.

Usage:
    check_deps.py                  # Check all dependencies
    check_deps.py --submodules     # Check only git submodules
    check_deps.py --qt             # Check Qt version
    check_deps.py --update         # Update submodules to latest
    check_deps.py --json           # Output as JSON
    check_deps.py --github-output  # Write to GITHUB_OUTPUT (CI only)
    check_deps.py --check-only     # Exit 1 if any dependency is outdated

Checks:
    - Git submodules vs upstream
    - Qt version vs latest
    - GStreamer version vs latest
    - Python dependencies
    - Build tool versions
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional
from urllib.error import URLError
from urllib.request import urlopen

from common import find_repo_root, Logger, log_info, log_ok, log_warn, log_error, log_verbose


# =============================================================================
# Repository and Config
# =============================================================================


def load_config(repo_root: Path) -> dict[str, Any]:
    """Load build configuration from .github/build-config.json."""
    config_file = repo_root / ".github" / "build-config.json"
    if not config_file.exists():
        return {}
    with open(config_file) as f:
        return json.load(f)


# =============================================================================
# Dependency Check Results
# =============================================================================


@dataclass
class SubmoduleStatus:
    """Status of a git submodule."""

    path: str
    current_hash: str
    branch: str
    behind_count: int
    up_to_date: bool


@dataclass
class ToolStatus:
    """Status of a build tool."""

    name: str
    installed: bool
    version: str = ""


@dataclass
class DependencyReport:
    """Complete dependency check report."""

    submodules: list[SubmoduleStatus] = field(default_factory=list)
    qt_configured: str = ""
    qt_latest_minor: str = ""
    qt_installed: str = ""
    gstreamer_configured: str = ""
    gstreamer_installed: str = ""
    python_outdated: list[dict[str, str]] = field(default_factory=list)
    tools: list[ToolStatus] = field(default_factory=list)
    outdated_count: int = 0

    def to_dict(self) -> dict[str, Any]:
        """Convert report to dictionary for JSON output."""
        return {
            "submodules": [
                {
                    "path": s.path,
                    "current_hash": s.current_hash,
                    "branch": s.branch,
                    "behind_count": s.behind_count,
                    "up_to_date": s.up_to_date,
                }
                for s in self.submodules
            ],
            "qt": {
                "configured": self.qt_configured,
                "latest_minor": self.qt_latest_minor,
                "installed": self.qt_installed,
            },
            "gstreamer": {
                "configured": self.gstreamer_configured,
                "installed": self.gstreamer_installed,
            },
            "python_outdated": self.python_outdated,
            "tools": [
                {"name": t.name, "installed": t.installed, "version": t.version}
                for t in self.tools
            ],
            "outdated_count": self.outdated_count,
        }


# =============================================================================
# Submodule Checks
# =============================================================================


def run_git(args: list[str], cwd: Optional[Path] = None) -> tuple[bool, str]:
    """Run git command and return (success, output)."""
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=60,
        )
        return result.returncode == 0, result.stdout.strip()
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False, ""


def check_submodules(repo_root: Path, update: bool = False) -> list[SubmoduleStatus]:
    """Check git submodule status against upstream."""
    log_info("Checking git submodules...")

    # Initialize submodules if needed
    run_git(["submodule", "update", "--init", "--recursive"], cwd=repo_root)

    # Get submodule status
    success, output = run_git(["submodule", "status", "--recursive"], cwd=repo_root)
    if not success:
        log_warn("Failed to get submodule status")
        return []

    results: list[SubmoduleStatus] = []
    outdated = 0

    for line in output.splitlines():
        if not line.strip():
            continue

        # Parse submodule status: status(1) + hash(40) + space + path + optional (branch)
        parts = line.split()
        if len(parts) < 2:
            continue

        # First char is status (+, -, U, space), rest is hash
        hash_str = parts[0].lstrip("+-U ")
        path = parts[1]

        submodule_path = repo_root / path
        if not submodule_path.is_dir():
            continue

        # Get current info
        _, current_hash = run_git(["rev-parse", "HEAD"], cwd=submodule_path)
        _, branch = run_git(["symbolic-ref", "--short", "HEAD"], cwd=submodule_path)
        if not branch:
            branch = "detached"

        # Fetch updates
        run_git(["fetch", "--quiet"], cwd=submodule_path)

        # Check if behind upstream
        behind_count = 0
        success, count_str = run_git(
            ["rev-list", "--count", "HEAD..@{u}"], cwd=submodule_path
        )
        if success and count_str.isdigit():
            behind_count = int(count_str)

        up_to_date = behind_count == 0
        status = SubmoduleStatus(
            path=path,
            current_hash=current_hash[:8] if current_hash else "unknown",
            branch=branch,
            behind_count=behind_count,
            up_to_date=up_to_date,
        )
        results.append(status)

        if behind_count > 0:
            log_warn(f"{path}: {behind_count} commits behind upstream")
            outdated += 1
        else:
            log_verbose(f"  + {path} (up to date)")

    if outdated == 0:
        log_ok("All submodules up to date")
    else:
        log_warn(f"{outdated} submodule(s) have updates available")
        if update:
            log_info("Updating submodules...")
            success, _ = run_git(
                ["submodule", "update", "--remote", "--merge"], cwd=repo_root
            )
            if success:
                log_ok("Submodules updated")
            else:
                log_error("Failed to update submodules")
        else:
            log_info("Run with --update to update submodules")

    return results


# =============================================================================
# Qt Version Check
# =============================================================================


def fetch_qt_versions() -> list[str]:
    """Fetch available Qt versions from download.qt.io."""
    try:
        with urlopen("https://download.qt.io/official_releases/qt/", timeout=10) as resp:
            html = resp.read().decode("utf-8")
            # Find Qt 6.x versions
            matches = re.findall(r'href="(6\.\d+)/"', html)
            return sorted(set(matches), key=lambda v: tuple(map(int, v.split("."))))
    except (URLError, TimeoutError):
        return []


def check_qt_version(config: dict[str, Any]) -> tuple[str, str, str]:
    """Check Qt version against latest available.

    Returns: (configured, latest_minor, installed)
    """
    log_info("Checking Qt version...")

    configured = config.get("qt_version", "unknown")
    log_verbose(f"  Current: Qt {configured}")

    # Check latest Qt version
    latest_minor = ""
    versions = fetch_qt_versions()
    if versions:
        latest_minor = versions[-1]
        log_verbose(f"  Latest minor: Qt {latest_minor}.x")

        current_minor = ".".join(configured.split(".")[:2])
        if current_minor != latest_minor:
            log_warn(f"Newer Qt minor version available: {latest_minor}")
        else:
            log_ok("Using latest Qt minor version")

    # Check installed Qt
    installed = ""
    qmake = shutil.which("qmake")
    if qmake:
        try:
            result = subprocess.run(
                ["qmake", "--version"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            match = re.search(r"(\d+\.\d+\.\d+)", result.stdout)
            if match:
                installed = match.group(1)
                log_verbose(f"  Installed: Qt {installed}")
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
    elif qt_root := os.environ.get("QT_ROOT_DIR"):
        log_verbose(f"  QT_ROOT_DIR: {qt_root}")

    return configured, latest_minor, installed


# =============================================================================
# GStreamer Version Check
# =============================================================================


def check_gstreamer_version(config: dict[str, Any]) -> tuple[str, str]:
    """Check GStreamer version.

    Returns: (configured, installed)
    """
    log_info("Checking GStreamer version...")

    configured = config.get("gstreamer_version", "unknown")
    log_verbose(f"  Configured: GStreamer {configured}")

    installed = ""
    gst_launch = shutil.which("gst-launch-1.0")
    if gst_launch:
        try:
            result = subprocess.run(
                ["gst-launch-1.0", "--version"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            match = re.search(r"(\d+\.\d+\.\d+)", result.stdout)
            if match:
                installed = match.group(1)
                log_verbose(f"  Installed: GStreamer {installed}")
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass

    return configured, installed


# =============================================================================
# Python Dependencies Check
# =============================================================================


def check_python_deps(repo_root: Path) -> list[dict[str, str]]:
    """Check for outdated Python dependencies.

    Returns: List of outdated packages with their versions.
    """
    log_info("Checking Python dependencies...")

    req_files = ["requirements.txt", "docs/requirements.txt"]
    outdated_packages: list[dict[str, str]] = []

    pip = shutil.which("pip") or shutil.which("pip3")
    if not pip:
        log_warn("pip not found")
        return []

    for req in req_files:
        req_path = repo_root / req
        if not req_path.exists():
            continue

        log_verbose(f"  Checking {req}...")
        try:
            result = subprocess.run(
                [pip, "list", "--outdated", "--format=json"],
                capture_output=True,
                text=True,
                timeout=60,
            )
            if result.returncode == 0 and result.stdout:
                packages = json.loads(result.stdout)
                for pkg in packages:
                    outdated_packages.append(
                        {
                            "name": pkg.get("name", ""),
                            "current": pkg.get("version", ""),
                            "latest": pkg.get("latest_version", ""),
                        }
                    )
                    log_verbose(
                        f"    {pkg['name']}: {pkg['version']} -> {pkg['latest_version']}"
                    )
        except (subprocess.TimeoutExpired, FileNotFoundError, json.JSONDecodeError):
            pass

    return outdated_packages


# =============================================================================
# Build Tools Check
# =============================================================================


def get_tool_version(name: str) -> tuple[bool, str]:
    """Get version of a build tool.

    Returns: (installed, version_string)
    """
    tool_path = shutil.which(name)
    if not tool_path:
        return False, ""

    try:
        result = subprocess.run(
            [name, "--version"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        # Get first line of output
        version = result.stdout.split("\n")[0] if result.stdout else "unknown"
        return True, version
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return True, "unknown"


def check_build_tools() -> list[ToolStatus]:
    """Check build tool versions."""
    log_info("Checking build tools...")

    tools = ["cmake", "ninja", "ccache", "clang-format", "clang-tidy"]
    results: list[ToolStatus] = []

    for tool in tools:
        installed, version = get_tool_version(tool)
        results.append(ToolStatus(name=tool, installed=installed, version=version))

        if installed:
            log_verbose(f"  + {tool}: {version}")
        else:
            log_verbose(f"  - {tool}: not installed")

    return results


# =============================================================================
# Output Formatting
# =============================================================================


def format_github_output(report: DependencyReport) -> str:
    """Format report for GITHUB_OUTPUT."""
    lines = [
        f"submodules_outdated={sum(1 for s in report.submodules if not s.up_to_date)}",
        f"qt_configured={report.qt_configured}",
        f"qt_latest={report.qt_latest_minor}",
        f"gstreamer_configured={report.gstreamer_configured}",
        f"gstreamer_installed={report.gstreamer_installed}",
        f"outdated_count={report.outdated_count}",
    ]
    return "\n".join(lines)


# =============================================================================
# Main
# =============================================================================


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check for outdated dependencies and submodules",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--submodules",
        action="store_true",
        help="Check only git submodules",
    )
    parser.add_argument(
        "--qt",
        action="store_true",
        help="Check only Qt version",
    )
    parser.add_argument(
        "--gstreamer",
        action="store_true",
        help="Check only GStreamer version",
    )
    parser.add_argument(
        "--tools",
        action="store_true",
        help="Check only build tools",
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="Update submodules to latest",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output as JSON",
    )
    parser.add_argument(
        "--github-output",
        action="store_true",
        help="Write to GITHUB_OUTPUT (GitHub Actions only)",
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Exit with code 1 if any dependency is outdated",
    )

    args = parser.parse_args()

    # Determine what to check
    check_all = not (args.submodules or args.qt or args.gstreamer or args.tools)

    try:
        repo_root = find_repo_root()
    except FileNotFoundError as e:
        log_error(str(e))
        return 1

    config = load_config(repo_root)
    report = DependencyReport()

    # Run checks
    if check_all or args.submodules:
        report.submodules = check_submodules(repo_root, update=args.update)
        report.outdated_count += sum(1 for s in report.submodules if not s.up_to_date)
        if check_all:
            log_verbose("")

    if check_all or args.qt:
        report.qt_configured, report.qt_latest_minor, report.qt_installed = (
            check_qt_version(config)
        )
        if report.qt_latest_minor and report.qt_configured:
            current_minor = ".".join(report.qt_configured.split(".")[:2])
            if current_minor != report.qt_latest_minor:
                report.outdated_count += 1
        if check_all:
            log_verbose("")

    if check_all or args.gstreamer:
        report.gstreamer_configured, report.gstreamer_installed = (
            check_gstreamer_version(config)
        )
        if check_all:
            log_verbose("")

    if check_all or args.tools:
        report.tools = check_build_tools()
        if check_all:
            log_verbose("")

    # Output results
    if args.json:
        print(json.dumps(report.to_dict(), indent=2))
    elif args.github_output:
        github_output = os.environ.get("GITHUB_OUTPUT")
        if not github_output:
            log_error("GITHUB_OUTPUT not set (not running in GitHub Actions?)")
            return 1
        with open(github_output, "a") as f:
            f.write(format_github_output(report) + "\n")
        log_ok("Wrote results to GITHUB_OUTPUT")
    else:
        log_ok("Dependency check complete")

    # Exit with error if check-only and there are outdated deps
    if args.check_only and report.outdated_count > 0:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
