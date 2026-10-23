"""Compiler-cache (ccache, sccache) statistics for CI build summaries and cache-save gates."""

from __future__ import annotations

import json
import shutil
from typing import Any

from common.format import format_count, format_ratio
from common.proc import run_captured

Fact = tuple[str, object]

_CCACHE_ERRORS = (
    "internal_error",
    "compiler_check_failed",
    "error_hashing_extra_file",
    "missing_cache_file",
    "preprocessor_error",
)


def tool_output(tool: str, *args: str) -> str | None:
    """Run ``tool <args>`` and return its output, or None when the tool is missing or fails."""
    binary = shutil.which(tool)
    if not binary:
        return None
    try:
        result = run_captured([binary, *args])
    except OSError:
        return None
    return result.stdout.strip() if result.returncode == 0 else None


def _json_object(text: str | None) -> dict[str, Any] | None:
    try:
        value = json.loads(text or "null")
    except json.JSONDecodeError:
        return None
    return value if isinstance(value, dict) else None


def ccache_stats() -> dict[str, Any] | None:
    """Return ``ccache --print-stats --format=json``; None when ccache is missing or < 4.10."""
    return _json_object(tool_output("ccache", "--print-stats", "--format=json"))


def sccache_stats() -> dict[str, Any] | None:
    """Return ``sccache --show-stats --stats-format json``; None when sccache is unavailable."""
    return _json_object(tool_output("sccache", "--show-stats", "--stats-format", "json"))


def _gib(kib: int) -> str:
    return f"{kib / 1048576:.1f} GiB" if kib >= 1048576 else f"{kib / 1024:.0f} MiB"


def ccache_facts(stats: dict[str, Any]) -> list[Fact]:
    """Summarize ccache statistics as job-summary facts."""
    direct = int(stats.get("direct_cache_hit", 0))
    preprocessed = int(stats.get("preprocessed_cache_hit", 0))
    misses = int(stats.get("cache_miss", 0))
    size_kib = int(stats.get("cache_size_kibibyte", 0))
    max_kib = int(stats.get("max_cache_size_kibibyte", 0))
    errors = sum(int(stats.get(key, 0)) for key in _CCACHE_ERRORS)
    cleanups = int(stats.get("cleanups_performed", 0))
    size = f"{_gib(size_kib)} of {_gib(max_kib)}" if max_kib else _gib(size_kib)
    return [
        (
            "Compiler cache",
            f"ccache: {format_ratio(direct + preprocessed, direct + preprocessed + misses)} hits "
            f"({format_count(direct)} direct, {format_count(preprocessed)} preprocessed)",
        ),
        (
            "Compiler cache size",
            f"{size}, {format_count(cleanups)} LRU cleanup{'' if cleanups == 1 else 's'}",
        ),
        ("Compiler cache errors", f"⚠️ {format_count(errors)}" if errors else None),
    ]


def sccache_facts(report: dict[str, Any]) -> list[Fact]:
    """Summarize ``sccache --show-stats --stats-format json`` as job-summary facts."""
    stats = report.get("stats") or {}

    def total(name: str) -> int:
        return sum(int(count) for count in ((stats.get(name) or {}).get("counts") or {}).values())

    hits = total("cache_hits")
    misses = total("cache_misses")
    errors = total("cache_errors") + sum(
        int(stats.get(key, 0))
        for key in ("cache_read_errors", "cache_write_errors", "cache_timeouts")
    )
    return [
        (
            "Compiler cache",
            f"sccache: {format_ratio(hits, hits + misses)} hits, "
            f"{format_count(int(stats.get('cache_writes', 0)))} writes, "
            f"{format_count(int(stats.get('requests_not_cacheable', 0)))} not cacheable",
        ),
        ("Compiler cache location", report.get("cache_location") or None),
        ("Compiler cache errors", f"⚠️ {format_count(errors)}" if errors else None),
    ]
