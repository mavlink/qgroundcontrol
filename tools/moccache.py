#!/usr/bin/env python3
"""moccache — a content-addressed cache for Qt's moc.

Speeds up clean builds / CI / branch switches by caching moc output keyed on
moc identity + arguments + input content + all transitive include contents
(tracked via moc's --output-dep-file, Qt 5.15+).

Usage (as CMake AUTOMOC executable, via a shim that bakes the real moc path):
    moccache.py --real-moc /path/to/moc [moc args...]
or set MOCCACHE_MOC=/path/to/moc and invoke:
    moccache.py [moc args...]
or trim the cache to a size limit (LRU eviction):
    moccache.py --trim [--max-size 256M]
or report / reset recorded stats (requires MOCCACHE_STATS during the build):
    moccache.py --show-stats --build-dir DIR [--verbose]
    moccache.py --zero-stats

Environment:
    MOCCACHE_MOC      Path to the real moc (if --real-moc not given).
    MOCCACHE_DIR      Cache directory (default <source tree>/.cache/moccache,
                      matching the CMake-generated launcher).
    MOCCACHE_BASEDIR  Build dir root; rewritten to a token in cache keys and
                      manifests so different build trees share cache entries
                      (same idea as ccache's base_dir).
    MOCCACHE_MAX_SIZE Size limit (e.g. 256M, 1G). If set, the cache is
                      LRU-trimmed opportunistically after misses (at most once
                      per hour). Unset = unlimited.
    MOCCACHE_DISABLE  If set, always pass through to real moc.
    MOCCACHE_STATS    If set, append "<kind>\t<input>\t<epoch>\t<basedir>\t<detail>"
                      lines to $MOCCACHE_DIR/stats.log. --show-stats reports
                      only the ninja build in DIR (entries newer than the
                      build start derived from DIR/.ninja_log, from this
                      MOCCACHE_BASEDIR) and then prunes that build dir's older
                      entries, so the log stays about one build long (other
                      build dirs' entries are left for their own reports; when
                      the build start cannot be derived it reports everything).
                      The log is always capped at _STATS_LOG_MAX_LINES. The
                      CMake-generated moccache-stats post-build script runs it
                      after linking.

Stats kinds. Misses carry a reason so legitimate misses can be told apart
from cache-design problems (a per-input index of recent keys under
$MOCCACHE_DIR/index makes the comparison possible). --show-stats prints each
kind's count with the explanation from _KIND_DESCRIPTIONS; in short:
    expected      miss-first-seen, miss-header-changed, miss-include-changed
                  (detail = the include), miss-predefs-changed (after a
                  compiler-flag change), miss-moc-version-changed (Qt upgrade)
    investigate   miss-args-changed (detail = first differing arg; -I/-D churn
                  between build trees defeats sharing), miss-output-depth-differs
    cache size    miss-cache-evicted, miss-history-evicted -> raise MOCCACHE_MAX_SIZE
    other         miss-uncacheable (moc gave no dep file), miss-cache-unreadable
                  (entry incomplete or could not be copied out), cache-trimmed
                  (detail = entries evicted)
"""

from __future__ import annotations

import contextlib
import functools
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# Options whose value is a separate following argument.
_VALUE_OPTS = {
    "-o",
    "--include",
    "--dep-file-path",
    "--dep-file-rule-name",
    "-F",
    "-M",
    "--collect-json",
    "-b",
    "-f",
    "-p",
    "-n",
    "--compiler-flavor",
    "-t",
    "-A",
    "--json-output",
}


def _expand_response_files(argv: list[str]) -> list[str]:
    """Expand CMake AUTOMOC response files for cache-key parsing."""
    expanded: list[str] = []
    for arg in argv:
        if not arg.startswith("@") or len(arg) == 1:
            expanded.append(arg)
            continue
        try:
            expanded.extend(Path(arg[1:]).read_text(encoding="utf-8").splitlines())
        except (OSError, UnicodeError):
            expanded.append(arg)
    return expanded


