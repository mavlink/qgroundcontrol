#!/usr/bin/env python3
"""Detect whether a CI build is needed based on changed files and platform.

Computes the git diff for the current event (pull_request, push, merge_group)
and matches changed files against platform-specific patterns.

Usage:
    python3 .github/scripts/detect_changes.py --platform linux
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from typing import Sequence

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output

# Patterns that trigger a build for ANY platform
_COMMON_PATTERNS: list[str] = [
    r"^src/",
    r"^CMakeLists\.txt$",
    r"^cmake/",
    r"^libs/",
    r"^resources/",
    r"^translations/",
    r"^.*\.qrc$",
    r"^test/",
    r"^\.github/actions/",
    r"^\.github/scripts/",
    r"^\.github/build-config\.json$",
]

# Per-platform additional patterns
_PLATFORM_PATTERNS: dict[str, list[str]] = {
    "android": [r"^android/"],
    "docker-linux": [
        r"^deploy/docker/Dockerfile-build-ubuntu$",
        r"^deploy/linux/",
    ],
    "docker-android": [
        r"^deploy/docker/Dockerfile-build-android$",
        r"^android/",
        r"^deploy/android/",
    ],
}

# Per-platform tools/setup patterns
_SETUP_PATTERNS: dict[str, list[str]] = {
    "linux": [r"^tools/setup/.*debian", r"^tools/setup/install_dependencies\.py$"],
    "windows": [r"^tools/setup/.*windows"],
    "macos": [r"^tools/setup/.*macos"],
    "android": [r"^tools/setup/"],
    "ios": [r"^tools/setup/.*ios", r"^tools/setup/.*macos"],
    "docker-linux": [r"^tools/setup/"],
    "docker-android": [r"^tools/setup/"],
}


def workflow_name_for_platform(platform: str) -> str:
    """Map a platform to its workflow YAML filename (without extension)."""
    if platform.startswith("docker-"):
        return "docker"
    return platform


def build_patterns(platform: str) -> list[re.Pattern[str]]:
    """Build the list of compiled regex patterns for a platform."""
    wf = workflow_name_for_platform(platform)
    raw: list[str] = list(_COMMON_PATTERNS)
    raw.append(rf"^deploy/{re.escape(platform)}/")
    raw.append(rf"^\.github/workflows/{re.escape(wf)}\.yml$")
    raw.extend(_PLATFORM_PATTERNS.get(platform, []))
    raw.extend(_SETUP_PATTERNS.get(platform, []))
    return [re.compile(p) for p in raw]


def has_relevant_changes(files: Sequence[str], platform: str) -> bool:
    """Check if any changed file matches the platform's patterns."""
    patterns = build_patterns(platform)
    for f in files:
        if not f:
            continue
        for p in patterns:
            if p.search(f):
                return True
    return False


# ---------------------------------------------------------------------------
# Git helpers — these call out to git as subprocesses
# ---------------------------------------------------------------------------

_NULL_SHA = "0" * 40


def _run_git(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        capture_output=True,
        text=True,
        check=check,
    )


def _ensure_commit(sha: str) -> bool:
    """Ensure a commit exists locally; fetch it shallowly if needed."""
    if not sha:
        return True
    r = _run_git("cat-file", "-e", f"{sha}^{{commit}}", check=False)
    if r.returncode == 0:
        return True
    _run_git("fetch", "--no-tags", "--depth=1", "origin", sha, check=False)
    return _run_git("cat-file", "-e", f"{sha}^{{commit}}", check=False).returncode == 0


def _diff_names(sha_a: str, sha_b: str) -> list[str] | None:
    """Return changed file names between two commits, or None on failure."""
    r = _run_git("diff", "--name-only", sha_a, sha_b, check=False)
    if r.returncode != 0:
        return None
    return r.stdout.strip().splitlines()


def _tree_names(sha: str) -> list[str]:
    """List files changed in a single commit."""
    r = _run_git("diff-tree", "--no-commit-id", "--name-only", "-r", sha, check=False)
    return r.stdout.strip().splitlines() if r.returncode == 0 else []


def get_changed_files() -> list[str] | None:
    """Determine changed files from environment variables set by GitHub Actions.

    Returns None if the diff cannot be computed reliably (caller should force build).
    """
    event = os.environ.get("EVENT_NAME", "")

    if event == "pull_request":
        base = os.environ.get("PR_BASE_SHA", "")
        head = os.environ.get("PR_HEAD_SHA", "")
        if not _ensure_commit(base) or not _ensure_commit(head):
            return None
        return _diff_names(base, head)

    if event == "push":
        before = os.environ.get("PUSH_BEFORE_SHA", "")
        current = os.environ.get("CURRENT_SHA", "")
        if before and before != _NULL_SHA:
            if not _ensure_commit(before) or not _ensure_commit(current):
                return None
            return _diff_names(before, current)
        return _tree_names(current)

    if event == "merge_group":
        merge_base = os.environ.get("MERGE_BASE_SHA", "")
        current = os.environ.get("CURRENT_SHA", "")
        if merge_base:
            if not _ensure_commit(merge_base) or not _ensure_commit(current):
                return None
            return _diff_names(merge_base, current)

    current = os.environ.get("CURRENT_SHA", "")
    return _tree_names(current) if current else None


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Detect CI-relevant file changes for a platform.")
    parser.add_argument("--platform", required=True,
                        help="Platform (linux, windows, macos, android, ios, docker-linux, docker-android)")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    platform = args.platform

    changed = get_changed_files()
    if changed is None:
        print("::warning::Unable to compute changed files reliably; forcing build for safety.")
        write_github_output({"any": "true"})
        return 0

    print("Changed files:")
    for f in changed:
        print(f"  {f}")

    any_changed = has_relevant_changes(changed, platform)
    write_github_output({"any": "true" if any_changed else "false"})
    print(f"Results: any={any_changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
