#!/usr/bin/env python3
"""Refuse to draft a release for a commit that is not on a ``Stable*`` branch.

release.yml drafts a GitHub Release for a pushed ``vX.Y.Z`` tag, but the platform workflows build
only release tags reachable from an ``origin/Stable*`` branch (``qgc_tools.release_tag_check``), so
a release for a tag anywhere else would have no build artifacts. Requires full history and remote
branch refs.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error
from qgc_tools.release_tag_check import GitQueryError, stable_branches_containing


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ref", default="HEAD", help="Release tag or commit to check")
    args = parser.parse_args(argv)
    try:
        branches = stable_branches_containing(args.ref)
    except GitQueryError as exc:
        gh_error(f"Cannot determine whether {args.ref} is on a Stable* branch: {exc}")
        return 1
    if not branches:
        gh_error(
            f"Releases must be cut from a commit on a Stable* branch, but {args.ref} is not on any "
            "origin/Stable* branch. The platform workflows build only release tags reachable from "
            "a Stable* branch, so its release would have no builds."
        )
        return 1
    print(f"{args.ref} is on Stable branch(es): {', '.join(branches)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
