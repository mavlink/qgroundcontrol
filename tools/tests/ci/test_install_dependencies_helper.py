"""Tests for install_dependencies_helper.py."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
from unittest.mock import call, patch

import pytest
from ci.cache import gh_cache_cleanup
from ci.runner import install_dependencies_helper as helper

pytestmark = pytest.mark.windows

OS_RELEASE = 'NAME="Ubuntu"\nID=ubuntu\nVERSION_ID="24.04"\nID_LIKE=debian\n'


def _plan(tmp_path: Path, category: str = "", packages: list[str] | None = None) -> dict:
    os_release = tmp_path / "os-release"
    os_release.write_text(OS_RELEASE)
    with (
        patch.object(helper, "get_debian_packages", return_value=packages or ["git", "nasm"]),
        patch.object(helper, "write_github_output") as output,
    ):
        helper.plan(category, os_release=os_release)
    return output.call_args.args[0]


class TestPlan:
    def test_key_keeps_the_family_month_and_package_digest(self, tmp_path, monkeypatch):
        monkeypatch.setenv("RUNNER_ARCH", "X64")
        outputs = _plan(tmp_path)

        assert re.fullmatch(r"apt-debs-ubuntu-24\.04-X64-\d{4}-\d{2}-[0-9a-f]{64}", outputs["key"])
        month_prefix = outputs["key"].rsplit("-", 1)[0] + "-"
        assert outputs["restore-keys"].splitlines() == [month_prefix, "apt-debs-ubuntu-24.04-X64-"]

    def test_monthly_keys_share_one_cleanup_family(self, tmp_path, monkeypatch):
        """Cache cleanup must see every month's key as one family so it can evict the old month."""
        monkeypatch.setenv("RUNNER_ARCH", "X64")
        key = _plan(tmp_path)["key"]

        assert gh_cache_cleanup._cache_family(key) == "apt-debs-ubuntu-24.04-X64-<generation>"

    def test_category_gets_its_own_family(self, tmp_path, monkeypatch):
        """A category's smaller set must never restore into, or save as, the full set's cache."""
        monkeypatch.setenv("RUNNER_ARCH", "ARM64")
        outputs = _plan(tmp_path, "core")

        assert outputs["key"].startswith("apt-debs-core-ubuntu-24.04-ARM64-")
        assert all(key.startswith("apt-debs-core-") for key in outputs["restore-keys"].splitlines())

    def test_package_list_changes_the_key(self, tmp_path, monkeypatch):
        monkeypatch.setenv("RUNNER_ARCH", "X64")
        first = _plan(tmp_path, packages=["git", "nasm"])
        second = _plan(tmp_path, packages=["git", "nasm", "patchelf"])

        assert first["key"] != second["key"]
        assert first["restore-keys"] == second["restore-keys"]

    def test_unknown_category_fails_instead_of_installing_nothing(self, tmp_path):
        with (
            patch.object(helper, "write_github_output") as output,
            pytest.raises(SystemExit),
        ):
            helper.plan("bogus", os_release=tmp_path / "unused")

        output.assert_not_called()

    def test_core_category_resolves_the_real_package_list(self):
        packages = set(helper.debian_packages("core"))
        assert {"git", "patchelf"} <= packages
        assert not packages & helper.CI_PROVIDED_TOOLS

    def test_tools_ci_pins_elsewhere_are_not_installed(self):
        listed = ["cmake", "git", "ninja-build", "ccache", "mold", "nasm"]
        with patch.object(helper, "get_debian_packages", return_value=listed):
            assert helper.debian_packages("") == ["git", "nasm"]


