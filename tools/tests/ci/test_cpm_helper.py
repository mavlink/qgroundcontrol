"""Tests for cpm_helper.py."""

from __future__ import annotations

import json
import sys

import pytest
from ci.cache.cpm_helper import (
    compute_cpm_fingerprint,
    compute_gstreamer_fingerprint,
    prepare_cpm_cache,
)

from .._helpers import commit_files, git

pytestmark = pytest.mark.windows


class TestFingerprint:
    def test_fingerprint_changes_when_dependency_file_changes(self, tmp_path):
        (tmp_path / "cmake/modules").mkdir(parents=True)
        (tmp_path / ".github").mkdir()
        (tmp_path / "CMakeLists.txt").write_text("project(QGC)\nCPMAddPackage(NAME foo)\n")
        (tmp_path / "cmake/modules/CPM.cmake").write_text("# helper\n")
        (tmp_path / ".github/build-config.json").write_text("{}\n")

        before = compute_cpm_fingerprint(tmp_path)
        (tmp_path / "CMakeLists.txt").write_text("project(QGC)\nCPMAddPackage(NAME bar)\n")
        after = compute_cpm_fingerprint(tmp_path)

        assert before != after


def test_gstreamer_fingerprint_follows_only_the_gstreamer_section(tmp_path):
    config = tmp_path / ".github/build-config.json"
    config.parent.mkdir()

    def fingerprint(qt: str, android: str) -> str:
        sections = {"qt": {"version": qt}, "gstreamer": {"version": {"android": android}}}
        config.write_text(json.dumps(sections))
        return compute_gstreamer_fingerprint(tmp_path)

    before = fingerprint("6.10", "1.28.4")
    assert fingerprint("6.11", "1.28.4") == before
    assert fingerprint("6.11", "1.28.5") != before


class TestPrepareCache:
    def test_relative_cache_uses_absolute_cmake_and_relative_archive_paths(
        self, monkeypatch, tmp_path
    ):
        workspace = tmp_path / "workspace"
        monkeypatch.setenv("GITHUB_WORKSPACE", str(workspace))
        environment = prepare_cpm_cache(".cache/CPM", workspace)
        cache = (workspace / ".cache/CPM").resolve()
        assert cache.is_dir()
        assert environment == {
            "CPM_SOURCE_CACHE": cache.as_posix(),
            "CPM_CACHE_PATH": ".cache/CPM",
        }

    def test_external_cache_keeps_its_absolute_archive_path(self, monkeypatch, tmp_path):
        monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path / "workspace"))
        cache = (tmp_path / "cpm-cache").resolve()
        environment = prepare_cpm_cache(str(cache), tmp_path / "workspace")
        assert cache.is_dir()
        assert environment == {
            "CPM_SOURCE_CACHE": cache.as_posix(),
            "CPM_CACHE_PATH": cache.as_posix(),
        }


def test_fingerprint_includes_dependency_defaults_and_patches(tmp_path):
    (tmp_path / "cmake").mkdir()
    (tmp_path / "src").mkdir()
    options = tmp_path / "cmake/CustomOptions.cmake"
    options.write_text('set(QGC_MAVLINK_GIT_TAG "one")\n')
    before = compute_cpm_fingerprint(tmp_path)
    options.write_text('set(QGC_MAVLINK_GIT_TAG "two")\n')
    changed = compute_cpm_fingerprint(tmp_path)
    assert before != changed
    (tmp_path / "src/dependency.patch").write_text("a patch\n")
    assert changed != compute_cpm_fingerprint(tmp_path)


def test_fingerprint_ignores_build_scripts_without_dependency_declarations(tmp_path):
    (tmp_path / "cmake").mkdir()
    (tmp_path / "src/Lib").mkdir(parents=True)
    helpers = tmp_path / "cmake/Helpers.cmake"
    sources = tmp_path / "src/CMakeLists.txt"
    declaring = tmp_path / "src/Lib/Lib.cmake"
    helpers.write_text("# Call after CPMAddPackage() with the target name\n")
    sources.write_text("target_sources(QGC PRIVATE a.cc)\n")
    declaring.write_text("CPMAddPackage(NAME lib VERSION 1)\n")
    before = compute_cpm_fingerprint(tmp_path)

    helpers.write_text("# Call after CPMAddPackage() with the library target name\n")
    sources.write_text("target_sources(QGC PRIVATE a.cc b.cc)\n")
    assert compute_cpm_fingerprint(tmp_path) == before

    declaring.write_text("CPMAddPackage(NAME lib VERSION 2)\n")
    assert compute_cpm_fingerprint(tmp_path) != before


