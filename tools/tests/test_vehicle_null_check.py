#!/usr/bin/env python3
"""Tests for vehicle_null_check.py analyzer."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import pytest

from ._helpers import TOOLS_DIR

FIXTURES_DIR = Path(__file__).parent / "fixtures"
ANALYZER = TOOLS_DIR / "analyzers" / "vehicle_null_check.py"


def run_analyzer(*args: Path | str, json_output: bool = False) -> subprocess.CompletedProcess:
    cmd = [sys.executable, str(ANALYZER)]
    if json_output:
        cmd.append("--json")
    cmd.extend(str(a) for a in args)
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


@pytest.fixture(scope="module")
def sample_violations() -> list[dict]:
    result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
    return json.loads(result.stdout)


def test_detects_unsafe_direct_access(sample_violations: list[dict]) -> None:
    assert [v for v in sample_violations if v["pattern"] == "unsafe_active_vehicle_direct"]


def test_detects_unsafe_variable_use(sample_violations: list[dict]) -> None:
    assert [v for v in sample_violations if v["pattern"] == "unsafe_active_vehicle_use"]


def test_detects_unsafe_get_parameter(sample_violations: list[dict]) -> None:
    assert [v for v in sample_violations if v["pattern"] == "unsafe_get_parameter"]


def test_ignores_safe_patterns(sample_violations: list[dict]) -> None:
    assert all("safeWith" not in v["code"] for v in sample_violations)


def test_ignores_comments(sample_violations: list[dict]) -> None:
    assert all("comment" not in v["code"].lower() for v in sample_violations)


def test_json_output_format(sample_violations: list[dict]) -> None:
    assert sample_violations, "analyzer should report violations for the fixture"
    for key in ("file", "line", "column", "pattern", "code", "suggestion"):
        assert key in sample_violations[0]


def test_exit_code_on_violations() -> None:
    assert run_analyzer(FIXTURES_DIR / "null_check_samples.cpp").returncode == 1


def test_exit_code_no_violations(tmp_path: Path) -> None:
    safe_file = tmp_path / "safe.cpp"
    safe_file.write_text(
        "void safeFunction() {\n"
        "    Vehicle *v = getVehicle();\n"
        "    if (!v) return;\n"
        "    v->doSomething();\n"
        "}\n"
    )
    assert run_analyzer(safe_file).returncode == 0


def test_help_flag() -> None:
    result = run_analyzer("--help")
    assert result.returncode == 0
    assert "usage" in result.stdout.lower()


def test_analyze_directory_finds_fixture_violations() -> None:
    result = run_analyzer(FIXTURES_DIR, json_output=True)
    violations = json.loads(result.stdout)
    assert violations, "directory scan should surface the known fixture violations"
    assert {Path(v["file"]).name for v in violations} == {"null_check_samples.cpp"}


def test_multiple_files(tmp_path: Path) -> None:
    f1 = tmp_path / "a.cpp"
    f2 = tmp_path / "b.cpp"
    f1.write_text("void f() { activeVehicle()->test(); }")
    f2.write_text("void g() { activeVehicle()->test(); }")

    result = run_analyzer(f1, f2, json_output=True)
    violations = json.loads(result.stdout)
    assert len({v["file"] for v in violations}) == 2
