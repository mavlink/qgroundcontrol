"""Ccache installation, configuration, reporting, and cache-scope contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING

import ccache_helper as mod
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def test_version_and_architecture_validation(monkeypatch: pytest.MonkeyPatch) -> None:
    for version, valid in (
        ("4.13.1", True),
        ("4.13", True),
        ("v4.13.1", False),
        ("4.13.1.2", False),
        ("latest", False),
    ):
        assert mod.CcacheInstaller.validate_version(version) is valid

    monkeypatch.delenv("RUNNER_ARCH", raising=False)
    for machine, expected in (
        ("x86_64", "x86_64"),
        ("amd64", "x86_64"),
        ("arm64", "aarch64"),
        ("aarch64", "aarch64"),
    ):
        monkeypatch.setattr("platform.machine", lambda machine=machine: machine)
        assert mod.CcacheInstaller.detect_arch() == expected


def test_installer_reads_configuration_and_prefix_override(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    config_path = tmp_path / "ccache.conf"
    config_path.write_text("max_size = 5G\ncompiler_check = content\n")
    monkeypatch.setenv("CCACHE_PREFIX", str(tmp_path / "prefix"))
    config = mod.CcacheInstaller(
        version="4.13.1", arch="aarch64", config_path=config_path
    ).get_config()
    assert (config.version, config.arch, config.max_size) == ("4.13.1", "aarch64", "5G")
    assert mod.CcacheInstaller._default_prefix() == tmp_path / "prefix"


def test_summary_reports_hits_capacity_cleanup_errors_and_empty_stats() -> None:
    markdown = mod.build_summary_markdown(
        {
            "direct_cache_hit": 100,
            "preprocessed_cache_hit": 20,
            "cache_miss": 30,
            "cache_size_kibibyte": 491520,
            "max_cache_size_kibibyte": 2097152,
            "cleanups_performed": 3,
            "missing_cache_file": 2,
            "internal_error": 1,
        }
    )
    for expected in (
        "120 / 150 (80.0%)",
        "480 MiB / 2.0 GiB (23.4%)",
        "Cleanups (LRU evictions) | 3",
        "⚠ Errors | 3",
    ):
        assert expected in markdown
    assert "0 / 0 (0.0%)" in mod.build_summary_markdown({})


def test_cli_arch_resolution_and_invalid_invocations(capsys: pytest.CaptureFixture[str]) -> None:
    cases = [
        (["config", "--arch", "aarch64"], "aarch64"),
        (["config", "--target", "linux_gcc_arm64"], "aarch64"),
        (["install", "--target", "linux_gcc_arm64"], "aarch64"),
        (["config", "--target", "linux_gcc_64"], None),
        (["config"], None),
    ]
    for arguments, expected in cases:
        assert mod.resolve_arch(mod.parse_args(arguments)) == expected
    with pytest.raises(SystemExit):
        mod.parse_args(["config", "--arch", "x86_64", "--target", "linux_gcc_arm64"])

    assert mod.main([]) == 1
    assert mod.main(["config", "--version", "bad"]) == 1
    assert mod.main(["config", "--version", "4.13.1", "--target", "linux_gcc_arm64"]) == 0
    assert "aarch64" in capsys.readouterr().out


def test_summary_cli_writes_step_summary(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    summary_file = tmp_path / "summary.md"
    monkeypatch.setattr(
        mod,
        "get_ccache_json_stats",
        lambda: {"direct_cache_hit": 10, "preprocessed_cache_hit": 2, "cache_miss": 5},
    )
    monkeypatch.setattr(mod, "get_ccache_compression_stats", lambda: "Compression ratio: 4.6 x")
    monkeypatch.setattr(mod, "get_ccache_verbose_stats", lambda: "cache hit (direct): 10")
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary_file))
    assert mod.main(["summary"]) == 0
    content = summary_file.read_text()
    assert "12 / 17" in content
    assert "cache hit (direct): 10" in content
    assert "Compression ratio: 4.6 x" in content


def test_scope_target_and_environment_helpers(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    assert mod.determine_cache_scope("pull_request", "feature/test", "42") == "pr-42"
    assert mod.determine_cache_scope("push", "feature/test") == "branch-feature-test"
    assert mod.resolve_windows_binary_config("windows_arm64", "windows")["arch"] == "aarch64"
    assert mod.resolve_windows_binary_config("windows", "android")["arch"] == "x86_64"

    github_env = tmp_path / "env.txt"
    workspace = tmp_path / "workspace"
    monkeypatch.setenv("GITHUB_ENV", str(github_env))
    assert mod.configure_ccache_environment(workspace) == workspace / ".ccache"
    content = github_env.read_text()
    assert "CCACHE_DIR=" in content and "CCACHE_CONFIGPATH=" in content
