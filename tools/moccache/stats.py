"""moccache stats: --show-stats reports the current ninja build; --zero-stats resets the log."""

from __future__ import annotations

import contextlib
import os
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

from .core import _STAT_FIELD_BREAKERS, _atomic_write, _cache_dir
from .trim import parse_size, scan_entries

# Shown next to each kind in --show-stats so the report is self-explanatory.
KIND_DESCRIPTIONS = {
    "miss-first-seen": "first time this header was moc'd",
    "miss-header-changed": "the header itself changed",
    "miss-include-changed": "a header it includes changed",
    "miss-predefs-changed": "moc_predefs.h changed: compiler flags or SDK differ",
    "miss-moc-version-changed": "moc version changed",
    "miss-args-changed": "moc arguments differ: build trees not sharing, investigate",
    "miss-output-depth-differs": "autogen output dir depth differs between build trees",
    "miss-cache-evicted": "entry was trimmed from the cache: raise MOCCACHE_MAX_SIZE",
    "miss-cache-unreadable": "entry exists but is incomplete or could not be copied out",
    "miss-history-evicted": "all previous entries trimmed, cannot diff: raise MOCCACHE_MAX_SIZE",
    "miss-uncacheable": "moc produced no dep file, result not cached",
    "cache-trimmed": "auto-trim ran, detail = entries evicted",
    "bad-max-size": "MOCCACHE_MAX_SIZE is not a valid size, detail = the value",
}


# Subtracted from the estimated build start so clock/mtime granularity never
# drops a moc run that happened in the build's first moments.
_BUILD_START_SLACK_SECONDS = 2.0

# ninja on Windows logs mtimes as FILETIME ticks (100 ns since 1601-01-01)
# minus its own 12622770400 s rebase (src/disk_interface.cc), which is *not*
# exactly 400 years; the Unix offset below is that constant minus 1601->1970.
_NINJA_LOG_MTIME_IS_FILETIME = sys.platform == "win32"
_NINJA_FILETIME_UNIX_OFFSET = 12_622_770_400 - 11_644_473_600

# A build start further back than this (or in the future) means the log isn't
# telling us about the current build; report all stats rather than misfilter.
_BUILD_START_MAX_AGE_SECONDS = 24 * 3600
_BUILD_START_MAX_FUTURE_SECONDS = 60

# Edges examined for the build start: enough to always include a real compile
# alongside any restat command that logged a stale output mtime.
_BUILD_START_EDGES = 32


def ninja_mtime_to_epoch(mtime: int) -> float:
    if _NINJA_LOG_MTIME_IS_FILETIME:
        return mtime / 1e7 + _NINJA_FILETIME_UNIX_OFFSET
    return mtime / 1e9


def build_start(build_dir: Path) -> float | None:
    """Approximate start of the ninja build running in build_dir, or None.

    .ninja_log records each finished edge as "start_ms end_ms mtime output
    hash" with the times relative to the build start and mtime absolute, so
    a recent edge gives start ~= mtime - end. Restat edges whose output was
    left untouched log a stale mtime that only ever makes that estimate too
    early, so the max over the last few edges is taken. (.ninja_lock's mtime
    is unusable: ninja rewrites it as edges complete.)
    """
    # Whole-file read is fine: ninja recompacts .ninja_log itself, keeping it
    # to a few records per edge (well under a few MB even for large trees).
    try:
        lines = (
            (build_dir / ".ninja_log").read_text(encoding="utf-8", errors="replace").splitlines()
        )
    except OSError:
        return None
    start: float | None = None
    examined = 0
    for line in reversed(lines):
        if examined >= _BUILD_START_EDGES:
            break
        fields = line.split("\t")
        if line.startswith("#") or len(fields) < 4:
            continue
        try:
            end_ms = int(fields[1])
            mtime = int(fields[2])
        except ValueError:
            continue
        if mtime <= 0:
            continue
        examined += 1
        candidate = ninja_mtime_to_epoch(mtime) - end_ms / 1000.0
        if start is None or candidate > start:
            start = candidate
    if start is None:
        return None
    now = time.time()
    if start > now + _BUILD_START_MAX_FUTURE_SECONDS or start < now - _BUILD_START_MAX_AGE_SECONDS:
        return None
    return start - _BUILD_START_SLACK_SECONDS


def stat_time(fields: list[str]) -> float | None:
    """Epoch time of a stats line, or None for legacy lines without one."""
    if len(fields) < 4:
        return None
    try:
        return float(fields[2])
    except ValueError:
        return None


# Growth cap for stats.log when no build boundary is available to prune at;
# comfortably more than one full QGC build's worth of moc runs.
_STATS_LOG_MAX_LINES = 50_000


def format_bytes(n: int) -> str:
    return f"{n / 1024**3:.1f} GiB" if n >= 1024**3 else f"{n / 1024**2:.0f} MiB"


