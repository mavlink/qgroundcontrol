#!/usr/bin/env python3
"""Resolve the cache save policy for the current workflow event.

Thin CLI wrapper around `tools.common.gh_actions.resolve_cache_policy`.
Writes the resolved policy ("true"/"false"/"none") to $GITHUB_OUTPUT under
`save=...` and prints it to stdout.

Also writes `scoped_save=...` for the large per-branch caches (compiler, moc and
CPM sources). Pull requests on GitHub storage only restore the default branch's
copies: their own would cost upload time and, for CPM, evict shared entries from
GitHub's size-limited cache. S3-backed runners keep saving per-PR copies.

Reads EVENT_NAME, PR_REPO, THIS_REPO and QGC_ACTIONS_CACHE_BACKEND from env.
"""

from __future__ import annotations

import argparse
import os

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import resolve_cache_policy, write_github_output


def resolve_scoped_save(requested: str, save: str) -> str:
    if requested != "auto" or save != "true":
        return save
    event = os.environ.get("EVENT_NAME", os.environ.get("GITHUB_EVENT_NAME", ""))
    backend = os.environ.get("QGC_ACTIONS_CACHE_BACKEND", "github")
    return "false" if (event == "pull_request" and backend != "s3") else save


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--requested",
        required=True,
        help="Requested policy: auto|true|false",
    )
    args = parser.parse_args(argv)

    save = resolve_cache_policy(args.requested)
    print(save)
    write_github_output({"save": save, "scoped_save": resolve_scoped_save(args.requested, save)})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