def _sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def _sha256_file_normalized(path: Path, prefixes: list[str]) -> str:
    """Hash file content with build-dir paths rewritten to the basedir token.

    Needed for force-included files like moc_predefs.h, which embed the build
    dir (e.g. QT_TESTCASE_BUILDDIR) and would otherwise hash differently per
    tree.
    """
    if not prefixes:
        return _sha256_file(path)
    # Whole-file read is deliberate: the only caller passes AUTOMOC's
    # moc_predefs.h (a few KB), so streaming isn't worth the complexity.
    data = path.read_bytes()
    pattern = _basedir_pattern(tuple(prefixes))
    data = re.sub(pattern.encode(), _BASEDIR_TOKEN.encode(), data)
    return hashlib.sha256(data).hexdigest()


def _moc_identity(moc: Path) -> str:
    try:
        out = subprocess.run(
            [str(moc), "--version"], capture_output=True, text=True, check=True
        ).stdout.strip()
        if out:
            return out
    except (OSError, subprocess.CalledProcessError):
        pass
    st = moc.stat()
    return f"{st.st_mtime_ns}:{st.st_size}"


def _parse_args(argv: list[str]):
    """Return (output, input_file, dep_file_path, wants_dep_file, wants_json,
    hashable_args, include_files)."""
    output = None
    dep_file_path = None
    wants_dep_file = False
    wants_json = False
    hashable: list[str] = []
    positional: list[str] = []
    include_files: list[str] = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "-o" or a == "--output":
            i += 1
            if i < len(argv):
                output = argv[i]
        elif a == "--output-dep-file":
            wants_dep_file = True
        elif a == "--output-json":
            wants_json = True
            hashable.append(a)
        elif a == "--dep-file-path":
            wants_dep_file = True
            i += 1
            if i < len(argv):
                dep_file_path = argv[i]
        elif a == "--include":
            # Force-included files (e.g. AUTOMOC's moc_predefs.h) live in the
            # build dir, so their *path* varies across build trees. Key on
            # their content instead (with build-dir paths inside the content
            # rewritten to a token); the path is excluded from the key.
            i += 1
            if i < len(argv):
                include_files.append(argv[i])
                p = Path(argv[i])
                content_id = (
                    _sha256_file_normalized(p, _basedir_prefixes()) if p.is_file() else argv[i]
                )
                hashable.append("--include")
                hashable.append(content_id)
        elif a in _VALUE_OPTS:
            hashable.append(a)
            i += 1
            if i < len(argv):
                hashable.append(argv[i])
        elif a.startswith("-") and a != "-":
            hashable.append(a)
        else:
            positional.append(a)
            hashable.append(a)
        i += 1
    input_file = positional[-1] if positional else None
    return output, input_file, dep_file_path, wants_dep_file, wants_json, hashable, include_files


def _parse_dep_file(text: str) -> list[str]:
    """Parse a Make-style dep file into a list of dependency paths."""
    # Strip line continuations, then split on the first unescaped ':'.
    text = text.replace("\\\n", " ").replace("\\\r\n", " ")
    colon = -1
    i = 0
    while i < len(text):
        if text[i] == ":" and (i + 1 >= len(text) or text[i + 1] in " \t\n\r"):
            colon = i
            break
        i += 1
    deps_part = text[colon + 1 :] if colon >= 0 else text
    # Unescape "\ " (escaped spaces in paths) by tokenizing manually.
    deps: list[str] = []
    cur: list[str] = []
    j = 0
    while j < len(deps_part):
        c = deps_part[j]
        if c == "\\" and j + 1 < len(deps_part) and deps_part[j + 1] == " ":
            cur.append(" ")
            j += 2
            continue
        if c in " \t\n\r":
            if cur:
                deps.append("".join(cur))
                cur = []
        else:
            cur.append(c)
        j += 1
    if cur:
        deps.append("".join(cur))
    return deps


