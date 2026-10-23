"""Use real Clang preprocessing to exercise transitive and conditional includes."""

import os
import shutil
from pathlib import Path
from subprocess import CompletedProcess
from unittest.mock import patch

import pytest
from analyzers.dependencies import DependencyScanError, scan_dependencies


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
    dependencies = scan_dependencies(entries, 2)
    assert other.resolve() in dependencies[active.resolve()]
    assert other.resolve() not in dependencies[idle.resolve()]
    shared.write_text("#pragma once\n")
    assert other.resolve() not in scan_dependencies(entries, 2)[active.resolve()]


@pytest.mark.parametrize(
    "output",
    ["invalid", "[]", '{"translation-units": []}', '{"modules": [{}], "translation-units": []}'],
)
def test_incomplete_dependency_scan_requires_full_analysis(tmp_path, output):
    entries = [{"directory": str(tmp_path), "file": "source.cc"}]
    with (
        patch("common.proc.shutil.which", return_value="clang-scan-deps"),
        patch(
            "analyzers.dependencies.run_captured", return_value=CompletedProcess([], 0, output, "")
        ),
        pytest.raises(DependencyScanError, match="incomplete"),
    ):
        scan_dependencies(entries, 1)


def test_missing_scanner_requires_full_analysis():
    with (
        patch("common.proc.shutil.which", return_value=None),
        pytest.raises(DependencyScanError, match="clang-scan-deps not found on PATH"),
    ):
        scan_dependencies([], 1)
