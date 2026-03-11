"""Tests for ccache_helper.py."""

from __future__ import annotations

import os
import tempfile
from pathlib import Path
from unittest.mock import patch

import pytest

from ccache_helper import (
    CcacheConfig,
    CcacheInstaller,
    build_summary_markdown,
    configure_ccache_environment,
    configure_cpm_cache,
    compute_cpm_fingerprint,
    determine_cache_scope,
    main,
    resolve_arch,
    resolve_windows_binary_config,
    parse_args,
    write_step_summary,
)


class TestCcacheInstaller:
    """Tests for CcacheInstaller class."""

    def test_validate_version_valid(self):
        """Test valid version formats."""
        assert CcacheInstaller.validate_version("4.13.1")
        assert CcacheInstaller.validate_version("4.13")
        assert CcacheInstaller.validate_version("5.0.0")

    def test_validate_version_invalid(self):
        """Test invalid version formats."""
        assert not CcacheInstaller.validate_version("4.12.2.1")
        assert not CcacheInstaller.validate_version("v4.12.2")
        assert not CcacheInstaller.validate_version("latest")
        assert not CcacheInstaller.validate_version("")

    def test_detect_arch_x86_64(self):
        """Test x86_64 architecture detection."""
        with patch("platform.machine", return_value="x86_64"):
            assert CcacheInstaller.detect_arch() == "x86_64"

    def test_detect_arch_amd64(self):
        """Test amd64 (alias) architecture detection."""
        with patch("platform.machine", return_value="amd64"):
            assert CcacheInstaller.detect_arch() == "x86_64"

    def test_detect_arch_arm64(self):
        """Test arm64 architecture detection."""
        with patch("platform.machine", return_value="arm64"):
            assert CcacheInstaller.detect_arch() == "aarch64"

    def test_detect_arch_aarch64(self):
        """Test aarch64 architecture detection."""
        with patch("platform.machine", return_value="aarch64"):
            assert CcacheInstaller.detect_arch() == "aarch64"

    def test_default_prefix_from_env(self):
        """Test prefix from CCACHE_PREFIX environment variable."""
        with patch.dict(os.environ, {"CCACHE_PREFIX": "/custom/path"}):
            assert CcacheInstaller._default_prefix() == Path("/custom/path")

    def test_default_prefix_fallback(self):
        """Test default prefix fallback to /usr/local."""
        with patch.dict(os.environ, {}, clear=True):
            os.environ.pop("CCACHE_PREFIX", None)
            assert CcacheInstaller._default_prefix() == Path("/usr/local")

    def test_read_max_size_from_config(self):
        """Test reading max_size from ccache.conf."""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".conf", delete=False) as f:
            f.write("max_size = 5G\n")
            f.write("compiler_check = content\n")
            f.flush()
            config_path = Path(f.name)

        try:
            installer = CcacheInstaller(config_path=config_path)
            assert installer.max_size == "5G"
        finally:
            config_path.unlink()

    def test_read_max_size_missing_file(self):
        """Test max_size default when config file is missing."""
        installer = CcacheInstaller(config_path=Path("/nonexistent/path.conf"))
        assert installer.max_size == CcacheInstaller.DEFAULT_MAX_SIZE

    def test_get_config(self):
        """Test get_config returns correct named tuple."""
        installer = CcacheInstaller(version="4.10", arch="aarch64")
        config = installer.get_config()

        assert isinstance(config, CcacheConfig)
        assert config.version == "4.10"
        assert config.arch == "aarch64"
        assert config.max_size == CcacheInstaller.DEFAULT_MAX_SIZE

    def test_installer_with_custom_prefix(self):
        """Test installer with custom prefix."""
        custom_prefix = Path("/opt/ccache")
        installer = CcacheInstaller(prefix=custom_prefix)
        assert installer.prefix == custom_prefix


class TestCcacheConfig:
    """Tests for CcacheConfig named tuple."""

    def test_config_creation(self):
        """Test CcacheConfig creation."""
        config = CcacheConfig(version="4.13.1", arch="x86_64", max_size="2G")
        assert config.version == "4.13.1"
        assert config.arch == "x86_64"
        assert config.max_size == "2G"

    def test_config_immutable(self):
        """Test CcacheConfig is immutable."""
        config = CcacheConfig(version="4.13.1", arch="x86_64", max_size="2G")
        with pytest.raises(AttributeError):
            config.version = "5.0.0"


class TestBuildSummaryMarkdown:
    """Tests for build_summary_markdown function."""

    def test_typical_stats(self):
        stats = {
            "direct_cache_hit": 100,
            "preprocessed_cache_hit": 20,
            "cache_miss": 30,
        }
        md = build_summary_markdown(stats)
        assert "120 / 150 (80.0%)" in md
        assert "| Direct hits | 100 |" in md
        assert "| Preprocessed hits | 20 |" in md
        assert "| Misses | 30 |" in md

    def test_all_hits(self):
        stats = {"direct_cache_hit": 50, "preprocessed_cache_hit": 0, "cache_miss": 0}
        md = build_summary_markdown(stats)
        assert "50 / 50 (100.0%)" in md

    def test_all_misses(self):
        stats = {"direct_cache_hit": 0, "preprocessed_cache_hit": 0, "cache_miss": 42}
        md = build_summary_markdown(stats)
        assert "0 / 42 (0.0%)" in md

    def test_empty_stats(self):
        md = build_summary_markdown({})
        assert "0 / 0 (0.0%)" in md

    def test_missing_fields_default_to_zero(self):
        stats = {"direct_cache_hit": 5}
        md = build_summary_markdown(stats)
        assert "5 / 5 (100.0%)" in md