def _atomic_copy(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=str(dst.parent), prefix=".moccache-")
    os.close(fd)
    try:
        shutil.copyfile(src, tmp)
        os.replace(tmp, dst)
    except BaseException:
        with contextlib.suppress(OSError):
            os.unlink(tmp)
        raise


def _atomic_write(dst: Path, data: str) -> None:
    """Replace dst with data as UTF-8/LF, independent of the platform locale."""
    dst.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=str(dst.parent), prefix=".moccache-")
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as f:
            f.write(data)
        os.replace(tmp, dst)
    except BaseException:
        with contextlib.suppress(OSError):
            os.unlink(tmp)
        raise


def _make_escape(path: str) -> str:
    return path.replace(" ", "\\ ")


def _append_line(path: Path, line: str) -> None:
    """Append one line as a single write(2) on an O_APPEND descriptor.

    The OS performs each such write atomically on local filesystems, so
    concurrent processes cannot interleave or clobber each other's lines;
    buffered text I/O gives no such guarantee. (Lines over PIPE_BUF, 4 KiB
    on POSIX, lose it too.)
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    flags = os.O_WRONLY | os.O_APPEND | os.O_CREAT | getattr(os, "O_BINARY", 0)
    fd = os.open(path, flags, 0o600)
    try:
        os.write(fd, (line + "\n").encode("utf-8"))
    finally:
        os.close(fd)


_BASEDIR_TOKEN = "<<MOCCACHE_BASEDIR>>"


def _basedir_prefixes() -> list[str]:
    """Build-dir spellings to rewrite (raw and resolved), longest first."""
    raw = os.environ.get("MOCCACHE_BASEDIR", "").rstrip("/\\")
    if not raw:
        return []
    prefixes = {raw}
    with contextlib.suppress(OSError):
        prefixes.add(os.path.realpath(raw))
    return sorted(prefixes, key=len, reverse=True)


@functools.lru_cache(maxsize=8)
def _basedir_pattern(prefixes: tuple[str, ...]) -> str:
    """Regex matching a prefix only at a path-component boundary.

    A positive lookahead requires the prefix to be followed by a character
    that actually terminates a path in moc args / C headers (end-of-string,
    path separator, quote, whitespace, backslash). Anything else — including
    uncommon-but-legal filename chars like + or @ — extends a sibling name
    (/repo/build vs /repo/build2, build-old, build+1, ...) and must not match.
    """
    return "(?:" + "|".join(re.escape(p) for p in prefixes) + r")(?=$|[/\"'\s\\])"


def _normalize_basedir(s: str, prefixes: list[str]) -> str:
    if not prefixes:
        return s
    return re.sub(_basedir_pattern(tuple(prefixes)), _BASEDIR_TOKEN, s)


def _denormalize_basedir(s: str) -> str:
    raw = os.environ.get("MOCCACHE_BASEDIR", "").rstrip("/\\")
    return s.replace(_BASEDIR_TOKEN, raw) if raw else s


def _write_dep_file(dst: Path, target: str, deps: list[str]) -> None:
    body = " \\\n  ".join(_make_escape(d) for d in deps)
    _atomic_write(dst, f"{_make_escape(target)}: \\\n  {body}\n")


_STAT_FIELD_BREAKERS = str.maketrans({"\t": " ", "\n": " ", "\r": " "})
_SHA256_HEX = re.compile(r"[0-9a-f]{64}")


def _log_stat(cache_dir: Path, what: str, input_file: str, detail: str = "") -> None:
    """Append a stats line; input_file is a moc input path or "" for non-input events."""
    if not os.environ.get("MOCCACHE_STATS"):
        return
    basedir = os.environ.get("MOCCACHE_BASEDIR", "")
    # Free-text fields must not be able to break the tab-separated record.
    input_file, basedir, detail = (
        s.translate(_STAT_FIELD_BREAKERS) for s in (input_file, basedir, detail)
    )
    with contextlib.suppress(OSError):
        _append_line(
            cache_dir / "stats.log",
            f"{what}\t{input_file}\t{time.time():.3f}\t{basedir}\t{detail}",
        )


# Per-input index of recent cache keys, used only to explain misses.
_INDEX_MAX_KEYS = 8

# Shown next to each kind in --show-stats so the report is self-explanatory.
_KIND_DESCRIPTIONS = {
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


def _index_path(cache_dir: Path, input_file: str, basedir_prefixes: list[str]) -> Path:
    ident = hashlib.sha256(
        _normalize_basedir(os.path.abspath(input_file), basedir_prefixes).encode()
    ).hexdigest()
    return cache_dir / "index" / ident[:2] / ident


def _index_read(path: Path) -> list[str]:
    """The newest _INDEX_MAX_KEYS distinct keys, oldest first."""
    try:
        raw = path.read_text(encoding="utf-8").split()
    except (OSError, UnicodeDecodeError):
        return []
    newest_first = list(dict.fromkeys(reversed(raw)))
    return newest_first[:_INDEX_MAX_KEYS][::-1]


# Compact once the append-only file grows well past what _index_read keeps.
_INDEX_COMPACT_LINES = _INDEX_MAX_KEYS * 8


def _index_add(path: Path, mkey: str) -> None:
    """Record mkey for this input.

    Append-only (see _append_line) so concurrent misses on the same header
    (parallel builds of sibling trees) never lose each other's keys the way
    a read-modify-write would. Compaction rewrites rarely enough that its
    small race window is acceptable for a diagnostics aid.
    """
    with contextlib.suppress(OSError):
        _append_line(path, mkey)
        if path.stat().st_size > _INDEX_COMPACT_LINES * (len(mkey) + 1):
            _atomic_write(path, "\n".join(_index_read(path)) + "\n")


def _diff_key_parts(old: list[str], new: list[str]) -> tuple[str, str]:
    """Classify how a previous key differs from the current one.

    Layout is [moc identity, *args, input sha, relpath]; args are compared
    after the fixed parts so the most legitimate cause wins.
    """
    if len(old) < 3 or len(new) < 3:
        return "miss-args-changed", "key layout changed"
    if old[0] != new[0]:
        return "miss-moc-version-changed", f"{old[0]} -> {new[0]}"
    if old[-2] != new[-2]:
        return "miss-header-changed", ""
    if old[-1] != new[-1]:
        return "miss-output-depth-differs", f"{old[-1]} -> {new[-1]}"
    old_args, new_args = old[1:-2], new[1:-2]
    for i, (a, b) in enumerate(zip(old_args, new_args, strict=False)):
        if a != b:
            if i > 0 and old_args[i - 1] == "--include":
                return "miss-predefs-changed", ""
            return "miss-args-changed", f"{a} -> {b}"
    if len(old_args) != len(new_args):
        extra = (
            new_args[len(old_args) :]
            if len(new_args) > len(old_args)
            else old_args[len(new_args) :]
        )
        sign = "+" if len(new_args) > len(old_args) else "-"
        return "miss-args-changed", sign + " ".join(extra)
    return "miss-args-changed", "identical key parts"  # should not happen: same key would have hit


def _explain_key_miss(
    cache_dir: Path, index: Path, mkey: str, key_parts: list[str]
) -> tuple[str, str]:
    """Reason for finding no entry at mkey, using the input's previous keys."""
    previous = _index_read(index)
    if not previous:
        return "miss-first-seen", ""
    if mkey in previous:
        return "miss-cache-evicted", ""
    best: tuple[int, str, str] | None = None
    for old_key in reversed(previous):
        try:
            old_parts = (
                (cache_dir / old_key[:2] / old_key / "keyinfo")
                .read_text(encoding="utf-8")
                .splitlines()
            )
        except (OSError, UnicodeDecodeError):
            continue
        kind, detail = _diff_key_parts(old_parts, key_parts)
        ndiff = sum(1 for a, b in zip(old_parts, key_parts, strict=False) if a != b) + abs(
            len(old_parts) - len(key_parts)
        )
        if best is None or ndiff < best[0]:
            best = (ndiff, kind, detail)
    if best is None:
        return "miss-history-evicted", "previous entries evicted"
    return best[1], best[2]


