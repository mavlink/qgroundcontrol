"""Git helpers for QGC tooling.

Centralises the ``["git", ...]`` subprocess shelling used by CI scripts and
dev tools. Provides default-branch discovery plus a generic ``run_git``
wrapper backed by :mod:`common.proc`.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from .proc import run_captured

if TYPE_CHECKING:
    import subprocess
    from pathlib import Path

__all__ = ["get_default_branch_ref", "run_git"]

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
