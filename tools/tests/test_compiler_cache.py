"""Tests for qgc_tools/compiler_cache.py."""

from __future__ import annotations

import json
import subprocess
from unittest.mock import patch

from qgc_tools import compiler_cache as mod


def test_ccache_facts() -> None:
    stats = {
        "direct_cache_hit": 100,
        "preprocessed_cache_hit": 20,
        "cache_miss": 30,
        "cache_size_kibibyte": 491520,
        "max_cache_size_kibibyte": 2097152,
        "cleanups_performed": 3,
    }
    assert mod.ccache_facts(stats) == [
        ("Compiler cache", "ccache: 120 of 150 (80.0%) hits (100 direct, 20 preprocessed)"),
        ("Compiler cache size", "480 MiB of 2.0 GiB, 3 LRU cleanups"),
        ("Compiler cache errors", None),
    ]


def test_ccache_facts_tolerate_missing_fields_and_report_errors() -> None:
    facts = dict(mod.ccache_facts({"missing_cache_file": 2, "internal_error": 1}))
    assert facts["Compiler cache"] == "ccache: 0 of 0 hits (0 direct, 0 preprocessed)"
    assert facts["Compiler cache size"] == "0 MiB, 0 LRU cleanups"
    assert facts["Compiler cache errors"] == "⚠️ 3"


def test_sccache_facts() -> None:
    report = {
        "stats": {
            "requests_not_cacheable": 3,
            "cache_hits": {"counts": {"C/C++": 1900, "CUDA": 0}},
            "cache_misses": {"counts": {"C/C++": 100}},
            "cache_errors": {"counts": {"C/C++": 2}},
            "cache_writes": 100,
            "cache_write_errors": 1,
        },
        "cache_location": "S3, name: runs-on-cache",
    }
    assert mod.sccache_facts(report) == [
        ("Compiler cache", "sccache: 1,900 of 2,000 (95.0%) hits, 100 writes, 3 not cacheable"),
        ("Compiler cache location", "S3, name: runs-on-cache"),
        ("Compiler cache errors", "⚠️ 3"),
    ]


def test_stats_readers_tolerate_missing_tools_and_bad_output() -> None:
    with patch.object(mod.shutil, "which", return_value=None):
        assert mod.ccache_stats() is None
        assert mod.sccache_stats() is None

    def run(command: list[str]) -> subprocess.CompletedProcess[str]:
        output = {"ccache": json.dumps({"cache_miss": 4}), "sccache": "not json"}
        return subprocess.CompletedProcess(command, 0, output[command[0].rsplit("/", 1)[-1]], "")

    with (
        patch.object(mod.shutil, "which", side_effect=lambda tool: f"/usr/bin/{tool}"),
        patch.object(mod, "run_captured", run),
    ):
        assert mod.ccache_stats() == {"cache_miss": 4}
        assert mod.sccache_stats() is None
