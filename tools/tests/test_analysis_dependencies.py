"""Use real Clang preprocessing to exercise transitive and conditional includes."""

import os
import shutil
from pathlib import Path
from subprocess import CompletedProcess
from unittest.mock import patch

import pytest
from analyzers.dependencies import header_dependents


def test_header_scan_tracks_transitive_macro_includes_and_updates(tmp_path, monkeypatch):
    scanner = shutil.which("clang-scan-deps-18") or shutil.which("clang-scan-deps")
    if not scanner:
        pytest.skip("Clang dependency scanner unavailable")
    tool_dir = Path(scanner).resolve().parent
    compiler = tool_dir / "clang++"
    if not compiler.is_file():
        pytest.skip("Matching Clang compiler unavailable")
    monkeypatch.setenv("PATH", f"{tool_dir}{os.pathsep}{os.environ['PATH']}")
    active, idle = tmp_path / "active.cc", tmp_path / "idle.cc"
    shared, other = tmp_path / "shared.h", tmp_path / "other.h"
    active.write_text("#include SELECTED_HEADER\nint active;\n")
    idle.write_text("int idle;\n")
    shared.write_text('#include "other.h"\n')
    other.write_text("#pragma once\n")
    entries = [
        {
            "directory": str(tmp_path),
            "file": str(source),
            "arguments": [str(compiler), '-DSELECTED_HEADER="shared.h"', "-c", str(source)],
        }
        for source in (active, idle)
    ]
    assert header_dependents(entries, {other}, 2) == {active}
    shared.write_text("#pragma once\n")
    assert header_dependents(entries, {other}, 2) == set()


@pytest.mark.parametrize(
    "output",
    ["invalid", "[]", '{"translation-units": []}', '{"modules": [{}], "translation-units": []}'],
)
def test_incomplete_dependency_scan_requires_full_analysis(tmp_path, output):
    entries = [{"directory": str(tmp_path), "file": "source.cc"}]
    with (
        patch("analyzers.dependencies.shutil.which", return_value="clang-scan-deps"),
        patch(
            "analyzers.dependencies.run_captured", return_value=CompletedProcess([], 0, output, "")
        ),
    ):
        assert header_dependents(entries, {tmp_path / "changed.h"}, 1) is None


def test_missing_scanner_requires_full_analysis():
    with patch("analyzers.dependencies.shutil.which", return_value=None):
        assert header_dependents([], set(), 1) is None