class TestInstall:
    def test_installs_packages_then_extras_with_the_short_timeout(self):
        with (
            patch.object(helper, "get_debian_packages", return_value=["git", "nasm"]),
            patch.object(helper, "enable_universe") as universe,
            patch.object(helper.apt_install_retry, "main", return_value=0) as apt,
        ):
            assert helper.install("", ["clang"]) == 0

        universe.assert_called_once_with()
        assert apt.call_args_list == [
            call(["--update", "--no-install-recommends", "--autoclean", "git", "nasm"]),
            call(["--install-timeout", "300", "--label", "extra apt packages", "clang"]),
        ]

    def test_failed_install_skips_extras(self):
        with (
            patch.object(helper, "get_debian_packages", return_value=["git"]),
            patch.object(helper, "enable_universe"),
            patch.object(helper.apt_install_retry, "main", return_value=1) as apt,
        ):
            assert helper.install("", ["clang"]) == 1

        apt.assert_called_once()

    def test_universe_failure_stops_before_apt(self, capsys):
        with (
            patch.object(helper, "get_debian_packages", return_value=["git"]),
            patch.object(helper, "enable_universe", return_value=False),
            patch.object(helper.apt_install_retry, "main") as apt,
        ):
            assert helper.install("", []) == 1

        apt.assert_not_called()
        assert "::error::Could not enable the universe repository" in capsys.readouterr().out

    def test_invalid_extra_package_fails_before_apt(self):
        with (
            patch.object(helper, "enable_universe") as universe,
            patch.object(helper.apt_install_retry, "main") as apt,
        ):
            assert helper.install("core", ["clang;rm"]) == 1

        universe.assert_not_called()
        apt.assert_not_called()

    def test_main_reads_the_action_inputs(self, monkeypatch):
        monkeypatch.setenv("INPUT_CATEGORY", "core")
        monkeypatch.setenv("INPUT_EXTRA_PACKAGES", "clang  clang-tools")
        with patch.object(helper, "install", return_value=0) as install:
            assert helper.main(["install"]) == 0

        install.assert_called_once_with("core", ["clang", "clang-tools"])


@pytest.mark.parametrize(("flag", "arm64"), [("", False), ("true", True), ("false", False)])
def test_windows_installs_msvc_and_nsis_without_gstreamer(monkeypatch, flag, arm64):
    monkeypatch.setenv("INPUT_MSVC_ARM64", flag)
    with patch.object(helper, "install_with_retries", return_value=3) as install:
        assert helper.main(["windows"]) == 3

    command, timeout = install.call_args.args
    assert timeout == 20 * 60
    assert command[:2] == [sys.executable, str(helper.INSTALL_DEPENDENCIES)]
    assert command[2:] == [
        "--platform",
        "windows",
        "--skip-gstreamer",
        "--msvc",
        "--nsis",
        *(["--msvc-arm64"] if arm64 else []),
    ]


def test_macos_retries_single_attempts_in_child_processes():
    with patch.object(helper, "install_with_retries", return_value=0) as install:
        assert helper.main(["macos"]) == 0

    command, timeout = install.call_args.args
    assert timeout == 10 * 60
    assert command == [
        sys.executable,
        str(Path(helper.__file__).resolve()),
        "macos",
        "--single-attempt",
    ]


@pytest.mark.parametrize(("installed", "status"), [(True, 0), (False, 1)])
def test_macos_installs_homebrew_packages_ci_does_not_provide(installed, status):
    with patch.object(helper, "install_macos", return_value=installed) as install:
        assert helper.main(["macos", "--single-attempt"]) == status

    packages = install.call_args.kwargs["packages"]
    assert {"create-dmg", "pkgconf"} <= set(packages)
    assert not set(packages) & helper.CI_PROVIDED_TOOLS


def test_installs_retry_and_kill_each_hung_attempts_process_tree():
    """Three attempts, as the action's former nick-fields/retry steps made."""
    with patch.object(
        helper, "run_with_retry", return_value=subprocess.CompletedProcess([], 2)
    ) as run:
        assert helper.install_with_retries(["installer"], 600) == 2

    run.assert_called_once()
    kwargs = run.call_args.kwargs
    assert run.call_args.args == (["installer"],)
    assert (kwargs["max_attempts"], kwargs["timeout"]) == (3, 600)
    assert kwargs["kill_tree_on_timeout"] is True
    assert kwargs["check"] is False
    retry = kwargs["retry_result_if"]
    assert retry(subprocess.CompletedProcess([], 1)) and not retry(
        subprocess.CompletedProcess([], 0)
    )


def test_installs_that_always_time_out_fail(capsys):
    timeout = subprocess.TimeoutExpired(["installer"], 600)
    with patch.object(helper, "run_with_retry", side_effect=timeout):
        assert helper.install_with_retries(["installer"], 600) == 1
    assert "::error::The last of 3 install attempts timed out after 10 minutes" in (
        capsys.readouterr().out
    )
