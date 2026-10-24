#!/usr/bin/env python3
"""List and optionally delete GitHub Actions caches via the built-in `gh cache` commands.

With --summary, appends a Cache cleanup section (pool size, candidates, deletion
totals or a dry-run notice, and a collapsed cache table) to GITHUB_STEP_SUMMARY.

In cache-cleanup.yml the branch and flags follow the triggering event (GITHUB_EVENT_NAME and the
GITHUB_EVENT_PATH payload): a schedule prunes and deletes with a summary; a closed pull request
deletes only its merge ref's caches, without a summary; a dispatch takes its branch, dry-run and
mode inputs. CLEANUP_BRANCH, CLEANUP_DELETE, CLEANUP_SUMMARY and CLEANUP_PRUNE override them.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_count
from common.gh_actions import (
    env_default,
    env_flag,
    gh,
    gh_warning,
    parse_bool,
    require_repository,
    write_step_summary,
)
from common.io import read_json
from common.markdown import code_span, md_table, more_items, summary_section

DEFAULT_PROTECT = (
    r"^(apt-debs|ccache|cpm-sources-v2|gst-sdk-v1|moccache|qt"
    r"|prek-v1|prek-renovate-v1|lychee-v1)-"
)
_ROLLING_SUFFIX_RE = re.compile(r"-\d+-\d+$")
_PR_BUILD_CACHE_RE = re.compile(r"^(ccache|moccache)-.*-(\d+)-(\d+)$")
_DIGEST_RE = re.compile(r"(?<=-)[0-9a-f]{64}(?=-|$)")
_APT_GENERATION_RE = re.compile(r"^(apt-debs-.+)-\d{4}-\d{2}-<digest>$")
_MIB = 1024 * 1024


@dataclass(frozen=True)
class CleanupDefaults:
    branch: str = ""
    delete: bool = False
    summary: bool = False
    prune: bool = False
    default_branch: str = "master"


# cache-cleanup.yml's workflow_dispatch input defaults, for dispatches that omit an input.
DISPATCH_INPUT_DEFAULTS = {"branch": "", "dry-run": True, "mode": "prune"}


def event_defaults(event_name: str, event: dict[str, Any]) -> CleanupDefaults:
    """Return the cleanup settings for a cache-cleanup.yml trigger and its event payload.

    Other events, and runs outside GitHub Actions, list caches without deleting them.
    """
    default_branch = (event.get("repository") or {}).get("default_branch") or "master"
    if event_name == "schedule":
        return CleanupDefaults(delete=True, summary=True, prune=True, default_branch=default_branch)
    if event_name == "pull_request_target":
        number = int(event["pull_request"]["number"])
        return CleanupDefaults(
            branch=f"refs/pull/{number}/merge", delete=True, default_branch=default_branch
        )
    if event_name == "workflow_dispatch":
        inputs = {**DISPATCH_INPUT_DEFAULTS, **(event.get("inputs") or {})}
        return CleanupDefaults(
            branch=str(inputs["branch"] or ""),
            delete=not parse_bool(str(inputs["dry-run"])),
            summary=True,
            prune=inputs["mode"] == "prune",
            default_branch=default_branch,
        )
    return CleanupDefaults(default_branch=default_branch)


def _event_payload() -> dict[str, Any]:
    path = os.environ.get("GITHUB_EVENT_PATH")
    return read_json(Path(path)) if path else {}


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

    return {max(group, key=lambda cache: cache.last_accessed) for group in families.values()}


def _cache_family(key: str) -> str:
    family = _ROLLING_SUFFIX_RE.sub("", key)
    family = _DIGEST_RE.sub("<digest>", family)
    return _APT_GENERATION_RE.sub(r"\1-<generation>", family)


_LISTED_CACHES = 100


def _mib(size_bytes: int) -> str:
    return f"{format_count(size_bytes // _MIB)} MiB"


def _cache_table(caches: list[CacheUsage]) -> str:
    table = md_table(
        ["Key", "Size", "Branch"],
        [(code_span(c.key[:80]), _mib(c.size_bytes), c.ref) for c in caches[:_LISTED_CACHES]],
        markdown=True,
    )
    if len(caches) > _LISTED_CACHES:
        table += f"\n\n{more_items(len(caches) - _LISTED_CACHES)}"
    return table


def _deletion_facts(deleted: int | None, failed: int) -> list[tuple[str, object]]:
    return [("Deleted", deleted), ("Failed deletions", failed or None)]


def _prune_summary(
    victims: list[CacheUsage],
    total: int,
    projected: int,
    *,
    deleted: int | None,
    failed: int,
) -> str:
    """Cache cleanup section for --prune; *deleted* is None for a dry run."""
    body = "No eviction candidates." if not victims else ""
    if victims and deleted is None:
        body = _DRY_RUN_NOTICE
    return summary_section(
        "Cache cleanup",
        [
            ("Mode", "prune" if deleted is not None else "prune (dry run)"),
            ("Pool", f"{_mib(total)} → {_mib(projected)}"),
            ("Eviction candidates", len(victims)),
            *_deletion_facts(deleted, failed),
        ],
        body=body,
        collapsed=[
            (f"Eviction candidates ({len(victims)})", _cache_table(victims) if victims else "")
        ],
    )


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
        write_step_summary(
            _prune_summary(
                victims, total, projected, deleted=deleted if args.delete else None, failed=failed
            )
        )


def _list_summary(rows: list[CacheUsage], branch: str, *, deleted: int | None, failed: int) -> str:
    """Cache cleanup section for listing (and deleting) one branch's caches."""
    body = "No caches found." if not rows else ""
    if rows and deleted is None:
        body = _DRY_RUN_NOTICE
    return summary_section(
        "Cache cleanup",
        [
            ("Mode", "delete" if deleted is not None else "list (dry run)"),
            ("Branch", code_span(branch) if branch else "all"),
            ("Caches", len(rows)),
            ("Total size", _mib(sum(row.size_bytes for row in rows))),
            *_deletion_facts(deleted, failed),
        ],
        body=body,
        collapsed=[(f"Caches ({len(rows)})", _cache_table(rows) if rows else "")],
    )


