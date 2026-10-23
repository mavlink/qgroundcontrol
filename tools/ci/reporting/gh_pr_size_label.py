#!/usr/bin/env python3
"""Apply the size/* label for a pull request's changed lines.

Subcommands:
    apply   Count additions + deletions across the PR's files (skipping lock files and
            translations), swap in the matching size/* label, remove stale ones and
            post a one-time notice on size/XL PRs.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from fnmatch import fnmatchcase
from pathlib import Path
from urllib.parse import quote

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh, gh_error, gh_warning, require_repository

SIZE_PREFIX = "size/"
# (label, exclusive upper bound on changed lines)
SIZE_LIMITS = (("size/XS", 150), ("size/S", 500), ("size/M", 1000), ("size/L", 1500))
XL_LABEL = "size/XL"
# fnmatch, like the shell patterns this replaced: `*` also matches `/`.
IGNORED_FILES = ("docs/package-lock.json", "*.lock", "translations/*")
XL_NOTICE_HEADING = "## Large PR Warning"
XL_NOTICE = f"""{XL_NOTICE_HEADING}

This PR has **more than 1500 lines** changed, which can be difficult to review thoroughly.

Consider:
- Splitting into smaller, focused PRs
- Separating refactoring from feature changes
- Breaking out unrelated changes into separate PRs

Smaller PRs get reviewed faster and reduce the risk of bugs slipping through.
"""
NOTICE_AUTHOR = "github-actions[bot]"


def _pr_number(arg: str | None) -> str:
    pr = arg or os.environ.get("PR_NUMBER", "")
    if not pr:
        gh_error("--pr-number or PR_NUMBER must be set")
        sys.exit(1)
    return str(pr)


def _gh_lines(*args: str) -> list[str]:
    """Run a read-only gh api call and return its non-empty output lines, exiting on failure."""
    result = gh("api", "--method", "GET", *args, check=False, retry_transient=True)
    if result.returncode != 0:
        gh_error(f"gh api failed ({result.returncode}): {result.stderr.strip()}")
        sys.exit(1)
    return [line for line in result.stdout.splitlines() if line.strip()]


def list_size_labels(repo: str, pr: str) -> list[str]:
    """Return the sorted list of size/* labels currently on the PR."""
    names = _gh_lines(
        f"repos/{repo}/issues/{pr}/labels",
        "--jq",
        '.[] | select(.name | startswith("size/")) | .name',
    )
    return sorted(name for name in names if name.startswith(SIZE_PREFIX))


def count_changed_lines(repo: str, pr: str) -> int:
    """Sum additions and deletions over the PR's files, skipping IGNORED_FILES."""
    files = _gh_lines(
        "--paginate",
        f"repos/{repo}/pulls/{pr}/files",
        "-F",
        "per_page=100",
        "--jq",
        ".[] | {filename, additions, deletions}",
    )
    total = 0
    for line in files:
        entry = json.loads(line)
        if not any(fnmatchcase(entry["filename"], pattern) for pattern in IGNORED_FILES):
            total += entry["additions"] + entry["deletions"]
    return total


def size_label(changed_lines: int) -> str:
    return next((label for label, limit in SIZE_LIMITS if changed_lines < limit), XL_LABEL)


def add_label(repo: str, pr: str, label: str) -> bool:
    """Add a label to the PR; GitHub creates a missing label with its default color."""
    result = gh(
        "api",
        "--method",
        "POST",
        f"repos/{repo}/issues/{pr}/labels",
        "-f",
        f"labels[]={label}",
        check=False,
    )
    if result.returncode != 0:
        gh_warning(f"failed to add label {label!r}: {result.stderr.strip()}")
    return result.returncode == 0


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


def has_xl_notice(repo: str, pr: str) -> bool:
    """Return whether the workflow already posted the size/XL notice on this PR."""
    notices = _gh_lines(
        "--paginate",
        f"repos/{repo}/issues/{pr}/comments",
        "-F",
        "per_page=100",
        "--jq",
        f'.[] | select(.user.login == "{NOTICE_AUTHOR}" and '
        f'(.body | startswith("{XL_NOTICE_HEADING}"))) | .id',
    )
    return bool(notices)


def post_xl_notice(repo: str, pr: str) -> bool:
    result = gh(
        "api",
        "--method",
        "POST",
        f"repos/{repo}/issues/{pr}/comments",
        "-f",
        f"body={XL_NOTICE}",
        check=False,
    )
    if result.returncode != 0:
        gh_warning(f"failed to post the size/XL notice: {result.stderr.strip()}")
    return result.returncode == 0


def cmd_apply(args: argparse.Namespace) -> int:
    """Label the PR by size, remove stale size labels and post the XL notice once."""
    repo = require_repository()
    pr = _pr_number(args.pr_number)
    current = list_size_labels(repo, pr)
    changed_lines = count_changed_lines(repo, pr)
    label = size_label(changed_lines)
    print(f"Changed lines: {changed_lines} -> {label} (current: {current or 'none'})")

    ok = label in current or add_label(repo, pr, label)
    for stale in current:
        if stale != label:
            print(f"Removing stale size label: {stale}")
            ok = remove_label(repo, pr, stale) and ok
    if label == XL_LABEL and not has_xl_notice(repo, pr):
        ok = post_xl_notice(repo, pr) and ok
    return int(not ok)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    apply = subparsers.add_parser("apply", help="Apply the size/* label for changed lines")
    apply.add_argument("--pr-number", help="PR number (default: $PR_NUMBER)")
    apply.set_defaults(func=cmd_apply)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
