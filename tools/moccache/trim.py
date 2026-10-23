"""LRU size limit for the moccache directory: --trim, and the auto-trim after misses."""

from __future__ import annotations

import contextlib
import os
import shutil
import sys
import time
from typing import TYPE_CHECKING

from .core import _cache_dir, _log_stat

if TYPE_CHECKING:
    from pathlib import Path

_SIZE_SUFFIXES = {"k": 1024, "m": 1024**2, "g": 1024**3, "t": 1024**4}
_TRIM_INTERVAL_SECONDS = 3600
_TRIM_LOW_WATER = 0.8  # trim to 80% of the limit so trims aren't back-to-back


def parse_size(s: str) -> int:
    """Parse a size like '256M', '1G', '2kb', or plain bytes into bytes."""
    t = s.strip().lower().removesuffix("b")
    if not t:
        raise ValueError(f"invalid size: {s!r}")
    factor = 1
    if t[-1] in _SIZE_SUFFIXES:
        factor = _SIZE_SUFFIXES[t[-1]]
        t = t[:-1]
    if not t.isdigit():
        raise ValueError(f"invalid size: {s!r}")
    return int(t) * factor


def scan_entries(cache_dir: Path) -> list[tuple[float, int, Path]]:
    """Every cache entry as (mtime, size in bytes, path); vanished entries are skipped."""
    entries = []
    for shard in cache_dir.glob("??"):
        if not shard.is_dir():
            continue
        for mdir in shard.iterdir():
            with contextlib.suppress(OSError):
                mtime = mdir.stat().st_mtime
                size = sum(f.stat().st_size for f in mdir.iterdir() if f.is_file())
                entries.append((mtime, size, mdir))
    return entries


def trim(cache_dir: Path, max_bytes: int) -> int:
    """LRU-evict cache entries until total size <= 80% of max_bytes.

    Entry recency is the entry directory's mtime (refreshed on hits).
    Returns the number of entries removed. Never raises: a concurrently
    written or vanished entry is skipped.
    """
    entries = scan_entries(cache_dir)
    total = sum(size for _, size, _ in entries)
    if total <= max_bytes:
        return 0
    target = int(max_bytes * _TRIM_LOW_WATER)
    removed = 0
    for _, size, mdir in sorted(entries):
        if total <= target:
            break
        shutil.rmtree(mdir, ignore_errors=True)
        if mdir.exists():
            continue  # deletion failed; don't account for it
        total -= size
        removed += 1
    return removed


def maybe_auto_trim(cache_dir: Path) -> None:
    """Trim after a miss if MOCCACHE_MAX_SIZE is set, at most once per hour.

    The trim slot is claimed atomically (O_EXCL lock file) so concurrent
    misses don't all walk the cache; a lock left by a crashed holder is
    reclaimed once it is older than the trim interval.
    """
    limit = os.environ.get("MOCCACHE_MAX_SIZE")
    if not limit:
        return
    try:
        max_bytes = parse_size(limit)
    except ValueError:
        _log_stat(
            cache_dir, "bad-max-size", "", limit
        )  # misconfig breadcrumb; cache stays untrimmed
        return
    stamp = cache_dir / "trim.stamp"
    lock = cache_dir / "trim.lock"
    with contextlib.suppress(OSError):
        try:
            fresh = time.time() - stamp.stat().st_mtime < _TRIM_INTERVAL_SECONDS
        except FileNotFoundError:
            fresh = False  # no stamp (or it vanished mid-check): proceed to trim
        if fresh:
            return
        try:
            os.close(os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY))
        except FileExistsError:
            # Another process holds the slot. Reclaim it if the holder
            # crashed long ago; either way, skip this trim.
            if time.time() - lock.stat().st_mtime > _TRIM_INTERVAL_SECONDS:
                lock.unlink()
            return
        try:
            removed = trim(cache_dir, max_bytes)
            if removed:
                _log_stat(cache_dir, "cache-trimmed", "", str(removed))
            stamp.touch()  # only after a completed trim; a crash must not suppress retries
        finally:
            with contextlib.suppress(OSError):
                lock.unlink()


def trim_main(argv: list[str]) -> int:
    max_size = os.environ.get("MOCCACHE_MAX_SIZE", "")
    i = 0
    while i < len(argv):
        if argv[i] == "--max-size":
            if i + 1 >= len(argv):
                print("moccache: --max-size requires a value", file=sys.stderr)
                return 2
            max_size = argv[i + 1]
            i += 2
        else:
            print(f"moccache: unknown --trim argument: {argv[i]}", file=sys.stderr)
            return 2
    if not max_size:
        print("moccache: --trim requires --max-size or MOCCACHE_MAX_SIZE", file=sys.stderr)
        return 2
    try:
        max_bytes = parse_size(max_size)
    except ValueError as e:
        print(f"moccache: {e}", file=sys.stderr)
        return 2
    cache_dir = _cache_dir()
    removed = trim(cache_dir, max_bytes)
    print(f"moccache: trimmed {removed} entries from {cache_dir}")
    return 0
