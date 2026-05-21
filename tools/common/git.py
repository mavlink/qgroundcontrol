"""Git helpers for QGC tooling.

Currently provides default-branch discovery used by analyzers and pre-commit
to scope "changed files" comparisons.
"""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

__all__ = ["get_default_branch_ref"]

if TYPE_CHECKING:
    from pathlib import Path

_FALLBACK_REFS: tuple[str, ...] = ("master", "main", "origin/master", "origin/main")


def get_default_branch_ref(repo_root: Path | None = None) -> str | None:
    """Return the local name of the default branch, or None if undiscoverable.

    Tries ``refs/remotes/origin/HEAD`` first (post-clone canonical), then
    probes the usual ``master``/``main`` variants. ``repo_root`` selects the
    git directory; ``None`` uses the caller's CWD.
    """
    git_prefix: list[str] = ["git"]
    if repo_root is not None:
        git_prefix.extend(["-C", str(repo_root)])

    head = subprocess.run(
        [*git_prefix, "symbolic-ref", "refs/remotes/origin/HEAD", "--short"],
        capture_output=True, text=True, check=False,
    )
    if head.returncode == 0:
        return head.stdout.strip().removeprefix("origin/")

    for ref in _FALLBACK_REFS:
        probe = subprocess.run(
            [*git_prefix, "rev-parse", "--verify", ref],
            capture_output=True, check=False,
        )
        if probe.returncode == 0:
            return ref
    return None
