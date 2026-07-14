#!/usr/bin/env python3
"""End-to-end contracts for the vehicle null-check analyzer."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from ._helpers import TOOLS_DIR

FIXTURES_DIR = Path(__file__).parent / "fixtures"
ANALYZER = TOOLS_DIR / "analyzers" / "vehicle_null_check.py"


def _run(*args: Path | str, json_output: bool = False) -> subprocess.CompletedProcess[str]:
    command = [sys.executable, str(ANALYZER)]
    if json_output:
        command.append("--json")
    command.extend(str(arg) for arg in args)
    return subprocess.run(command, capture_output=True, text=True, check=False)


def test_fixture_reports_each_unsafe_pattern_in_structured_output() -> None:
    result = _run(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
    violations = json.loads(result.stdout)
    assert result.returncode == 1
    assert {violation["pattern"] for violation in violations} >= {
        "unsafe_active_vehicle_direct",
        "unsafe_active_vehicle_use",
        "unsafe_get_parameter",
    }
    assert all("safeWith" not in violation["code"] for violation in violations)
    assert all("comment" not in violation["code"].lower() for violation in violations)
    assert set(violations[0]) >= {"file", "line", "column", "pattern", "code", "suggestion"}


def test_safe_file_help_and_directory_modes(tmp_path: Path) -> None:
    safe = tmp_path / "safe.cpp"
    safe.write_text("void f() { Vehicle *v = getVehicle(); if (!v) return; v->go(); }")
    assert _run(safe).returncode == 0
    help_result = _run("--help")
    assert help_result.returncode == 0 and "usage" in help_result.stdout.lower()

    directory_result = _run(FIXTURES_DIR, json_output=True)
    assert {Path(item["file"]).name for item in json.loads(directory_result.stdout)} == {
        "null_check_samples.cpp"
    }


def test_multiple_input_files_are_analyzed(tmp_path: Path) -> None:
    files = [tmp_path / "a.cpp", tmp_path / "b.cpp"]
    for path in files:
        path.write_text("void f() { activeVehicle()->test(); }")
    result = _run(*files, json_output=True)
    assert len({item["file"] for item in json.loads(result.stdout)}) == 2
