"""Contracts for CPM cache fingerprinting and CI outputs."""

from __future__ import annotations

from typing import TYPE_CHECKING

from cpm_helper import compute_cpm_fingerprint, configure_cpm_cache

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_fingerprint_changes_with_dependency_inputs(tmp_path: Path) -> None:
    (tmp_path / "cmake/modules").mkdir(parents=True)
    (tmp_path / ".github").mkdir()
    cmake = tmp_path / "CMakeLists.txt"
    cmake.write_text("project(QGC)\nCPMAddPackage(NAME foo)\n")
    (tmp_path / "cmake/modules/CPM.cmake").write_text("# helper\n")
    (tmp_path / ".github/build-config.json").write_text("{}\n")

    before = compute_cpm_fingerprint(tmp_path)
    cmake.write_text("project(QGC)\nCPMAddPackage(NAME bar)\n")
    assert compute_cpm_fingerprint(tmp_path) != before


def test_cache_configuration_writes_environment_and_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    github_env, github_output = tmp_path / "env", tmp_path / "output"
    monkeypatch.setenv("GITHUB_ENV", str(github_env))
    monkeypatch.setenv("GITHUB_OUTPUT", str(github_output))
    cache = tmp_path / "cpm-cache"
    assert configure_cpm_cache(str(cache)) == cache
    assert "CPM_SOURCE_CACHE=" in github_env.read_text()
    assert "path=" in github_output.read_text()
