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

Environment:
    MOCCACHE_MOC      Path to the real moc (if --real-moc not given).
    MOCCACHE_DIR      Cache directory (default ~/.cache/moccache).
    MOCCACHE_BASEDIR  Build dir root; rewritten to a token in cache keys and
                      manifests so different build trees share cache entries
                      (same idea as ccache's base_dir).
    MOCCACHE_MAX_SIZE Size limit (e.g. 256M, 1G). If set, the cache is
                      LRU-trimmed opportunistically after misses (at most once
                      per hour). Unset = unlimited.
    MOCCACHE_DISABLE  If set, always pass through to real moc.
    MOCCACHE_STATS    If set, append hit/miss lines to $MOCCACHE_DIR/stats.log.
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
    dst.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=str(dst.parent), prefix=".moccache-")
    try:
        with os.fdopen(fd, "w") as f:
            f.write(data)
        os.replace(tmp, dst)
    except BaseException:
        with contextlib.suppress(OSError):
            os.unlink(tmp)
        raise


def _make_escape(path: str) -> str:
    return path.replace(" ", "\\ ")


_BASEDIR_TOKEN = "<<MOCCACHE_BASEDIR>>"


def _basedir_prefixes() -> list[str]:
    """Build-dir spellings to rewrite (raw and resolved), longest first."""
    raw = os.environ.get("MOCCACHE_BASEDIR", "").rstrip("/")
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
    raw = os.environ.get("MOCCACHE_BASEDIR", "").rstrip("/")
    return s.replace(_BASEDIR_TOKEN, raw) if raw else s


def _write_dep_file(dst: Path, target: str, deps: list[str]) -> None:
    body = " \\\n  ".join(_make_escape(d) for d in deps)
    _atomic_write(dst, f"{_make_escape(target)}: \\\n  {body}\n")


def _log_stat(cache_dir: Path, what: str, input_file: str) -> None:
    if not os.environ.get("MOCCACHE_STATS"):
        return
    try:
        with (cache_dir / "stats.log").open("a") as f:
            f.write(f"{what}\t{input_file}\n")
    except OSError:
        pass


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
        _log_stat(cache_dir, "bad-max-size", limit)  # misconfig breadcrumb; cache stays untrimmed
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
            _trim(cache_dir, max_bytes)
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
    cache_dir = Path(os.environ.get("MOCCACHE_DIR", str(Path.home() / ".cache" / "moccache")))
    removed = _trim(cache_dir, max_bytes)
    print(f"moccache: trimmed {removed} entries from {cache_dir}")
    return 0


def main() -> int:
    argv = sys.argv[1:]
    if argv and argv[0] == "--trim":
        return _trim_main(argv[1:])
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
        _parse_args(argv)
    )
    if not output or not input_file or not Path(input_file).is_file():
        return passthrough()

    cache_dir = Path(os.environ.get("MOCCACHE_DIR", str(Path.home() / ".cache" / "moccache")))
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
    if manifest.is_file() and cached_out.is_file() and (not wants_json or cached_json.is_file()):
        hit = True
        manifest_deps: list[str] = []
        try:
            for line in manifest.read_text().splitlines():
                dep, _, dep_hash = line.partition("\t")
                dep = _denormalize_basedir(dep)
                p = Path(dep)
                if not p.is_file() or _sha256_file(p) != dep_hash:
                    hit = False
                    break
                manifest_deps.append(dep)
        except OSError:
            hit = False
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
                pass  # fall through to a real run

    # --- Miss: run real moc with dep-file capture ---------------------------
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
            dep_text = dep_src.read_text()
        except OSError:
            _log_stat(cache_dir, "miss-nodep", input_file)
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
        except OSError:
            pass  # cache write failure must not fail the build
        _log_stat(cache_dir, "miss", input_file)
        _maybe_auto_trim(cache_dir)
        return 0
    finally:
        if tmp_dep:
            with contextlib.suppress(OSError):
                os.unlink(tmp_dep)


if __name__ == "__main__":
    sys.exit(main())
