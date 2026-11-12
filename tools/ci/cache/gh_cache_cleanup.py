#!/usr/bin/env python3
"""List and optionally delete GitHub Actions caches and superseded compiler-cache snapshots.

Caches go through the built-in `gh cache` commands. With --summary, appends a Cache cleanup
section (pool size, candidates, deletion totals or a dry-run notice, and a collapsed cache
table) to GITHUB_STEP_SUMMARY.

A prune also removes the compiler-cache snapshot artifacts that newer ones superseded: it keeps
the newest --keep-snapshots of each artifact name and restore group (the workflow, branch,
repository and event class a restore requires of a snapshot's producer) and adds a
Compiler-cache snapshot cleanup section. It lists everything before deleting anything, and
exits with an error, deleting nothing, when a listing fails or is incomplete.

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
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import UTC, datetime, timedelta
from pathlib import Path
from typing import TYPE_CHECKING, Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_count
from common.gh_actions import (
    env_default,
    env_flag,
    gh,
    gh_api_json,
    gh_warning,
    parse_bool,
    parse_github_timestamp,
    require_repository,
    write_step_summary,
)
from common.io import read_json
from common.markdown import code_span, md_table, more_items, summary_section

from ci.cache.build_cache_artifact import is_snapshot_name, restore_group

if TYPE_CHECKING:
    from collections.abc import Mapping

DEFAULT_PROTECT = (
    r"^(apt-debs|ccache|cpm-sources-v3|gst-sdk-v1|moccache|qt"
    r"|prek-v1|prek-renovate-v1|lychee-v1)-"
)
_ROLLING_SUFFIX_RE = re.compile(r"-\d+-\d+$")
_PR_BUILD_CACHE_RE = re.compile(r"^(ccache|moccache)-.*-(\d+)-(\d+)$")
_DIGEST_RE = re.compile(r"(?<=-)[0-9a-f]{64}(?=-|$)")
_APT_GENERATION_RE = re.compile(r"^(apt-debs-.+)-\d{4}-\d{2}-<digest>$")
_MIB = 1024 * 1024
# A restore reads only the newest snapshot of its restore group. The one before it serves a
# re-run of the newest's own producer, a restore that began before the newest was saved, and the
# pull requests after a bad newest snapshot is deleted by hand; older ones serve nothing.
KEEP_SNAPSHOTS = 2
# Only a name saved since the last cleanup can have a superseded snapshot. Two days of the
# artifact list span eight six-hourly cleanups; what a longer gap leaves expires with its retention.
SNAPSHOT_LOOKBACK_HOURS = 48
_ARTIFACTS_PER_PAGE = 100
_MAX_DISCOVERY_PAGES = 300
_LISTING_ATTEMPTS = 4


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


class ArtifactListingError(RuntimeError):
    """The artifact list is unreadable or incomplete, so no snapshot may be deleted."""


@dataclass(frozen=True)
class Snapshot:
    artifact_id: int
    name: str
    size_bytes: int
    created_at: str
    run_id: int
    branch: str


def _artifact_page(repo: str, page: int, name: str = "") -> tuple[list[dict[str, Any]], int]:
    """Return one page of the repository's artifacts, newest first, and the API's total count.

    The unfiltered list of a large repository answers some requests with an empty HTTP 500,
    which gh reports as a JSON error that the shared transient-error check does not know, so
    this retries every failure of the read-only request.
    """
    args = ["api", "--method", "GET", f"repos/{repo}/actions/artifacts"]
    args += ["-f", f"per_page={_ARTIFACTS_PER_PAGE}", "-f", f"page={page}"]
    if name:
        args += ["-f", f"name={name}"]
    detail = ""
    for attempt in range(_LISTING_ATTEMPTS):
        if attempt:
            time.sleep(2 * attempt)
        try:
            result = gh(*args, check=False)
        except subprocess.TimeoutExpired:
            detail = "timed out"
            continue
        if result.returncode != 0:
            detail = (result.stderr or result.stdout).strip()
            continue
        try:
            payload = json.loads(result.stdout)
            artifacts, total = payload["artifacts"], payload["total_count"]
        except (ValueError, KeyError, TypeError) as error:
            detail = f"malformed response: {error}"
            continue
        if (
            isinstance(artifacts, list)
            and all(isinstance(artifact, dict) for artifact in artifacts)
            and isinstance(total, int)
        ):
            return artifacts, total
        detail = "malformed response"
    listed = f"artifacts named {name}" if name else "artifacts"
    raise ArtifactListingError(f"cannot list {listed} (page {page}): {detail}")


def recent_snapshot_names(repo: str, since: datetime) -> list[str]:
    """Return the snapshot names among the artifacts created since *since*."""
    names: set[str] = set()
    for page in range(1, _MAX_DISCOVERY_PAGES + 1):
        artifacts, _ = _artifact_page(repo, page)
        names.update(a["name"] for a in artifacts if is_snapshot_name(a.get("name")))
        created = [parse_github_timestamp(artifact.get("created_at")) for artifact in artifacts]
        if len(artifacts) < _ARTIFACTS_PER_PAGE or all(
            moment is not None and moment < since for moment in created
        ):
            return sorted(names)
    raise ArtifactListingError(
        f"the artifact list did not reach {since:%Y-%m-%d %H:%M} UTC "
        f"within {_MAX_DISCOVERY_PAGES} pages"
    )


def list_snapshots(repo: str, name: str) -> list[Snapshot]:
    """Return every unexpired snapshot named *name*; raise unless the list is complete."""
    listed: list[dict[str, Any]] = []
    page = 1
    while True:
        artifacts, total = _artifact_page(repo, page, name)
        listed += artifacts
        if len(artifacts) < _ARTIFACTS_PER_PAGE:
            break
        page += 1
    if len(listed) != total:
        raise ArtifactListingError(f"listed {len(listed)} of {total} artifacts named {name}")
    try:
        return [
            Snapshot(
                artifact_id=int(artifact["id"]),
                name=name,
                size_bytes=int(artifact.get("size_in_bytes") or 0),
                created_at=str(artifact.get("created_at") or ""),
                run_id=int(artifact["workflow_run"]["id"]),
                branch=str(artifact["workflow_run"].get("head_branch") or ""),
            )
            for artifact in listed
            if artifact.get("name") == name and not artifact.get("expired")
        ]
    except (KeyError, TypeError, ValueError) as error:
        raise ArtifactListingError(f"malformed artifact named {name}: {error!r}") from error


def snapshot_restore_groups(repo: str, snapshots: list[Snapshot]) -> dict[int, tuple[Any, ...]]:
    """Return the restore group of each producing run, from one lookup per run."""
    return {
        run_id: restore_group(gh_api_json(f"repos/{repo}/actions/runs/{run_id}"))
        for run_id in sorted({snapshot.run_id for snapshot in snapshots})
    }


def select_superseded_snapshots(
    snapshots: list[Snapshot], groups: Mapping[int, tuple[Any, ...]], keep: int
) -> list[Snapshot]:
    """Return the snapshots older than the newest *keep* of their name and restore group.

    A restore takes the highest artifact id, so the same order decides which are newest.
    """
    if keep < KEEP_SNAPSHOTS:
        raise ValueError(f"keep at least {KEEP_SNAPSHOTS} snapshots, not {keep}")
    members: dict[tuple[Any, ...], list[Snapshot]] = {}
    for snapshot in snapshots:
        members.setdefault((snapshot.name, groups[snapshot.run_id]), []).append(snapshot)
    superseded: list[Snapshot] = []
    for group in members.values():
        superseded += sorted(group, key=lambda item: item.artifact_id, reverse=True)[keep:]
    return superseded


def delete_snapshots(repo: str, snapshots: list[Snapshot]) -> tuple[list[Snapshot], int]:
    """Delete each snapshot artifact; return (deleted, failed)."""
    deleted: list[Snapshot] = []
    failed = 0
    for snapshot in snapshots:
        result = gh(
            "api",
            "--method",
            "DELETE",
            f"repos/{repo}/actions/artifacts/{snapshot.artifact_id}",
            check=False,
        )
        if result.returncode == 0:
            deleted.append(snapshot)
        else:
            failed += 1
    return deleted, failed


def _size(snapshots: list[Snapshot]) -> int:
    return sum(snapshot.size_bytes for snapshot in snapshots)


def _snapshot_table(snapshots: list[Snapshot]) -> str:
    table = md_table(
        ["Artifact", "Name", "Size", "Created", "Branch", "Run"],
        [
            (
                item.artifact_id,
                code_span(item.name),
                _mib(item.size_bytes),
                item.created_at,
                code_span(item.branch),
                item.run_id,
            )
            for item in snapshots[:_LISTED_CACHES]
        ],
        markdown=True,
    )
    if len(snapshots) > _LISTED_CACHES:
        table += f"\n\n{more_items(len(snapshots) - _LISTED_CACHES)}"
    return table


def _snapshot_summary(
    snapshots: list[Snapshot],
    superseded: list[Snapshot],
    *,
    keep: int,
    deleted: list[Snapshot] | None,
    failed: int,
) -> str:
    """Compiler-cache snapshot cleanup section; *deleted* is None for a dry run."""
    body = "No superseded snapshots." if not superseded else ""
    if superseded and deleted is None:
        body = "**Dry run**: no snapshots were deleted. Set `dry-run: false` to delete."
    names = len({snapshot.name for snapshot in snapshots})
    kept = f"{len(snapshots) - len(superseded)}, {_mib(_size(snapshots) - _size(superseded))}"
    return summary_section(
        "Compiler-cache snapshot cleanup",
        [
            ("Mode", "prune" if deleted is not None else "prune (dry run)"),
            ("Snapshots", f"{len(snapshots)} under {names} names, {_mib(_size(snapshots))}"),
            ("Kept", f"{kept}: the newest {keep} of each name and restore group"),
            ("Superseded", f"{len(superseded)}, {_mib(_size(superseded))}"),
            ("Deleted", None if deleted is None else len(deleted)),
            ("Freed", None if deleted is None else _mib(_size(deleted))),
            ("Failed deletions", failed or None),
        ],
        body=body,
        collapsed=[
            (
                f"Superseded snapshots ({len(superseded)})",
                _snapshot_table(superseded) if superseded else "",
            )
        ],
    )


def run_snapshot_prune(repo: str, args: argparse.Namespace) -> None:
    """Delete the compiler-cache snapshots that newer ones of their restore group superseded.

    Every listing completes before the first deletion: a failed one raises and deletes nothing.
    """
    since = datetime.now(UTC) - timedelta(hours=args.snapshot_lookback_hours)
    snapshots = [
        snapshot
        for name in recent_snapshot_names(repo, since)
        for snapshot in list_snapshots(repo, name)
    ]
    superseded = select_superseded_snapshots(
        snapshots, snapshot_restore_groups(repo, snapshots), args.keep_snapshots
    )
    for snapshot in superseded:
        print(
            f"Superseded snapshot {snapshot.artifact_id} ({_mib(snapshot.size_bytes)}, "
            f"{snapshot.created_at}, {snapshot.branch}): {snapshot.name}"
        )
    deleted: list[Snapshot] | None = None
    failed = 0
    if args.delete:
        deleted, failed = delete_snapshots(repo, superseded)
    print(
        f"Snapshots: {len(snapshots)}; superseded {len(superseded)} "
        f"({_mib(_size(superseded))}); deleted {len(deleted or [])} "
        f"({_mib(_size(deleted or []))} freed); failed {failed}"
    )
    if failed:
        gh_warning(f"Failed to delete {failed} compiler-cache snapshot artifacts")
    if args.summary:
        write_step_summary(
            _snapshot_summary(
                snapshots, superseded, keep=args.keep_snapshots, deleted=deleted, failed=failed
            )
        )


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
    parser.add_argument(
        "--keep-snapshots",
        type=int,
        default=KEEP_SNAPSHOTS,
        help="Prune keeps this many compiler-cache snapshots of each name and restore group "
        f"(default and minimum {KEEP_SNAPSHOTS})",
    )
    parser.add_argument(
        "--snapshot-lookback-hours",
        type=int,
        default=SNAPSHOT_LOOKBACK_HOURS,
        help="Prune the snapshot names saved in this many hours "
        f"(default {SNAPSHOT_LOOKBACK_HOURS})",
    )
    args = parser.parse_args(argv)
    if args.keep_snapshots < KEEP_SNAPSHOTS:
        parser.error(f"--keep-snapshots must be at least {KEEP_SNAPSHOTS}")

    repo = require_repository()

    if args.prune:
        run_prune(repo, args)
        run_snapshot_prune(repo, args)
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
