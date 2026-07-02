#!/usr/bin/env python3
"""Read and prune size/* labels on a pull request.

Subcommands:
    current   Write the current size/* label name to $GITHUB_OUTPUT (key: label).
              Empty string if none. Multiple labels → returns the alphabetically
              first one (label-pruning runs after labeling, so transient
              multi-label states are normal).
    prune     When more than one size/* label is set, remove a stale one
              (defaults to $OLD_LABEL) so the freshly applied label stands alone.

The third-party labeler (codelytv/pr-size-labeler) only adds labels; it
doesn't remove obsolete ones, so we own the cleanup.
"""

from __future__ import annotations

import argparse
import os
import sys
from urllib.parse import quote

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh, gh_error, gh_warning, write_github_output

SIZE_PREFIX = "size/"


def _repo() -> str:
    repo = os.environ.get("GH_REPO") or os.environ.get("GITHUB_REPOSITORY", "")
    if not repo:
        gh_error("GH_REPO or GITHUB_REPOSITORY must be set")
        sys.exit(1)
    return repo


def _pr_number(arg: str | None) -> str:
    pr = arg or os.environ.get("PR_NUMBER", "")
    if not pr:
        gh_error("--pr-number or PR_NUMBER must be set")
        sys.exit(1)
    return str(pr)


def list_size_labels(repo: str, pr: str) -> list[str]:
    """Return the sorted list of size/* labels currently on the PR."""
    result = gh(
        "api",
        f"repos/{repo}/issues/{pr}/labels",
        "--jq",
        '.[] | select(.name | startswith("size/")) | .name',
        check=False,
    )
    if result.returncode != 0:
        gh_error(f"gh api failed ({result.returncode}): {result.stderr.strip()}")
        sys.exit(1)
    return sorted(line for line in result.stdout.splitlines() if line.startswith(SIZE_PREFIX))


def remove_label(repo: str, pr: str, label: str) -> bool:
    """Delete a single label from the PR. Returns True on success or 404 (already gone)."""
    encoded = quote(label, safe="")
    result = gh(
        "api",
        f"repos/{repo}/issues/{pr}/labels/{encoded}",
        "-X",
        "DELETE",
        check=False,
    )
    if result.returncode == 0:
        return True
    # 404 means the label was already removed (e.g. concurrent run); treat as success.
    if "Not Found" in result.stderr or "404" in result.stderr:
        return True
    gh_warning(f"failed to remove label {label!r}: {result.stderr.strip()}")
    return False


def cmd_current(args: argparse.Namespace) -> int:
    """Emit the first size/* label (or empty string) to $GITHUB_OUTPUT."""
    labels = list_size_labels(_repo(), _pr_number(args.pr_number))
    write_github_output({"label": labels[0] if labels else ""})
    print(f"label={labels[0] if labels else ''}")
    return 0


def cmd_prune(args: argparse.Namespace) -> int:
    """Remove stale size/* labels when more than one is present."""
    repo = _repo()
    pr = _pr_number(args.pr_number)
    labels = list_size_labels(repo, pr)

    if len(labels) <= 1:
        print(f"size labels: {labels} — nothing to prune")
        return 0

    old_label = args.old_label or os.environ.get("OLD_LABEL", "")
    to_remove = [old_label] if old_label and old_label in labels else list(labels[1:])
    for label in to_remove:
        print(f"Removing stale size label: {label}")
        remove_label(repo, pr, label)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    current = subparsers.add_parser("current", help="Write current size/* label to GITHUB_OUTPUT")
    current.add_argument("--pr-number", help="PR number (default: $PR_NUMBER)")
    current.set_defaults(func=cmd_current)

    prune = subparsers.add_parser("prune", help="Remove stale size/* labels")
    prune.add_argument("--pr-number", help="PR number (default: $PR_NUMBER)")
    prune.add_argument("--old-label", help="Specific label to drop (default: $OLD_LABEL)")
    prune.set_defaults(func=cmd_prune)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
