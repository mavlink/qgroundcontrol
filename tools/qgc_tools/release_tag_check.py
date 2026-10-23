"""Decide whether the current tag ref is a publishable release tag.

A release tag must match ``vX.Y.Z`` exactly and be reachable from a ``Stable_V*``
branch. Anything else (version markers on master, ``-rc``/``-dev`` suffixes,
manual dispatches on odd refs) must not trigger the release pipeline or overwrite
the public S3 ``latest/`` downloads. The name check matters because
``workflow_dispatch`` on a tag bypasses the workflows' ``push.tags`` glob.
Requires a checkout with full history and remote branch refs.
"""

from __future__ import annotations

import re

from common.gh_actions import gh_warning
from common.git import run_git

_STABLE_BRANCH_GLOB = "origin/Stable*"
_RELEASE_TAG_RE = re.compile(r"^v\d+\.\d+\.\d+$")


class GitQueryError(RuntimeError):
    """git branch --contains failed; the verdict is unknown, not "false"."""


def is_release_tag(ref_name: str) -> bool:
    return _RELEASE_TAG_RE.fullmatch(ref_name) is not None


def stable_branches_containing(ref: str = "HEAD") -> list[str]:
    result = run_git("branch", "-r", "--contains", ref, "--list", _STABLE_BRANCH_GLOB)
    if result.returncode != 0:
        raise GitQueryError(result.stderr.strip() or f"git exited {result.returncode}")
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def check_release_tag(ref_name: str, context: str = "") -> bool:
    """Return whether *ref_name* is publishable, warning (with *context*) when it is not.

    Raises GitQueryError when git cannot answer.
    """
    suffix = f"; {context}" if context else ""
    if not is_release_tag(ref_name):
        gh_warning(f"Tag {ref_name} is not a vX.Y.Z release tag{suffix}")
        return False

    branches = stable_branches_containing()
    if branches:
        print(f"Tag {ref_name} is on Stable branch(es): {', '.join(branches)}")
    else:
        gh_warning(f"Tag {ref_name} is not on a Stable* branch{suffix}")
    return bool(branches)