_SIZE_SUFFIXES = {"k": 1024, "m": 1024**2, "g": 1024**3, "t": 1024**4}
_TRIM_INTERVAL_SECONDS = 3600
_TRIM_LOW_WATER = 0.8  # trim to 80% of the limit so trims aren't back-to-back


def _parse_size(s: str) -> int:
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


def _trim(cache_dir: Path, max_bytes: int) -> int:
    """LRU-evict cache entries until total size <= 80% of max_bytes.

    Entry recency is the entry directory's mtime (refreshed on hits).
    Returns the number of entries removed. Never raises: a concurrently
    written or vanished entry is skipped.
    """
    entries = []  # (mtime, size, path)
    total = 0
    for shard in cache_dir.glob("??"):
        if not shard.is_dir():
            continue
        for mdir in shard.iterdir():
            with contextlib.suppress(OSError):
                mtime = mdir.stat().st_mtime
                size = sum(f.stat().st_size for f in mdir.iterdir() if f.is_file())
                entries.append((mtime, size, mdir))
                total += size
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


def _maybe_auto_trim(cache_dir: Path) -> None:
    """Trim after a miss if MOCCACHE_MAX_SIZE is set, at most once per hour.

    The trim slot is claimed atomically (O_EXCL lock file) so concurrent
    misses don't all walk the cache; a lock left by a crashed holder is
    reclaimed once it is older than the trim interval.
    """
    limit = os.environ.get("MOCCACHE_MAX_SIZE")
    if not limit:
        return
    try:
        max_bytes = _parse_size(limit)
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
            removed = _trim(cache_dir, max_bytes)
            if removed:
                _log_stat(cache_dir, "cache-trimmed", "", str(removed))
            stamp.touch()  # only after a completed trim; a crash must not suppress retries
        finally:
            with contextlib.suppress(OSError):
                lock.unlink()


