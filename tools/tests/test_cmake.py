"""Contracts for shared CMake metadata parsing."""

from __future__ import annotations

from common.cmake import read_cache_dict, read_cache_var


def test_cache_parser_matches_complete_names_and_types(tmp_path) -> None:
    cache = tmp_path / "CMakeCache.txt"
    cache.write_text(
        "CMAKE_BUILD_TYPE:STRING=Release\n"
        "QGC_COVERAGE_LINE_THRESHOLD:STRING=42\n"
        "QGC_ENABLE_GST:BOOL=ON\n"
    )

    assert read_cache_dict(cache)["CMAKE_BUILD_TYPE"] == "Release"
    assert read_cache_var(cache, "QGC_COVERAGE_LINE_THRESHOLD") == "42"
    assert read_cache_var(cache, "QGC_ENABLE_GST") == "ON"
    for name in ("ABSENT_VAR", "QGC_COVERAGE_LINE"):
        assert read_cache_var(cache, name) is None
    assert read_cache_var(tmp_path / "missing", "X") is None
