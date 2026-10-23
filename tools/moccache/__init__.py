"""moccache — a content-addressed cache for Qt's moc.

Speeds up clean builds / CI / branch switches by caching moc output keyed on
moc identity + arguments + input content + all transitive include contents
(tracked via moc's --output-dep-file, Qt 5.15+).

A hit re-validates every include recorded in the entry's manifest by content
hash, skipping the hash while the file's mtime, ctime and size still match the
ones recorded next to it. moc's --version is memoized in
$MOCCACHE_DIR/moc-versions per moc binary (resolved path, mtime, size), so a
hit spawns no process.

``core`` holds everything a moc run needs; ``trim`` (LRU eviction) and
``stats`` (reports) are imported only when their subcommands run or an
auto-trim is due, so a hit loads neither.

Usage (as CMake AUTOMOC executable, via a shim that bakes the real moc path):
    moccache_launcher.py --real-moc /path/to/moc [moc args...]
or set MOCCACHE_MOC=/path/to/moc and invoke:
    moccache_launcher.py [moc args...]
or trim the cache to a size limit (LRU eviction):
    moccache_launcher.py --trim [--max-size 1G]
or report / reset recorded stats (requires MOCCACHE_STATS during the build):
    moccache_launcher.py --show-stats --build-dir DIR [--verbose]
    moccache_launcher.py --zero-stats

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
                      The log is always capped at stats._STATS_LOG_MAX_LINES.
                      The CMake-generated moccache-stats post-build script
                      runs it after linking. CI job summaries read the same
                      report through stats.collect_stats().

Stats kinds. Misses carry a reason so legitimate misses can be told apart
from cache-design problems (a per-input index of recent keys under
$MOCCACHE_DIR/index makes the comparison possible). --show-stats prints each
kind's count with the explanation from stats._KIND_DESCRIPTIONS; in short:
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
