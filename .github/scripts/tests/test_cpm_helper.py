"""Tests for cpm_helper.py."""

from __future__ import annotations

import os
from unittest.mock import patch

from cpm_helper import compute_cpm_fingerprint, configure_cpm_cache


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


class TestConfigureCache:
    def test_relative_cache_exports_absolute_cmake_and_relative_archive_paths(
        self, monkeypatch, tmp_path
    ):
        workspace = tmp_path / "workspace"
        github_env = tmp_path / "env.txt"
        monkeypatch.setenv("GITHUB_WORKSPACE", str(workspace))
        monkeypatch.setenv("GITHUB_ENV", str(github_env))
        configured = configure_cpm_cache(".cache/CPM")
        assert configured == workspace / ".cache/CPM"
        assert configured.is_dir()
        assert f"CPM_SOURCE_CACHE={configured.as_posix()}\n" in github_env.read_text()
        assert "CPM_CACHE_PATH=.cache/CPM\n" in github_env.read_text()

    def test_configure_cpm_cache_writes_outputs(self, tmp_path):
        github_env = tmp_path / "env.txt"
        github_output = tmp_path / "output.txt"
        cache = tmp_path / "cpm-cache"
        with patch.dict(
            os.environ, {"GITHUB_ENV": str(github_env), "GITHUB_OUTPUT": str(github_output)}
        ):
            configured = configure_cpm_cache(str(cache))
        assert configured == cache
        assert "CPM_SOURCE_CACHE=" in github_env.read_text()
        assert "path=" in github_output.read_text()


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
    from cpm_helper import seed_cache

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
    from cpm_helper import seed_cache

    root, seed = _seed(tmp_path)
    (root / "CMakeLists.txt").write_text("project(changed)\n")
    destination = tmp_path / "cache"
    assert not seed_cache(root, destination, seed)
    assert not destination.exists()


def test_interrupted_seed_copy_leaves_no_partial_cache(tmp_path, monkeypatch):
    import cpm_helper
    import pytest

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


def test_seed_creation_with_real_cmake_configuration(tmp_path):
    import json
    import shutil
    import subprocess
    from pathlib import Path

    from cpm_helper import create_seed

    root = tmp_path / "repo"
    root.mkdir()
    subprocess.run(["git", "init", str(root)], check=True, capture_output=True)
    (root / "CMakeLists.txt").write_text(
        'cmake_minimum_required(VERSION 3.25)\nproject(Seed NONE)\nfile(MAKE_DIRECTORY "$ENV{CPM_SOURCE_CACHE}/package")\nfile(WRITE "$ENV{CPM_SOURCE_CACHE}/package/source" "downloaded source")\n'
    )
    (root / "CMakePresets.json").write_text(
        json.dumps({"version": 6, "configurePresets": [{"name": "Linux", "generator": "Ninja"}]})
    )
    subprocess.run(["git", "-C", str(root), "add", "."], check=True)
    subprocess.run(
        [
            "git",
            "-C",
            str(root),
            "-c",
            "user.name=Test",
            "-c",
            "user.email=test@example.com",
            "commit",
            "-m",
            "fixture",
        ],
        check=True,
        capture_output=True,
    )
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