def _trim_main(argv: list[str]) -> int:
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
        max_bytes = _parse_size(max_size)
    except ValueError as e:
        print(f"moccache: {e}", file=sys.stderr)
        return 2
    cache_dir = _cache_dir()
    removed = _trim(cache_dir, max_bytes)
    print(f"moccache: trimmed {removed} entries from {cache_dir}")
    return 0


def _cache_dir() -> Path:
    """Cache directory: $MOCCACHE_DIR, else <source tree>/.cache/moccache.

    The source-tree default matches the CMake-generated launcher so the
    --show-stats/--trim CLI works out-of-the-box after a normal build.
    """
    env = os.environ.get("MOCCACHE_DIR")
    if env:
        return Path(env)
    return Path(__file__).resolve().parent.parent / ".cache" / "moccache"


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


def _ninja_mtime_to_epoch(mtime: int) -> float:
    if _NINJA_LOG_MTIME_IS_FILETIME:
        return mtime / 1e7 + _NINJA_FILETIME_UNIX_OFFSET
    return mtime / 1e9


def _build_start(build_dir: Path) -> float | None:
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
        candidate = _ninja_mtime_to_epoch(mtime) - end_ms / 1000.0
        if start is None or candidate > start:
            start = candidate
    if start is None:
        return None
    now = time.time()
    if start > now + _BUILD_START_MAX_FUTURE_SECONDS or start < now - _BUILD_START_MAX_AGE_SECONDS:
        return None
    return start - _BUILD_START_SLACK_SECONDS


def _stat_time(fields: list[str]) -> float | None:
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


