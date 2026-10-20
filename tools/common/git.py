"""Git helpers for QGC tooling.

Centralises the ``["git", ...]`` subprocess shelling used by CI scripts and
dev tools. Provides default-branch discovery plus a generic ``run_git``
wrapper backed by :mod:`common.proc`.
"""

from __future__ import annotations

import re
from typing import TYPE_CHECKING

from .proc import run_captured

if TYPE_CHECKING:
    import subprocess
    from pathlib import Path

__all__ = ["get_changed_line_ranges", "get_default_branch_ref", "run_git"]

_FALLBACK_REFS: tuple[str, ...] = ("master", "main", "origin/master", "origin/main")


def run_git(
    *args: str,
    cwd: Path | str | None = None,
    check: bool = False,
    timeout: float | None = None,
) -> subprocess.CompletedProcess[str]:
    """Run ``git *args`` capturing stdout/stderr as text. Thin wrapper over run_captured."""
    return run_captured(["git", *args], cwd=cwd, check=check, timeout=timeout)


def get_default_branch_ref(repo_root: Path | None = None) -> str | None:
    """Return the local name of the default branch, or None if undiscoverable.

    Tries ``refs/remotes/origin/HEAD`` first (post-clone canonical), then
    probes the usual ``master``/``main`` variants. ``repo_root`` selects the
    git directory; ``None`` uses the caller's CWD.
    """
    head = run_git("symbolic-ref", "refs/remotes/origin/HEAD", "--short", cwd=repo_root)
    if head.returncode == 0:
        return head.stdout.strip().removeprefix("origin/")

    for ref in _FALLBACK_REFS:
        probe = run_git("rev-parse", "--verify", ref, cwd=repo_root)
        if probe.returncode == 0:
            return ref
    return None


def get_changed_line_ranges(
    repo_root: Path, base: str, extensions: tuple[str, ...]
) -> dict[Path, list[tuple[int, int]]]:
    """Return existing changed files and inclusive new-side ranges in base...HEAD.

    Keep deletion-only files with empty ranges: their compilation can still fail.
    NUL-delimited names avoid Git's quoting of spaces and non-ASCII filenames.
    """
    revision = f"{base}...HEAD"

    def diff(*args: str) -> str:
        result = run_git(
            "--literal-pathspecs",
            "diff",
            "--no-ext-diff",
            "--no-textconv",
            "--no-color",
            "--find-renames",
            *args,
            cwd=repo_root,
        )
        if result.returncode != 0:
            raise RuntimeError(f"Unable to determine changed lines: {result.stderr.strip()}")
        return result.stdout

    names = iter(diff("--name-status", "-z", revision, "--").split("\0"))
    changed: dict[Path, list[tuple[int, int]]] = {}
    for status in names:
        if not status:
            continue
        old_path = next(names)
        new_path = next(names) if status.startswith(("R", "C")) else old_path
        file = repo_root / new_path
        if status == "D" or file.suffix not in extensions or not file.is_file():
            continue
        patch = diff("--unified=0", revision, "--", *dict.fromkeys((old_path, new_path)))
        ranges = []
        for match in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", patch, re.MULTILINE):
            start = int(match[1])
            count = int(match[2]) if match[2] is not None else 1
            if count:
                ranges.append((start, start + count - 1))
        changed[file.resolve()] = ranges
    return changed
