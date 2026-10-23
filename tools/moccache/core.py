"""moccache hot path: cache-key computation, hit validation and the miss path.

Everything a moc run needs lives here; see the package docstring for usage.
"""

from __future__ import annotations

import contextlib
import functools
import hashlib
import os
import re
import stat
import sys
import time
from pathlib import Path

# subprocess, shutil, tempfile and the trim and stats submodules are imported
# where they are needed: a hit never needs them, and the imports are a
# measurable share of a hit's runtime.

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
    return _sha256_file_stat(path)[0]


def _sha256_file_stat(path: Path) -> tuple[str, os.stat_result]:
    """Hash path and fstat the same open file, so both describe one version of it."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        st = os.fstat(f.fileno())
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest(), st


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


# `moc --version` memo, beside the ?? entry shards (the only things trimming
# and size accounting ever walk). Bounded because a few Qt installs may share
# one cache dir.
_MOC_VERSIONS_FILE = "moc-versions"
_MOC_VERSIONS_MAX = 8


def _read_moc_versions(path: Path) -> dict[str, tuple[int, int, str]]:
    """{resolved moc path: (mtime_ns, size, version)}; empty if missing or unreadable."""
    try:
        lines = path.read_text(encoding="utf-8").split("\n")
    except (OSError, UnicodeDecodeError):
        return {}
    versions: dict[str, tuple[int, int, str]] = {}
    for line in lines[:-1]:  # the last element is "" or an unterminated, partial line
        fields = line.split("\t")
        if len(fields) != 4 or not fields[3]:
            continue
        try:
            versions[fields[0]] = (int(fields[1]), int(fields[2]), fields[3])
        except ValueError:
            continue
    return versions


def _moc_identity(moc: Path, cache_dir: Path) -> str:
    """moc's --version output (else its mtime:size), memoized per moc binary.

    moc is only asked again when its resolved path, mtime or size differ from
    the memo's. The memo is best effort: when missing, corrupt or unwritable,
    moc is asked; concurrent updates may drop an entry, costing one more ask.
    """
    st = moc.stat()
    resolved = os.path.realpath(moc)
    memo = cache_dir / _MOC_VERSIONS_FILE
    versions = _read_moc_versions(memo)
    known = versions.get(resolved)
    if known is not None and known[:2] == (st.st_mtime_ns, st.st_size):
        return known[2]
    import subprocess

    try:
        version = subprocess.run(
            [str(moc), "--version"], capture_output=True, text=True, check=True
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        version = ""
    if not version:
        # Not memoized, so a transient failure doesn't stick.
        return f"{st.st_mtime_ns}:{st.st_size}"
    # Tabs, newlines or undecodable path bytes would corrupt the memo's format.
    if resolved.isprintable() and version.isprintable():
        versions.pop(resolved, None)
        versions[resolved] = (st.st_mtime_ns, st.st_size, version)
        recent = list(versions.items())[-_MOC_VERSIONS_MAX:]
        with contextlib.suppress(OSError):
            _atomic_write(memo, "".join(f"{p}\t{m}\t{s}\t{v}\n" for p, (m, s, v) in recent))
    return version


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
    import shutil
    import tempfile

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
    # A pid-unique name instead of tempfile.mkstemp keeps tempfile off the hit path
    tmp = str(dst.parent / f".moccache-{os.getpid()}-{dst.name}")
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_BINARY", 0)
    fd = os.open(tmp, flags, 0o600)
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
    if _BASEDIR_TOKEN not in s:
        return s
    raw = os.environ.get("MOCCACHE_BASEDIR", "").rstrip("/\\")
    return s.replace(_BASEDIR_TOKEN, raw) if raw else s


# A dependency's stat is recorded in the manifest only when the file's mtime
# and ctime are both at least this old when the manifest is written:
# rewritten again within one (possibly coarse) timestamp tick, a file can keep
# its whole stat while its content changes (git's "racy" case).
_STAT_RACY_WINDOW_NS = 2_000_000_000


def _stat_fields(st: os.stat_result) -> list[str]:
    """The stat recorded next to a dependency's hash.

    ctime catches content rewritten with its old mtime restored (archive
    re-extraction, cp -p), since no tool can set it. On Windows st_ctime_ns
    is the creation time instead, leaving mtime and size to catch writes.
    """
    return [str(st.st_mtime_ns), str(st.st_ctime_ns), str(st.st_size)]


def _manifest_line(dep: str, basedir_prefixes: list[str], racy_after_ns: int) -> str:
    """dep's manifest record: path, content hash and, when trustworthy, _stat_fields.

    The stat is left out for build-dir (tokenized) and relative paths, which
    name a different file in each build tree sharing the entry, and for files
    modified or changed at or after racy_after_ns.
    """
    digest, st = _sha256_file_stat(Path(dep))
    path = _normalize_basedir(dep, basedir_prefixes)
    if (
        path != dep
        or not os.path.isabs(dep)
        or max(st.st_mtime_ns, st.st_ctime_ns) >= racy_after_ns
    ):
        return f"{path}\t{digest}"
    return "\t".join([path, digest, *_stat_fields(st)])


def _dep_stat(dep: str) -> os.stat_result:
    """Stat dep the way _sha256_file_stat recorded it.

    On Windows, os.stat by path can report different times than os.fstat on an open handle
    (Python 3.12 reads cached directory metadata), so the comparison would never match there.
    """
    if os.name != "nt":
        return os.stat(dep)
    with open(dep, "rb") as f:
        return os.fstat(f.fileno())


def _dep_unchanged(dep: str, dep_hash: str, recorded_stat: list[str]) -> bool:
    """Whether dep still has the content hashed into the manifest.

    Re-hashing is skipped while dep's _stat_fields equal the recorded ones
    (empty for old-format or untrusted lines, which never match).
    """
    try:
        st = _dep_stat(dep)
    except (OSError, ValueError):
        return False
    if not stat.S_ISREG(st.st_mode):
        return False
    if recorded_stat == _stat_fields(st):
        return True
    return _sha256_file(Path(dep)) == dep_hash


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


def _cache_dir() -> Path:
    """Cache directory: $MOCCACHE_DIR, else <source tree>/.cache/moccache.

    The source-tree default matches the CMake-generated launcher so the
    --show-stats/--trim CLI works out-of-the-box after a normal build.
    """
    env = os.environ.get("MOCCACHE_DIR")
    if env:
        return Path(env)
    return Path(__file__).resolve().parents[2] / ".cache" / "moccache"


def main() -> int:
    argv = sys.argv[1:]
    if argv and argv[0] == "--trim":
        from .trim import trim_main

        return trim_main(argv[1:])
    if argv and argv[0] == "--show-stats":
        from .stats import show_stats_main

        return show_stats_main(argv[1:])
    if argv and argv[0] == "--zero-stats":
        from .stats import zero_stats_main

        return zero_stats_main(argv[1:])
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
        import subprocess

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
        _moc_identity(real_moc, cache_dir),
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
                # path<TAB>sha256, optionally followed by <TAB>mtime_ns<TAB>ctime_ns<TAB>size.
                fields = line.split("\t")
                if (
                    len(fields) not in (2, 5)
                    or not fields[0]
                    or not _SHA256_HEX.fullmatch(fields[1])
                ):
                    hit = False
                    miss_reason = ("miss-cache-unreadable", "manifest malformed")
                    break
                dep = _denormalize_basedir(fields[0])
                if not _dep_unchanged(dep, fields[1], fields[2:]):
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
                out_path.write_bytes(cached_out.read_bytes())
                if wants_dep_file:
                    # Synthesize for this tree: the cached dep file references
                    # the originating tree's output and moc_predefs.h paths.
                    deps = list(dict.fromkeys([input_file, *include_files, *manifest_deps]))
                    _write_dep_file(dep_out, output, deps)
                if wants_json:
                    json_out.write_bytes(cached_json.read_bytes())
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
        import tempfile

        fd, tmp_dep = tempfile.mkstemp(suffix=".d", prefix="moccache-")
        os.close(fd)
        cmd += ["--output-dep-file", "--dep-file-path", tmp_dep]
    import subprocess

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
        racy_after_ns = time.time_ns() - _STAT_RACY_WINDOW_NS
        lines = []
        for dep in _parse_dep_file(dep_text):
            if Path(dep).is_file() and os.path.realpath(dep) not in include_reals:
                lines.append(_manifest_line(dep, basedir_prefixes, racy_after_ns))
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
        if os.environ.get("MOCCACHE_MAX_SIZE"):
            from .trim import maybe_auto_trim

            maybe_auto_trim(cache_dir)
        return 0
    finally:
        if tmp_dep:
            with contextlib.suppress(OSError):
                os.unlink(tmp_dep)
