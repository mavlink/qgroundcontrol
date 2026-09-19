#!/usr/bin/env python3
"""Decide whether the current tag ref is a publishable release tag.

A release tag must match ``vX.Y.Z`` exactly and be reachable from a ``Stable_V*``
branch. Anything else (version markers on master, ``-rc``/``-dev`` suffixes,
manual dispatches on odd refs) must not trigger the release pipeline or overwrite
the public S3 ``latest/`` downloads. The name check matters because
``workflow_dispatch`` on a tag bypasses the workflows' ``push.tags`` glob.
Requires a checkout with full history and remote branch refs.

Emits ``release_tag=true|false`` to ``GITHUB_OUTPUT``.

Usage:
    python3 .github/scripts/release_tag_check.py [--context "skipping build"]
"""

from __future__ import annotations

import argparse
import os
import re

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, gh_warning, write_github_output
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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--context",
        default="",
        help="Appended to the warning when the ref is not a publishable release tag",
    )
    args = parser.parse_args(argv)
    suffix = f"; {args.context}" if args.context else ""
    ref_name = os.environ.get("GITHUB_REF_NAME", "HEAD")

    if not is_release_tag(ref_name):
        gh_warning(f"Tag {ref_name} is not a vX.Y.Z release tag{suffix}")
        write_github_output({"release_tag": "false"})
        return 0

    try:
        branches = stable_branches_containing()
    except GitQueryError as exc:
        gh_error(f"Cannot determine whether tag is on a Stable* branch: {exc}")
        return 1
    if branches:
        print(f"Tag {ref_name} is on Stable branch(es): {', '.join(branches)}")
    else:
        gh_warning(f"Tag {ref_name} is not on a Stable* branch{suffix}")
    write_github_output({"release_tag": "true" if branches else "false"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
