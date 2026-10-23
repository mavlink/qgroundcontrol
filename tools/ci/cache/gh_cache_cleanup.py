#!/usr/bin/env python3
"""List and optionally delete GitHub Actions caches via the built-in `gh cache` commands.

With --summary, appends a markdown table of caches (plus deletion totals or a
dry-run notice) to GITHUB_STEP_SUMMARY. cache-cleanup.yml sets the branch and
flags through CLEANUP_* environment variables.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    env_default,
    env_flag,
    gh,
    gh_warning,
    require_repository,
    write_step_summary,
)
from common.markdown import md_table

DEFAULT_PROTECT = (
    r"^(apt-debs|ccache|cpm-sources-v2|gst-sdk-v1|moccache|qt|build-baseline-v2"
    r"|prek-v1|prek-renovate-v1|lychee-v1)-"
)
_BASELINE_RE = re.compile(r"^build-baseline-v2-[0-9a-f]{40,64}-(\d+)-(\d+)$")
_ROLLING_SUFFIX_RE = re.compile(r"-\d+-\d+$")
_PR_BUILD_CACHE_RE = re.compile(r"^(ccache|moccache)-.*-(\d+)-(\d+)$")
_DIGEST_RE = re.compile(r"(?<=-)[0-9a-f]{64}(?=-|$)")
_APT_GENERATION_RE = re.compile(r"^(apt-debs-.+)-\d{4}-\d{2}-<digest>$")
_MIB = 1024 * 1024


@dataclass(frozen=True)
class CacheUsage:
    key: str
    ref: str
    size_bytes: int
    last_accessed: str


def _ref_args(branch: str) -> list[str]:
    """Return the `gh cache --ref` filter; bare branch names map to refs/heads/<name>."""
    if not branch:
        return []
    ref = branch if branch.startswith("refs/") else f"refs/heads/{branch}"
    return ["--ref", ref]


def list_caches(repo: str, branch: str, limit: int) -> list[CacheUsage]:
    """Return cached entries, most recently used first, via `gh cache list --json`."""
    result = gh(
        "cache",
        "list",
        "-R",
        repo,
        *_ref_args(branch),
        "--order",
        "desc",
        "--limit",
        str(limit),
        "--json",
        "key,ref,sizeInBytes,lastAccessedAt",
    )
    return [
        CacheUsage(
            key=row["key"],
            ref=row.get("ref", ""),
            size_bytes=int(row.get("sizeInBytes", 0)),
            last_accessed=row.get("lastAccessedAt", ""),
        )
        for row in json.loads(result.stdout or "[]")
        if row.get("key")
    ]


def delete_caches(repo: str, branch: str, keys: list[str]) -> tuple[int, int]:
    """Delete each key; return (deleted, failed)."""
    deleted = failed = 0
    for key in keys:
        if not key:
            continue
        result = gh(
            "cache",
            "delete",
            key,
            "-R",
            repo,
            *_ref_args(branch),
            check=False,
        )
        if result.returncode == 0:
            deleted += 1
        else:
            failed += 1
    return deleted, failed


def select_prune_victims(
    caches: list[CacheUsage],
    *,
    keep_mb: int,
    high_water_mb: int,
    protect: str,
    default_branch: str = "master",
) -> tuple[list[CacheUsage], int, int]:
    """Pick evictable caches to delete; return (victims, total_bytes, projected_bytes).

    Drop superseded PR build generations even below high_water_mb. Above it,
    keep the newest protected default-branch families and evict least recently used first.
    """
    protect_re = re.compile(protect)
    total = sum(cache.size_bytes for cache in caches)
    victims = _superseded_pr_caches(caches)
    projected = total - sum(cache.size_bytes for cache in victims)
    if projected <= high_water_mb * _MIB:
        return victims, total, projected

    keep = keep_mb * _MIB
    protected = _protected_cache_entries(caches, protect_re, default_branch)
    evictable = sorted(
        (cache for cache in caches if cache not in protected and cache not in victims),
        key=lambda cache: (cache.last_accessed, -cache.size_bytes),
    )
    for cache in evictable:
        if projected <= keep:
            break
        victims.append(cache)
        projected -= cache.size_bytes
    return victims, total, projected


def _superseded_pr_caches(caches: list[CacheUsage]) -> list[CacheUsage]:
    families: dict[tuple[str, str], list[CacheUsage]] = {}
    for cache in caches:
        if re.fullmatch(r"refs/pull/\d+/merge", cache.ref) and _PR_BUILD_CACHE_RE.fullmatch(
            cache.key
        ):
            families.setdefault((cache.ref, _cache_family(cache.key)), []).append(cache)

    victims = []
    for group in families.values():
        # Run IDs/attempts identify generations; a restore can touch an older entry.
        newest = max(group, key=lambda cache: tuple(map(int, cache.key.rsplit("-", 2)[1:])))
        victims.extend(cache for cache in group if cache != newest)
    return victims


def _protected_cache_entries(
    caches: list[CacheUsage], protect_re: re.Pattern[str], default_branch: str
) -> set[CacheUsage]:
    families: dict[tuple[str, str], list[CacheUsage]] = {}
    for cache in caches:
        if cache.ref == f"refs/heads/{default_branch}" and protect_re.search(cache.key):
            families.setdefault((cache.ref, _cache_family(cache.key)), []).append(cache)

    return {max(group, key=_retention_order) for group in families.values()}


def _retention_order(cache: CacheUsage) -> tuple[str, int, int]:
    if match := _BASELINE_RE.fullmatch(cache.key):
        # Reading an older PR base must not displace the latest published baseline.
        return "", int(match[1]), int(match[2])
    return cache.last_accessed, 0, 0


def _cache_family(key: str) -> str:
    if _BASELINE_RE.fullmatch(key):
        return "build-baseline-v2"
    family = _ROLLING_SUFFIX_RE.sub("", key)
    family = _DIGEST_RE.sub("<digest>", family)
    return _APT_GENERATION_RE.sub(r"\1-<generation>", family)


def _cache_table(caches: list[CacheUsage]) -> str:
    return md_table(
        ["Key", "Size", "Branch"],
        [(f"`{c.key[:50]}`", f"{c.size_bytes // _MIB} MiB", c.ref) for c in caches],
    )


def _prune_summary(victims: list[CacheUsage], total: int, projected: int, *, deleted: bool) -> str:
    verb = "Deleted" if deleted else "Selected"
    lines = [
        "## Cache GC\n",
        f"\nPool: {total // _MIB} MiB → {projected // _MIB} MiB "
        f"({verb.lower()} {len(victims)} cache(s))\n",
    ]
    if victims:
        lines.append(f"\n{_cache_table(victims)}\n")
    else:
        lines.append("\nNo eviction candidates.\n")
    return "".join(lines)


def run_prune(repo: str, args: argparse.Namespace) -> None:
    """Evict stale cache generations when the pool exceeds the high-water mark."""
    caches = list_caches(repo, "", args.limit)
    victims, total, projected = select_prune_victims(
        caches,
        keep_mb=args.keep_mb,
        high_water_mb=args.high_water_mb,
        protect=args.protect,
        default_branch=args.default_branch,
    )
    deleted = 0
    failed = 0
    if victims and args.delete:
        projected = total
        for cache in victims:
            removed, errors = delete_caches(repo, cache.ref, [cache.key])
            deleted += removed
            failed += errors
            if removed:
                projected -= cache.size_bytes
    print(
        f"Pool {total // _MIB} MiB; {len(victims)} eviction candidate(s); deleted {deleted}; failed {failed}"
    )
    if failed:
        gh_warning(f"Failed to delete {failed} cache entries")
    if total > args.high_water_mb * _MIB and projected > args.keep_mb * _MIB:
        gh_warning(
            f"Cache pool remains above target: {projected // _MIB} MiB; protected entries or deletion failures prevent further cleanup"
        )

    if args.summary:
        summary = _prune_summary(victims, total, projected, deleted=args.delete and failed == 0)
        if args.delete:
            summary += _deletion_summary(deleted, failed)
        if victims and not args.delete:
            summary += _DRY_RUN_NOTICE
        write_step_summary(summary)


def _list_summary(rows: list[CacheUsage], branch: str) -> str:
    lines = ["## Cache Summary\n"]
    if branch:
        lines.append(f"Branch filter: `{branch}`\n")
    if not rows:
        lines.append("\nNo caches found\n")
        return "".join(lines)
    lines.append(f"\n{_cache_table(rows)}\n")
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
    parser.add_argument(
        "--branch",
        default=env_default("CLEANUP_BRANCH"),
        help="Branch filter (default: $CLEANUP_BRANCH; empty = all)",
    )
    parser.add_argument(
        "--delete",
        action="store_true",
        default=env_flag("CLEANUP_DELETE"),
        help="Delete listed caches (default: $CLEANUP_DELETE, else list only)",
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        default=env_flag("CLEANUP_SUMMARY"),
        help="Append markdown summary to $GITHUB_STEP_SUMMARY (default: $CLEANUP_SUMMARY)",
    )
    parser.add_argument("--limit", type=int, default=500, help="Maximum cache entries to inspect")
    parser.add_argument(
        "--prune",
        action="store_true",
        default=env_flag("CLEANUP_PRUNE"),
        help="GC mode: evict stale and non-protected caches above --high-water-mb "
        "(default: $CLEANUP_PRUNE)",
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
        help="Regex of default-branch cache families whose newest entry is retained",
    )
    parser.add_argument("--default-branch", default=env_default("DEFAULT_BRANCH", "master"))
    args = parser.parse_args(argv)

    repo = require_repository()

    if args.prune:
        run_prune(repo, args)
        return 0

    rows = list_caches(repo, args.branch, args.limit)
    count = len(rows)
    print(f"Found {count} cache(s)")

    summary_parts: list[str] = []
    if args.summary:
        summary_parts.append(_list_summary(rows, args.branch))

    if args.delete and count > 0:
        deleted, failed = delete_caches(repo, args.branch, [r.key for r in rows])
        print(f"Deleted {deleted} cache(s); {failed} failed")
        if args.summary:
            summary_parts.append(_deletion_summary(deleted, failed))
    elif args.summary and not args.delete:
        summary_parts.append(_DRY_RUN_NOTICE)

    if summary_parts:
        write_step_summary("".join(summary_parts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