def _show_stats_main(argv: list[str]) -> int:
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

    since = _build_start(build_dir)
    scope = "this build"
    note = ""
    if since is None:
        scope = "all recorded"
        note = f"  (build start unknown: no usable {build_dir / '.ninja_log'}; showing all recorded stats)"
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
                    when = _stat_time(fields)
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
    total = hits + misses
    print(f"moccache stats ({scope}, {cache_dir}):")
    if note:
        print(note)
    if total == 0 and not counts:
        print("  no stats recorded")
        return 0
    print(f"  hits     {hits}")
    print(f"  misses   {misses}")
    if total:
        print(f"  hit rate {100.0 * hits / total:.1f}%")
    width = max((len(k) for k in counts), default=0)
    for what in sorted(counts):
        why = _KIND_DESCRIPTIONS.get(what)
        suffix = f"  ({why})" if why else ""
        print(f"  {what:<{width}}  {counts[what]:>5}{suffix}")
    if verbose and details:
        print("details:")
        for kind, input_file, detail in details:
            print(f"  {kind}  {input_file}  {detail}".rstrip())
    return 0


def _zero_stats_main(argv: list[str]) -> int:
    if argv:
        print(f"moccache: --zero-stats: unknown argument: {argv[0]}", file=sys.stderr)
        return 2
    with contextlib.suppress(OSError):
        (_cache_dir() / "stats.log").unlink()
    return 0


