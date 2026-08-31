"""Contracts for CPM source caches derived from external patch contents."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path


MODULE_PATH = REPO_ROOT / "cmake/modules/CPMPatchCache.cmake"


def _cache_key(
    tmp_path: Path,
    patch_contents: str,
    *,
    git_repository: str = "https://example.invalid/dependency.git",
    git_tag: str = "0123456789abcdef",
) -> str:
    patch_path = tmp_path / "dependency.patch"
    result_path = tmp_path / "cache-key.txt"
    script_path = tmp_path / "cache-key-contract.cmake"
    patch_path.write_text(patch_contents, encoding="utf-8")
    script_path.write_text(
        f'include("{MODULE_PATH.as_posix()}")\n'
        "qgc_cpm_patch_cache_key(\n"
        "    cache_key\n"
        f'    GIT_REPOSITORY "{git_repository}"\n'
        f'    GIT_TAG "{git_tag}"\n'
        f'    PATCHES "{patch_path.as_posix()}"\n'
        ")\n"
        f'file(WRITE "{result_path.as_posix()}" "${{cache_key}}")\n',
        encoding="utf-8",
    )

    subprocess.run(["cmake", "-P", str(script_path)], check=True, capture_output=True, text=True)
    return result_path.read_text(encoding="utf-8")


def test_patch_contents_are_part_of_cpm_cache_key(tmp_path: Path) -> None:
    initial_key = _cache_key(tmp_path, "first patch contents\n")

    assert len(initial_key) == 64
    assert set(initial_key) <= set("0123456789abcdef")
    assert initial_key == _cache_key(tmp_path, "first patch contents\n")
    assert initial_key != _cache_key(tmp_path, "different patch contents\n")
    assert initial_key != _cache_key(tmp_path, "first patch contents\n", git_tag="fedcba9876543210")
    assert initial_key != _cache_key(
        tmp_path,
        "first patch contents\n",
        git_repository="https://example.invalid/other-dependency.git",
    )


def test_patch_change_triggers_cmake_reconfigure(tmp_path: Path) -> None:
    source_path = tmp_path / "source"
    build_path = tmp_path / "build"
    patch_path = source_path / "dependency.patch"
    result_path = build_path / "cache-key.txt"
    source_path.mkdir()
    patch_path.write_text("first patch contents\n", encoding="utf-8")
    (source_path / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        "project(cache_key_contract NONE)\n"
        f'include("{MODULE_PATH.as_posix()}")\n'
        "qgc_cpm_patch_cache_key(\n"
        "    cache_key\n"
        '    GIT_REPOSITORY "https://example.invalid/dependency.git"\n'
        '    GIT_TAG "0123456789abcdef"\n'
        '    PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/dependency.patch"\n'
        ")\n"
        'file(WRITE "${CMAKE_BINARY_DIR}/cache-key.txt" "${cache_key}")\n',
        encoding="utf-8",
    )

    subprocess.run(
        ["cmake", "-S", str(source_path), "-B", str(build_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    initial_key = result_path.read_text(encoding="utf-8")

    patch_path.write_text("different patch contents\n", encoding="utf-8")
    subprocess.run(
        ["cmake", "--build", str(build_path)], check=True, capture_output=True, text=True
    )

    assert result_path.read_text(encoding="utf-8") != initial_key
