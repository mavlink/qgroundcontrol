#!/usr/bin/env python3
"""Run semantic-release for automated versioning and GitHub Releases.

Examples:
    ./tools/release.py              # Dry-run (preview what would happen)
    ./tools/release.py --run        # Actually create release (CI only)
    ./tools/release.py --run --github-output  # Also write new_release/version step outputs
    ./tools/release.py --install    # Install semantic-release dependencies locally

Requires: Node.js 24+, npm.

Environment:
    GITHUB_TOKEN - required for --run mode (set automatically in CI)
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

import _bootstrap as _bootstrap
from common.file_traversal import find_repo_root
from common.gh_actions import gh_error, write_github_output
from common.io import chdir
from common.logging import log_error, log_info, log_ok
from common.tool_version import probe_version

RELEASE_DIR = Path(__file__).resolve().parent / "release"
# @semantic-release/exec's verifyReleaseCmd writes the next version here only when commits warrant
# a release; reading a file is immune to log-format drift across semantic-release major versions.
VERSION_FILE = RELEASE_DIR / ".release-version"
MIN_NODE_MAJOR = 24


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-r", "--run", action="store_true", help="Actually create the release (CI only)"
    )
    parser.add_argument(
        "-i", "--install", action="store_true", help="Install semantic-release deps locally"
    )
    parser.add_argument(
        "--github-output",
        action="store_true",
        help="Write new_release and version to $GITHUB_OUTPUT (CI only)",
    )
    return parser.parse_args(argv)


def repo_root() -> Path:
    return find_repo_root(Path(__file__))


def check_node() -> int:
    """Return Node major version, or exit with error."""
    version = probe_version("node")
    if version is None:
        log_error("Node.js not found or unreadable. Install from: https://nodejs.org/")
        sys.exit(1)
    major = version[0]
    if major < MIN_NODE_MAJOR:
        log_error(
            f"Node.js {MIN_NODE_MAJOR}+ required (found: {'.'.join(str(p) for p in version)})"
        )
        sys.exit(1)
    return major


def handle_install() -> int:
    if not shutil.which("npm"):
        log_error("npm not found")
        return 1
    log_info("Installing semantic-release dependencies locally...")
    subprocess.run(["npm", "ci", "--prefix", str(RELEASE_DIR)], check=True)
    log_ok("Dependencies installed")
    return 0


def run_semantic_release(*, dry_run: bool) -> int:
    cmd = ["npm", "run", "release", "--prefix", str(RELEASE_DIR), "--"]
    if dry_run:
        cmd.append("--dry-run")
        log_info("Running semantic-release in dry-run mode...")
        log_info("This will show what would happen without making changes")
    else:
        log_info("Running semantic-release...")
    print()
    return subprocess.run(cmd, check=False).returncode


def write_release_outputs(version_file: Path) -> int:
    """Publish whether semantic-release made a release, and its version, as step outputs."""
    if not version_file.is_file():
        write_github_output({"new_release": "false"})
        log_info("No new release")
        return 0
    version = "".join(version_file.read_text(encoding="utf-8").split())
    if not version:
        gh_error("semantic-release wrote an empty version file")
        return 1
    write_github_output({"new_release": "true", "version": version})
    log_ok(f"New release: v{version}")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    check_node()

    with chdir(repo_root()):
        if args.install:
            return handle_install()

        if not Path(".releaserc.json").is_file():
            log_error(".releaserc.json not found in repository root")
            return 1

        if args.run and not os.environ.get("GITHUB_TOKEN"):
            log_error("GITHUB_TOKEN required for --run mode (CI-only)")
            return 1

        if args.github_output:
            if not os.environ.get("GITHUB_OUTPUT"):
                log_error("GITHUB_OUTPUT required for --github-output (CI-only)")
                return 1
            VERSION_FILE.unlink(missing_ok=True)

        exit_code = run_semantic_release(dry_run=not args.run)
        print()
        if exit_code == 0:
            log_ok(
                "Release dry-run complete (no changes made)" if not args.run else "Release complete"
            )
            if not args.run:
                log_info("Run with --run to create an actual release")
        else:
            log_error(f"semantic-release exited with code {exit_code}")
        if exit_code == 0 and args.github_output:
            return write_release_outputs(VERSION_FILE)
        return exit_code


if __name__ == "__main__":
    sys.exit(main())