class TestWriteStepSummary:
    """Tests for write_step_summary function."""

    def test_writes_to_file(self, tmp_path):
        summary_file = tmp_path / "summary.md"
        with patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(summary_file)}):
            assert write_step_summary("hello\n")
        assert summary_file.read_text() == "hello\n"

    def test_appends_to_existing(self, tmp_path):
        summary_file = tmp_path / "summary.md"
        summary_file.write_text("existing\n")
        with patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(summary_file)}):
            write_step_summary("appended\n")
        assert summary_file.read_text() == "existing\nappended\n"

    def test_prints_when_no_env(self, capsys):
        with patch.dict(os.environ, {}, clear=True):
            os.environ.pop("GITHUB_STEP_SUMMARY", None)
            write_step_summary("fallback\n")
        assert "fallback" in capsys.readouterr().out


class TestResolveArch:
    """Tests for --target / --arch resolution."""

    def test_arch_flag(self):
        args = parse_args(["config", "--arch", "aarch64"])
        assert resolve_arch(args) == "aarch64"

    def test_target_arm64(self):
        args = parse_args(["config", "--target", "linux_gcc_arm64"])
        assert resolve_arch(args) == "aarch64"

    def test_target_unknown_defaults_to_none(self):
        args = parse_args(["config", "--target", "linux_gcc_64"])
        assert resolve_arch(args) is None

    def test_neither_returns_none(self):
        args = parse_args(["config"])
        assert resolve_arch(args) is None

    def test_arch_and_target_mutually_exclusive(self):
        with pytest.raises(SystemExit):
            parse_args(["config", "--arch", "x86_64", "--target", "linux_gcc_arm64"])

    def test_install_target(self):
        args = parse_args(["install", "--target", "linux_gcc_arm64"])
        assert resolve_arch(args) == "aarch64"


class TestCLI:
    """Tests for subcommand dispatch."""

    def test_no_subcommand_returns_error(self):
        assert main([]) == 1

    def test_config_invalid_version(self):
        assert main(["config", "--version", "bad"]) == 1

    def test_config_valid(self, capsys):
        assert main(["config", "--version", "4.13.1"]) == 0
        out = capsys.readouterr().out
        assert "4.13.1" in out

    def test_config_with_target(self, capsys):
        assert main(["config", "--target", "linux_gcc_arm64"]) == 0
        out = capsys.readouterr().out
        assert "aarch64" in out

    def test_summary_without_ccache(self):
        with patch("ccache_helper.get_ccache_json_stats", return_value=None), \
             patch("ccache_helper.get_ccache_verbose_stats", return_value=None):
            assert main(["summary"]) == 0

    def test_summary_with_stats(self, tmp_path):
        stats = {"direct_cache_hit": 10, "preprocessed_cache_hit": 2, "cache_miss": 5}
        verbose = "cache hit (direct): 10\ncache miss: 5"
        summary_file = tmp_path / "summary.md"
        with patch("ccache_helper.get_ccache_json_stats", return_value=stats), \
             patch("ccache_helper.get_ccache_verbose_stats", return_value=verbose), \
             patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(summary_file)}):
            assert main(["summary"]) == 0
        content = summary_file.read_text()
        assert "12 / 17" in content
        assert "<details>" in content
        assert "cache hit (direct): 10" in content


class TestCacheScope:
    """Tests for workflow cache scope helper."""

    def test_pull_request_scope(self):
        assert determine_cache_scope("pull_request", "feature/test", "42") == "pr-42"

    def test_push_non_master_scope(self):
        assert determine_cache_scope("push", "feature/test") == "branch-feature-test"


class TestFingerprint:
    """Tests for CPM fingerprint helper."""

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


class TestWindowsConfig:
    """Tests for Windows ccache binary resolution."""

    def test_windows_arm64_uses_aarch64_binary(self):
        values = resolve_windows_binary_config("windows_arm64", "windows")
        assert values["arch"] == "aarch64"

    def test_android_windows_uses_x86_64_binary(self):
        values = resolve_windows_binary_config("windows", "android")
        assert values["arch"] == "x86_64"


class TestEnvironmentConfig:
    """Tests for environment configuration helpers."""

    def test_configure_ccache_environment_writes_env(self, tmp_path):
        github_env = tmp_path / "env.txt"
        workspace = tmp_path / "workspace"
        with patch.dict(os.environ, {"GITHUB_ENV": str(github_env)}):
            ccache_dir = configure_ccache_environment(workspace)
        assert ccache_dir == workspace / ".ccache"
        content = github_env.read_text()
        assert "CCACHE_DIR=" in content
        assert "CCACHE_CONFIGPATH=" in content

    def test_configure_cpm_cache_writes_outputs(self, tmp_path):
        github_env = tmp_path / "env.txt"
        github_output = tmp_path / "output.txt"
        cache = tmp_path / "cpm-cache"
        with patch.dict(os.environ, {"GITHUB_ENV": str(github_env), "GITHUB_OUTPUT": str(github_output)}):
            configured = configure_cpm_cache(str(cache))
        assert configured == cache
        assert "CPM_SOURCE_CACHE=" in github_env.read_text()
        assert "path=" in github_output.read_text()