def main() -> int:
    argv = sys.argv[1:]
    if argv and argv[0] == "--trim":
        return _trim_main(argv[1:])
    if argv and argv[0] == "--show-stats":
        return _show_stats_main(argv[1:])
    if argv and argv[0] == "--zero-stats":
        return _zero_stats_main(argv[1:])
    real_moc = None
    if argv and argv[0] == "--real-moc":
        if len(argv) < 2:
            print("moccache: --real-moc requires a path", file=sys.stderr)
            return 2
        real_moc = Path(argv[1])
        argv = argv[2:]
    elif os.environ.get("MOCCACHE_MOC"):
        real_moc = Path(os.environ["MOCCACHE_MOC"])
    if real_moc is None or not real_moc.is_file():
        print("moccache: real moc not found (set MOCCACHE_MOC or use --real-moc)", file=sys.stderr)
        return 2

    def passthrough() -> int:
        return subprocess.run([str(real_moc), *argv]).returncode

    if os.environ.get("MOCCACHE_DISABLE"):
        return passthrough()

    output, input_file, dep_file_path, wants_dep_file, wants_json, hashable, include_files = (
        _parse_args(_expand_response_files(argv))
    )
    if not output or not input_file or not Path(input_file).is_file():
        return passthrough()

    cache_dir = _cache_dir()
    basedir_prefixes = _basedir_prefixes()

    # Manifest key: moc identity + args (minus output/dep paths) + input content
    # + the input's path relative to the output dir. moc embeds that relative
    # path as an #include in its output, so trees whose output dirs sit at a
    # different depth must not share entries. Sibling build dirs (same depth)
    # yield the same relpath and still share. Build-dir paths in the args are
    # rewritten to a token so different build trees produce the same key.
    key_parts = [
        _moc_identity(real_moc),
        *(_normalize_basedir(a, basedir_prefixes) for a in hashable),
        _sha256_file(Path(input_file)),
        os.path.relpath(input_file, os.path.dirname(os.path.abspath(output)) or "."),
    ]
    h = hashlib.sha256()
    for part in key_parts:
        h.update(part.encode())
        h.update(b"\x00")
    mkey = h.hexdigest()
    mdir = cache_dir / mkey[:2] / mkey

    manifest = mdir / "manifest"
    cached_out = mdir / "output.cpp"
    cached_json = mdir / "output.json"
    out_path = Path(output)
    dep_out = Path(dep_file_path) if dep_file_path else Path(str(out_path) + ".d")
    json_out = Path(str(out_path) + ".json")

    # --- Try for a hit ------------------------------------------------------
    miss_reason: tuple[str, str] | None = None
    if manifest.is_file() and cached_out.is_file() and (not wants_json or cached_json.is_file()):
        hit = True
        manifest_deps: list[str] = []
        try:
            for line in manifest.read_text(encoding="utf-8").splitlines():
                dep, _, dep_hash = line.partition("\t")
                if not dep or not _SHA256_HEX.fullmatch(dep_hash):
                    hit = False
                    miss_reason = ("miss-cache-unreadable", "manifest malformed")
                    break
                dep = _denormalize_basedir(dep)
                p = Path(dep)
                if not p.is_file() or _sha256_file(p) != dep_hash:
                    hit = False
                    miss_reason = ("miss-include-changed", dep)
                    break
                manifest_deps.append(dep)
        except (OSError, UnicodeDecodeError):
            hit = False
            miss_reason = ("miss-cache-unreadable", "manifest unreadable")
        if hit:
            try:
                out_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(cached_out, out_path)
                if wants_dep_file:
                    # Synthesize for this tree: the cached dep file references
                    # the originating tree's output and moc_predefs.h paths.
                    deps = list(dict.fromkeys([input_file, *include_files, *manifest_deps]))
                    _write_dep_file(dep_out, output, deps)
                if wants_json:
                    shutil.copyfile(cached_json, json_out)
                with contextlib.suppress(OSError):
                    os.utime(mdir)  # refresh LRU recency
                _log_stat(cache_dir, "hit", input_file)
                return 0
            except OSError:
                miss_reason = ("miss-cache-unreadable", "")  # fall through to a real run
    elif mdir.is_dir():
        miss_reason = ("miss-cache-unreadable", "entry incomplete")

    # --- Miss: run real moc with dep-file capture ---------------------------
    index = _index_path(cache_dir, input_file, basedir_prefixes)
    if miss_reason is None:
        # Explaining costs up to _INDEX_MAX_KEYS keyinfo reads; skip when nobody is listening.
        if os.environ.get("MOCCACHE_STATS"):
            miss_reason = _explain_key_miss(cache_dir, index, mkey, key_parts)
        else:
            miss_reason = ("miss", "")
    tmp_dep = None
    cmd = [str(real_moc), *argv]
    if not wants_dep_file:
        fd, tmp_dep = tempfile.mkstemp(suffix=".d", prefix="moccache-")
        os.close(fd)
        cmd += ["--output-dep-file", "--dep-file-path", tmp_dep]
    try:
        rc = subprocess.run(cmd).returncode
        if rc != 0:
            return rc

        dep_src = Path(tmp_dep) if tmp_dep else dep_out
        try:
            dep_text = dep_src.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            _log_stat(cache_dir, "miss-uncacheable", input_file)
            return 0  # moc succeeded; just can't cache

        # Force-included files are keyed by content, so their (build-dir
        # specific) paths must stay out of the manifest. Build-dir dep paths
        # are stored token-relative so other trees can validate them.
        include_reals = {os.path.realpath(f) for f in include_files}
        lines = []
        for dep in _parse_dep_file(dep_text):
            p = Path(dep)
            if p.is_file() and os.path.realpath(dep) not in include_reals:
                lines.append(f"{_normalize_basedir(dep, basedir_prefixes)}\t{_sha256_file(p)}")
        try:
            _atomic_copy(out_path, cached_out)
            if wants_json and json_out.is_file():
                _atomic_copy(json_out, cached_json)
            _atomic_write(manifest, "\n".join(lines) + "\n")
            _atomic_write(mdir / "keyinfo", "\n".join(key_parts) + "\n")
            _index_add(index, mkey)
        except OSError:
            pass  # cache write failure must not fail the build
        _log_stat(cache_dir, miss_reason[0], input_file, miss_reason[1])
        _maybe_auto_trim(cache_dir)
        return 0
    finally:
        if tmp_dep:
            with contextlib.suppress(OSError):
                os.unlink(tmp_dep)


if __name__ == "__main__":
    sys.exit(main())