def _seed(tmp_path):
    import json

    root = tmp_path / "repo"
    root.mkdir()
    (root / "CMakeLists.txt").write_text("project(test)\n")
    seed = tmp_path / "seed"
    (seed / "sources/package").mkdir(parents=True)
    (seed / "sources/package/file").write_text("cached source")
    (seed / "manifest.json").write_text(
        json.dumps(
            {
                "schema": 1,
                "fingerprint": compute_cpm_fingerprint(root),
                "bytes": 13,
                "prepare_seconds": 30,
            }
        )
    )
    return root, seed


def test_seed_copies_into_writable_cache_without_modifying_image(tmp_path):
    from ci.cache.cpm_helper import seed_cache

    root, seed = _seed(tmp_path)
    original = seed / "sources/package/file"
    original.chmod(0o444)
    destination = tmp_path / "workspace/.cache/CPM"
    assert seed_cache(root, destination, seed)
    copy = destination / "package/file"
    assert copy.stat().st_mode & 0o200
    copy.write_text("updated dependency")
    assert original.read_text() == "cached source"
    assert not seed_cache(root, destination, seed)
    assert copy.read_text() == "updated dependency"


def test_changed_dependencies_skip_seed(tmp_path):
    from ci.cache.cpm_helper import seed_cache

    root, seed = _seed(tmp_path)
    (root / "CMakeLists.txt").write_text("project(changed)\n")
    destination = tmp_path / "cache"
    assert not seed_cache(root, destination, seed)
    assert not destination.exists()


def test_interrupted_seed_copy_leaves_no_partial_cache(tmp_path, monkeypatch):
    import pytest
    from ci.cache import cpm_helper

    root, seed = _seed(tmp_path)
    destination = tmp_path / "cache"

    def fail(source, target, **kwargs):
        target.mkdir()
        (target / "partial").write_text("partial")
        raise OSError("disk full")

    monkeypatch.setattr(cpm_helper.shutil, "copytree", fail)
    with pytest.raises(OSError, match="disk full"):
        cpm_helper.seed_cache(root, destination, seed)
    assert not destination.exists()
    assert not list(tmp_path.glob(".qgc-cpm-copy-*"))


@pytest.mark.skipif(sys.platform == "win32", reason="runner images build CPM seeds on Linux")
@pytest.mark.usefixtures("isolated_git")
def test_seed_creation_with_real_cmake_configuration(tmp_path):
    import json
    import shutil
    from pathlib import Path

    from ci.cache.cpm_helper import create_seed

    root = tmp_path / "repo"
    root.mkdir()
    git(root, "init", "-q")
    presets = {"version": 6, "configurePresets": [{"name": "Linux", "generator": "Ninja"}]}
    files = {
        "CMakeLists.txt": 'cmake_minimum_required(VERSION 3.25)\nproject(Seed NONE)\nfile(MAKE_DIRECTORY "$ENV{CPM_SOURCE_CACHE}/package")\nfile(WRITE "$ENV{CPM_SOURCE_CACHE}/package/source" "downloaded source")\n',
        "CMakePresets.json": json.dumps(presets),
    }
    commit_files(root, files, "fixture")
    qt = tmp_path / "qt"
    (qt / "bin").mkdir(parents=True)
    cmake = shutil.which("cmake")
    assert cmake
    (qt / "bin/qt-cmake").symlink_to(Path(cmake).resolve())
    seed = tmp_path / "seed"
    create_seed(root, qt, seed)
    manifest = json.loads((seed / "manifest.json").read_text())
    assert manifest["fingerprint"] == compute_cpm_fingerprint(root)
    assert len(manifest["source_commit"]) == 40
    assert manifest["bytes"] == len("downloaded source")
