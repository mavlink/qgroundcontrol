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
import json
import os
import re
import sys
from dataclasses import dataclass

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh, gh_error, write_github_output, write_step_summary
from common.markdown import md_table

# Build caches are the expensive-to-rebuild data the GC must never evict; the
# 10 GiB/repo pool is reclaimed from everything else first.
DEFAULT_PROTECT = r"^(ccache|cpm-modules)-"
_MIB = 1024 * 1024


@dataclass(frozen=True)
class CacheRow:
    key: str
    size: str
    ref: str


@dataclass(frozen=True)
class CacheUsage:
    key: str
    ref: str
    size_bytes: int
    last_accessed: str


def _repo() -> str:
    repo = os.environ.get("GH_REPO") or os.environ.get("GITHUB_REPOSITORY", "")
    if not repo:
        gh_error("GH_REPO or GITHUB_REPOSITORY must be set")
        sys.exit(1)
    return repo


def _branch_args(branch: str) -> list[str]:
    return ["-B", branch] if branch else []


def list_caches(repo: str, branch: str, limit: int = 100) -> list[CacheRow]:
    """Return cached entries via `gh actions-cache list`. Empty list on no rows."""
    result = gh(
        "actions-cache",
        "list",
        "-R",
        repo,
        *_branch_args(branch),
        "--order",
        "desc",
        "--limit",
        str(limit),
    )
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
        result = gh(
            "actions-cache",
            "delete",
            key,
            "-R",
            repo,
            *_branch_args(branch),
            "--confirm",
            check=False,
        )
        if result.returncode == 0:
            deleted += 1
        else:
            failed += 1
    return deleted, failed


def list_caches_usage(repo: str, limit: int = 200) -> list[CacheUsage]:
    """Return all caches with numeric sizes via built-in `gh cache list --json`."""
    result = gh(
        "cache",
        "list",
        "-R",
        repo,
        "--limit",
        str(limit),
        "--json",
        "key,ref,sizeInBytes,lastAccessedAt",
    )
    data = json.loads(result.stdout or "[]")
    return [
        CacheUsage(
            key=row.get("key", ""),
            ref=row.get("ref", ""),
            size_bytes=int(row.get("sizeInBytes", 0)),
            last_accessed=row.get("lastAccessedAt", ""),
        )
        for row in data
        if row.get("key")
    ]


def select_prune_victims(
    caches: list[CacheUsage], *, keep_mb: int, high_water_mb: int, protect: str
) -> tuple[list[CacheUsage], int, int]:
    """Pick evictable caches to delete; return (victims, total_bytes, projected_bytes).

    No-op below high_water_mb. Above it, evicts non-protected caches largest-first
    (cold-first on ties) until the pool would drop to keep_mb. Protected build
    caches are never selected, so the floor can exceed keep_mb.
    """
    protect_re = re.compile(protect)
    total = sum(cache.size_bytes for cache in caches)
    if total <= high_water_mb * _MIB:
        return [], total, total

    keep = keep_mb * _MIB
    evictable = sorted(
        (cache for cache in caches if not protect_re.search(cache.key)),
        key=lambda cache: (-cache.size_bytes, cache.last_accessed),
    )
    victims: list[CacheUsage] = []
    projected = total
    for cache in evictable:
        if projected <= keep:
            break
        victims.append(cache)
        projected -= cache.size_bytes
    return victims, total, projected


def _prune_summary(victims: list[CacheUsage], total: int, projected: int, *, deleted: bool) -> str:
    verb = "Deleted" if deleted else "Would delete"
    lines = [
        "## Cache GC\n",
        f"\nPool: {total // _MIB} MiB → {projected // _MIB} MiB "
        f"({verb.lower()} {len(victims)} cache(s))\n",
    ]
    if victims:
        table = md_table(
            ["Key", "Size", "Branch"],
            [(f"`{c.key[:50]}`", f"{c.size_bytes // _MIB} MiB", c.ref) for c in victims],
        )
        lines.append(f"\n{table}\n")
    else:
        lines.append("\nUnder high-water mark — nothing to evict.\n")
    return "".join(lines)


def run_prune(repo: str, args: argparse.Namespace) -> dict[str, str]:
    """Evict non-protected caches when the pool exceeds the high-water mark."""
    caches = list_caches_usage(repo, args.limit)
    victims, total, projected = select_prune_victims(
        caches, keep_mb=args.keep_mb, high_water_mb=args.high_water_mb, protect=args.protect
    )
    deleted = 0
    if victims and args.delete:
        deleted, _ = delete_caches(repo, "", [cache.key for cache in victims])
    print(f"Pool {total // _MIB} MiB; {len(victims)} eviction candidate(s); deleted {deleted}")

    if args.summary:
        summary = _prune_summary(victims, total, projected, deleted=args.delete)
        if victims and not args.delete:
            summary += _DRY_RUN_NOTICE
        write_step_summary(summary)
    return {"count": str(len(caches)), "deleted": str(deleted)}


def _list_summary(rows: list[CacheRow], branch: str) -> str:
    lines = ["## Cache Summary\n"]
    if branch:
        lines.append(f"Branch filter: `{branch}`\n")
    if not rows:
        lines.append("\nNo caches found\n")
        return "".join(lines)
    table = md_table(
        ["Key", "Size", "Branch"],
        [(f"`{row.key[:50]}`", row.size, row.ref) for row in rows],
    )
    lines.append(f"\n{table}\n")
    lines.append(f"\n**Total: {len(rows)} caches**\n")
    return "".join(lines)


def _deletion_summary(deleted: int, failed: int) -> str:
    out = ["\n## Deletion Results\n", f"Deleted: {deleted}\n"]
    if failed:
        out.append(f"Failed: {failed}\n")
    return "".join(out)


_DRY_RUN_NOTICE = "\n> **Dry run** — no caches were deleted. Set `dry-run: false` to delete.\n"


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
    parser.add_argument(
        "--prune",
        action="store_true",
        help="GC mode: evict non-protected caches when the pool exceeds --high-water-mb",
    )
    parser.add_argument(
        "--high-water-mb",
        type=int,
        default=9000,
        help="Prune only when total cache size exceeds this (default 9000, cap is 10240)",
    )
    parser.add_argument(
        "--keep-mb",
        type=int,
        default=6500,
        help="Prune target: evict down to roughly this size, leaving headroom (default 6500)",
    )
    parser.add_argument(
        "--protect",
        default=DEFAULT_PROTECT,
        help="Regex of cache keys never evicted in --prune mode",
    )
    args = parser.parse_args(argv)

    repo = _repo()

    if args.prune:
        write_github_output(run_prune(repo, args))
        return 0

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
