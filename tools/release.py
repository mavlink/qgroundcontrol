#!/usr/bin/env python3
"""Run semantic-release for automated versioning and GitHub Releases.

Examples:
    ./tools/release.py              # Dry-run (preview what would happen)
    ./tools/release.py --run        # Actually create release (CI only)
    ./tools/release.py --install    # Install semantic-release dependencies locally

Requires: Node.js 18+, npm.

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

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common import find_repo_root, probe_version
from common.io import chdir
from common.logging import log_error, log_info, log_ok

# Pin versions for reproducibility + supply chain (bumped via Dependabot npm ecosystem)
SR_VERSION = "24.2.5"
SR_PACKAGES: tuple[str, ...] = (
    f"semantic-release@{SR_VERSION}",
    "@semantic-release/exec@7.1.0",
    "conventional-changelog-conventionalcommits@8.0.0",
)
MIN_NODE_MAJOR = 18


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
    subprocess.run(["npm", "install", "--save-dev", *SR_PACKAGES], check=True)
    log_ok("Dependencies installed")
    return 0


def run_semantic_release(*, dry_run: bool) -> int:
    cmd = ["npx", "--yes", f"semantic-release@{SR_VERSION}"]
    if dry_run:
        cmd.append("--dry-run")
        log_info("Running semantic-release in dry-run mode...")
        log_info("This will show what would happen without making changes")
    else:
        log_info("Running semantic-release...")
    print()
    return subprocess.run(cmd, check=False).returncode


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
        return exit_code


if __name__ == "__main__":
    sys.exit(main())
