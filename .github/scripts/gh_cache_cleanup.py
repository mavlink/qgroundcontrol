#!/usr/bin/env python3
"""List and optionally delete GitHub Actions caches via gh-actions-cache.

Writes count (and deleted, when --delete) to GITHUB_OUTPUT. With --summary,
appends a markdown table of caches (plus deletion totals or a dry-run notice)
to GITHUB_STEP_SUMMARY.

Requires the `gh-actions-cache` extension to be installed; the caller (action
or workflow) is responsible for installation.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output, write_step_summary  # noqa: E402


@dataclass(frozen=True)
class CacheRow:
    key: str
    size: str
    ref: str


def _repo() -> str:
    repo = os.environ.get("GH_REPO") or os.environ.get("GITHUB_REPOSITORY", "")
    if not repo:
        sys.exit("::error::GH_REPO or GITHUB_REPOSITORY must be set")
    return repo


def _branch_args(branch: str) -> list[str]:
    return ["-B", branch] if branch else []


def list_caches(repo: str, branch: str, limit: int = 100) -> list[CacheRow]:
    """Return cached entries via `gh actions-cache list`. Empty list on no rows."""
    cmd = [
        "gh", "actions-cache", "list",
        "-R", repo,
        *_branch_args(branch),
        "--order", "desc",
        "--limit", str(limit),
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    rows: list[CacheRow] = []
    for line in result.stdout.splitlines():
        if not line.strip():
            continue
        # gh-actions-cache emits tab-separated: KEY \t SIZE \t REF \t LAST_USED
        parts = line.split("\t")
        if len(parts) < 3:
            parts = line.split()
            if len(parts) < 3:
                continue
        rows.append(CacheRow(key=parts[0], size=parts[1], ref=parts[2]))
    return rows


def delete_caches(repo: str, branch: str, keys: list[str]) -> tuple[int, int]:
    """Delete each key; return (deleted, failed)."""
    deleted = failed = 0
    for key in keys:
        if not key:
            continue
        cmd = [
            "gh", "actions-cache", "delete", key,
            "-R", repo,
            *_branch_args(branch),
            "--confirm",
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            deleted += 1
        else:
            failed += 1
    return deleted, failed


def _list_summary(rows: list[CacheRow], branch: str) -> str:
    lines = ["## Cache Summary\n"]
    if branch:
        lines.append(f"Branch filter: `{branch}`\n")
    if not rows:
        lines.append("\nNo caches found\n")
        return "".join(lines)
    lines.append("\n| Key | Size | Branch |\n|-----|------|--------|\n")
    for row in rows:
        lines.append(f"| `{row.key[:50]}` | {row.size} | {row.ref} |\n")
    lines.append(f"\n**Total: {len(rows)} caches**\n")
    return "".join(lines)


def _deletion_summary(deleted: int, failed: int) -> str:
    out = ["\n## Deletion Results\n", f"Deleted: {deleted}\n"]
    if failed:
        out.append(f"Failed: {failed}\n")
    return "".join(out)


_DRY_RUN_NOTICE = (
    "\n> **Dry run** — no caches were deleted. Set `dry-run: false` to delete.\n"
)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--branch", default="", help="Branch filter (empty = all)")
    parser.add_argument(
        "--delete",
        action="store_true",
        help="Delete listed caches (default lists only)",
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        help="Append markdown summary to $GITHUB_STEP_SUMMARY",
    )
    parser.add_argument("--limit", type=int, default=100)
    args = parser.parse_args(argv)

    repo = _repo()
    rows = list_caches(repo, args.branch, args.limit)
    count = len(rows)
    print(f"Found {count} cache(s)")

    outputs = {"count": str(count)}
    summary_parts: list[str] = []
    if args.summary:
        summary_parts.append(_list_summary(rows, args.branch))

    if args.delete and count > 0:
        deleted, failed = delete_caches(repo, args.branch, [r.key for r in rows])
        print(f"Deleted {deleted} cache(s); {failed} failed")
        outputs["deleted"] = str(deleted)
        if args.summary:
            summary_parts.append(_deletion_summary(deleted, failed))
    else:
        outputs["deleted"] = "0"
        if args.summary and not args.delete:
            summary_parts.append(_DRY_RUN_NOTICE)

    write_github_output(outputs)
    if summary_parts:
        write_step_summary("".join(summary_parts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
