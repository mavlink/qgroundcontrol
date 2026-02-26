"""Tests for benchmark_runner.py."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path
from unittest.mock import patch

from benchmark_runner import (
    fallback_startup_benchmark,
    parse_benchmark_output,
    run_benchmarks,
    write_github_output,
)


def test_parse_benchmark_output_unit_conversion() -> None:
    output = "\n".join(
        [
            "RESULT : Foo::msecsCase(): 1.5 msecs per iteration",
            "RESULT : Foo::secsCase(): 2 secs per iteration",
            "RESULT : Foo::nsecsCase(): 5000 nsecs per iteration",
            "RESULT : Foo::usecsCase(): 250 usecs per iteration",
        ],
    )

    results = parse_benchmark_output(output)
    by_name = {entry["name"]: entry for entry in results}

    assert by_name["msecsCase"]["value"] == 1500.0
    assert by_name["secsCase"]["value"] == 2000000.0
    assert by_name["nsecsCase"]["value"] == 5.0
    assert by_name["usecsCase"]["value"] == 250.0
    assert all(entry["unit"] == "usecs" for entry in results)


def test_run_benchmarks_missing_binary_exits(tmp_path: Path) -> None:
    missing = tmp_path / "does-not-exist"
    try:
        run_benchmarks(missing, "MAVLinkBenchmark", "offscreen")
        assert False, "Expected SystemExit"
    except SystemExit as exc:
        assert exc.code == 1


def test_run_benchmarks_oserror_returns_empty(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_text("fake", encoding="utf-8")

    with patch("benchmark_runner.subprocess.run", side_effect=OSError("boom")):
        output = run_benchmarks(binary, "MAVLinkBenchmark", "offscreen")
    assert output == ""


def test_fallback_startup_timeout_returns_empty(tmp_path: Path) -> None:
    binary = tmp_path / "QGroundControl"
    binary.write_text("fake", encoding="utf-8")

    with patch(
        "benchmark_runner.subprocess.run",
        side_effect=subprocess.TimeoutExpired(cmd="qgc", timeout=30),
    ):
        results = fallback_startup_benchmark(binary, "offscreen")
    assert results == []


def test_write_github_output(tmp_path: Path) -> None:
    output_file = tmp_path / "github_output.txt"
    with patch.dict(os.environ, {"GITHUB_OUTPUT": str(output_file)}):
        write_github_output([{"name": "Startup", "unit": "usecs", "value": 1.0}])

    content = output_file.read_text(encoding="utf-8")
    assert "has_results=true" in content
    assert "result_count=1" in content