_DRY_RUN_NOTICE = "**Dry run**: no caches were deleted. Set `dry-run: false` to delete."


def main(argv: list[str] | None = None) -> int:
    defaults = event_defaults(os.environ.get("GITHUB_EVENT_NAME", ""), _event_payload())
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--branch",
        default=env_default("CLEANUP_BRANCH", defaults.branch),
        help="Branch filter (default: $CLEANUP_BRANCH, else the event's; empty = all)",
    )
    parser.add_argument(
        "--delete",
        action="store_true",
        default=env_flag("CLEANUP_DELETE", defaults.delete),
        help="Delete listed caches (default: $CLEANUP_DELETE, else the event's, else list only)",
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        default=env_flag("CLEANUP_SUMMARY", defaults.summary),
        help="Append markdown summary to $GITHUB_STEP_SUMMARY "
        "(default: $CLEANUP_SUMMARY, else the event's)",
    )
    parser.add_argument("--limit", type=int, default=500, help="Maximum cache entries to inspect")
    parser.add_argument(
        "--prune",
        action="store_true",
        default=env_flag("CLEANUP_PRUNE", defaults.prune),
        help="GC mode: evict stale and non-protected caches above --high-water-mb "
        "(default: $CLEANUP_PRUNE, else the event's)",
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
    parser.add_argument(
        "--default-branch", default=env_default("DEFAULT_BRANCH", defaults.default_branch)
    )
    args = parser.parse_args(argv)

    repo = require_repository()

    if args.prune:
        run_prune(repo, args)
        return 0

    rows = list_caches(repo, args.branch, args.limit)
    count = len(rows)
    print(f"Found {count} cache(s)")

    deleted: int | None = None
    failed = 0
    if args.delete:
        deleted = 0
        if count > 0:
            deleted, failed = delete_caches(repo, args.branch, [r.key for r in rows])
            print(f"Deleted {deleted} cache(s); {failed} failed")
    if args.summary:
        write_step_summary(_list_summary(rows, args.branch, deleted=deleted, failed=failed))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
