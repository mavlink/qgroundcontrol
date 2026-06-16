#!/usr/bin/env python3
"""Resolve the cache save policy for the current workflow event.

Thin CLI wrapper around `tools.common.gh_actions.resolve_cache_policy`.
Writes the resolved policy ("true"/"false"/"none") to $GITHUB_OUTPUT under
`save=...` and prints it to stdout.

Reads EVENT_NAME, PR_REPO, THIS_REPO from env (set by the caller workflow).
"""

from __future__ import annotations

import argparse

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import resolve_cache_policy, write_github_output


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
    write_github_output({"save": save})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