@dataclass(frozen=True)
class BuildStats:
    """One build's moc cache events; ``counts`` holds every kind except hits."""

    scope: str
    note: str
    cache_dir: Path
    hits: int = 0
    misses: int = 0
    counts: dict[str, int] = field(default_factory=dict)
    details: list[tuple[str, str, str]] = field(default_factory=list)

    def size(self) -> str:
        """Cache size, with the MOCCACHE_MAX_SIZE limit when one is set."""
        size = format_bytes(sum(size for _, size, _ in scan_entries(self.cache_dir)))
        limit = os.environ.get("MOCCACHE_MAX_SIZE", "")
        if limit:
            with contextlib.suppress(ValueError):
                size += f" of {format_bytes(parse_size(limit))}"
        return size


def collect_stats(build_dir: Path) -> BuildStats:
    """Return the moc cache events of the ninja build in *build_dir* and prune older records."""
    since = build_start(build_dir)
    scope = "this build"
    note = ""
    if since is None:
        scope = "all recorded"
        note = (
            f"build start unknown: no usable {build_dir / '.ninja_log'}; showing all recorded stats"
        )
    basedir = os.environ.get("MOCCACHE_BASEDIR", "") if since is not None else ""
    basedir = basedir.translate(_STAT_FIELD_BREAKERS)  # match what _log_stat recorded

    cache_dir = _cache_dir()
    log = cache_dir / "stats.log"
    counts: dict[str, int] = {}
    details: list[tuple[str, str, str]] = []  # (kind, input, detail) for non-hit events
    keep: list[str] = []  # this build's lines from its start onward, plus every other build dir's
    try:
        with log.open(encoding="utf-8", errors="replace") as f:
            for raw in f:
                line = raw.rstrip("\n")
                fields = line.split("\t")
                if since is not None:
                    when = stat_time(fields)
                    if when is None:
                        continue  # legacy line: prune
                    if basedir and fields[3] != basedir:
                        # Another build dir's record; a still-running sibling
                        # build may need it, so only its own report prunes it.
                        keep.append(line)
                        continue
                    if when < since:
                        continue  # older than this build: prune
                    keep.append(line)
                else:
                    keep.append(line)
                kind = fields[0]
                counts[kind] = counts.get(kind, 0) + 1
                if kind != "hit":
                    details.append(
                        (
                            kind,
                            fields[1] if len(fields) > 1 else "",
                            fields[4] if len(fields) > 4 else "",
                        )
                    )
    except OSError:
        pass
    # Only per-build reports are ever wanted, so this build's older entries are
    # dead weight; drop them to keep the log about one build long. The cap
    # bounds records left behind by build dirs that never report again. A
    # concurrent build appending during this rewrite can lose a few lines;
    # acceptable for a diagnostics aid.
    if since is not None or len(keep) > _STATS_LOG_MAX_LINES:
        with contextlib.suppress(OSError):
            _atomic_write(log, "".join(line + "\n" for line in keep[-_STATS_LOG_MAX_LINES:]))
    hits = counts.pop("hit", 0)
    # Pre-reason logs used a bare "miss"; fold it in with the miss-* reasons.
    misses = counts.pop("miss", 0) + sum(n for k, n in counts.items() if k.startswith("miss-"))
    return BuildStats(scope, note, cache_dir, hits, misses, counts, details)


def show_stats_main(argv: list[str]) -> int:
    build_dir: Path | None = None
    verbose = False
    i = 0
    while i < len(argv):
        if argv[i] == "--build-dir":
            if i + 1 >= len(argv):
                print("moccache: --build-dir requires a path", file=sys.stderr)
                return 2
            build_dir = Path(argv[i + 1])
            i += 2
        elif argv[i] == "--verbose":
            verbose = True
            i += 1
        else:
            print(f"moccache: --show-stats: unknown argument: {argv[i]}", file=sys.stderr)
            return 2
    if build_dir is None:
        print("moccache: --show-stats requires --build-dir", file=sys.stderr)
        return 2

    stats = collect_stats(build_dir)
    counts = stats.counts
    total = stats.hits + stats.misses
    print(f"moccache stats ({stats.scope}, {stats.cache_dir}):")
    if stats.note:
        print(f"  ({stats.note})")
    if total == 0 and not counts:
        print("  no stats recorded")
        return 0
    print(f"  hits     {stats.hits}")
    print(f"  misses   {stats.misses}")
    if total:
        print(f"  hit rate {100.0 * stats.hits / total:.1f}%")
    width = max((len(k) for k in counts), default=0)
    for what in sorted(counts):
        why = KIND_DESCRIPTIONS.get(what)
        suffix = f"  ({why})" if why else ""
        print(f"  {what:<{width}}  {counts[what]:>5}{suffix}")
    if verbose and stats.details:
        print("details:")
        for kind, input_file, detail in stats.details:
            print(f"  {kind}  {input_file}  {detail}".rstrip())
    return 0


def zero_stats_main(argv: list[str]) -> int:
    if argv:
        print(f"moccache: --zero-stats: unknown argument: {argv[0]}", file=sys.stderr)
        return 2
    with contextlib.suppress(OSError):
        (_cache_dir() / "stats.log").unlink()
    return 0
