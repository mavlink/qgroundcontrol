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
    def test_configure_cpm_cache_writes_outputs(self, tmp_path):
        github_env = tmp_path / "env.txt"
        github_output = tmp_path / "output.txt"
        cache = tmp_path / "cpm-cache"
        with patch.dict(os.environ, {"GITHUB_ENV": str(github_env), "GITHUB_OUTPUT": str(github_output)}):
            configured = configure_cpm_cache(str(cache))
        assert configured == cache
        assert "CPM_SOURCE_CACHE=" in github_env.read_text()
        assert "path=" in github_output.read_text()
