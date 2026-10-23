"""Git helpers for QGC tooling.

Centralises the ``["git", ...]`` subprocess shelling used by CI scripts and
dev tools. Provides default-branch discovery, pull request diff refs, diff bases
and changed line ranges, plus a generic ``run_git`` wrapper backed by
:mod:`common.proc`.
"""

from __future__ import annotations

import re
import sys
from typing import TYPE_CHECKING

from .proc import run_captured

if TYPE_CHECKING:
    import subprocess
    from pathlib import Path

__all__ = [
    "ZERO_SHA",
    "changed_ranges",
    "diff_base",
    "get_default_branch_ref",
    "merge_checkout_parent",
    "pr_diff_refs",
    "run_git",
]

# The base SHA GitHub sends for a push that creates a branch.
ZERO_SHA = "0" * 40
_FALLBACK_REFS: tuple[str, ...] = ("master", "main", "origin/master", "origin/main")
_HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", re.MULTILINE)


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


def merge_checkout_parent(head: str | None, *, cwd: Path | None = None) -> str | None:
    """Return HEAD's first parent when HEAD merges *head* into it, else None.

    A pull_request checkout is GitHub's merge of the PR head into the current base-branch tip,
    which can be newer than the event's base SHA. Diffing against that first parent keeps newer
    base-branch changes out of the PR's changes and needs only a depth-2 fetch.
    """
    if not head or head == "HEAD":
        return None
    parents = run_git("rev-list", "--parents", "-n", "1", "HEAD", cwd=cwd).stdout.split()
    resolved = run_git(
        "rev-parse", "--verify", "--quiet", "--end-of-options", f"{head}^{{commit}}", cwd=cwd
    ).stdout.strip()
    if len(parents) == 3 and resolved and parents[2] == resolved:
        return parents[1]
    return None


def pr_diff_refs(
    base: str | None, head: str | None = None, *, cwd: Path | None = None
) -> tuple[str, str] | None:
    """Return the (base, head) refs whose merge-base diff is the change to check, or None.

    *base* and *head* are a pull request's base and head SHAs, or a push's before and after SHAs
    (PR_BASE_SHA and PR_HEAD_SHA in CI). A pull_request merge checkout of *head* yields its first
    parent and HEAD. Otherwise an unset *base* means the default branch and an unset *head* HEAD.
    None selects everything: a new-branch push, no default branch, or no merge base in the fetched
    history (such as a shallow clone); the reason goes to stderr.
    """
    if base == ZERO_SHA:
        reason = "New branch push"
    elif parent := merge_checkout_parent(head, cwd=cwd):
        return parent, "HEAD"
    elif not (base := base or get_default_branch_ref(cwd)):
        reason = "Default branch not available"
    else:
        head = head or "HEAD"
        if run_git("merge-base", "--end-of-options", base, head, cwd=cwd).returncode == 0:
            return base, head
        reason = f"No merge base of {base} and {head} in the fetched history"
    print(f"{reason}; checking everything", file=sys.stderr)
    return None


def diff_base(
    ref: str | None = None, head: str = "HEAD", *, cwd: Path | str | None = None
) -> str | None:
    """Return the merge base of *ref* and *head*, or the HEAD commit when *ref* is None.

    None means the repository has no commits yet. Raises
    :class:`subprocess.CalledProcessError` outside a work tree or for an unknown ref.
    """
    run_git("rev-parse", "--show-toplevel", cwd=cwd, check=True)
    if ref:
        return run_git("merge-base", ref, head, cwd=cwd, check=True).stdout.strip()
    result = run_git("rev-parse", "--verify", "HEAD", cwd=cwd)
    return result.stdout.strip() if result.returncode == 0 else None


def changed_ranges(
    path: Path | str, base: str | None, *, cwd: Path | str | None = None
) -> list[tuple[int, int]] | None:
    """Return the 1-based inclusive line ranges of *path* that differ from *base*.

    None requests a whole-file check; an empty list means deletion-only changes.
    """
    if base is None:
        return None
    diff = run_git(
        "diff",
        "--no-ext-diff",
        "--no-textconv",
        "--no-color",
        "--no-renames",
        "--unified=0",
        base,
        "--",
        str(path),
        cwd=cwd,
        check=True,
    ).stdout
    if not diff or "\nBinary files " in diff:
        # Untracked files and unchanged files selected explicitly need a full check.
        return None
    ranges = []
    for match in _HUNK_RE.finditer(diff):
        start = int(match[1])
        count = int(match[2]) if match[2] is not None else 1
        if count:
            ranges.append((start, start + count - 1))
    return ranges
