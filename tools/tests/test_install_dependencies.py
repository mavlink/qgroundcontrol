#!/usr/bin/env python3
"""Tests for tools/setup/install_dependencies."""

from __future__ import annotations

from typing import TYPE_CHECKING
from unittest.mock import call, patch

import pytest
from setup.install_dependencies import (
    ARCH_PACKAGES,
    DEBIAN_PACKAGES,
    FEDORA_PACKAGES,
    JUST_MIN_VERSION,
    MACOS_PACKAGES,
    PACKAGE_NAME_RE,
    PIPX_PACKAGES,
    _arch,
    _detect_just_version,
    _fedora,
    _windows,
    detect_platform,
    get_apt_install_command,
    get_apt_update_command,
    get_arch_packages,
    get_available_debian_packages,
    get_brew_install_command,
    get_debian_packages,
    get_fedora_packages,
    get_macos_packages,
    install_just_debian,
    parse_args,
    resolve_package_alternatives,
    run_apt_install_with_retry,
    validate_extra_packages,
)

from ._helpers import REPO_ROOT, completed

if TYPE_CHECKING:
    from pathlib import Path


def test_debian_packages_not_empty() -> None:
    assert DEBIAN_PACKAGES
    for category, pkgs in DEBIAN_PACKAGES.items():
        assert pkgs, f"Category '{category}' is empty"


def test_debian_packages_no_duplicates_within_category() -> None:
    for category, pkgs in DEBIAN_PACKAGES.items():
        assert len(pkgs) == len(set(pkgs)), f"Duplicates in category '{category}'"


def test_macos_packages_not_empty() -> None:
    assert MACOS_PACKAGES


def test_pipx_packages_not_empty() -> None:
    assert PIPX_PACKAGES


def test_get_debian_packages_all_returns_no_optional() -> None:
    pkgs = get_debian_packages()
    assert "gstreamer1.0-qt6" not in pkgs, (
        "Optional gstreamer pkg should be excluded from default list"
    )


def test_get_debian_packages_category_core() -> None:
    pkgs = get_debian_packages("core")
    assert "cmake" in pkgs
    assert "git" in pkgs
    assert "ninja-build" in pkgs


def test_get_debian_packages_category_qt() -> None:
    pkgs = get_debian_packages("qt")
    assert any("libxcb" in p for p in pkgs)


def test_get_debian_packages_unknown_category_returns_empty() -> None:
    assert get_debian_packages("nonexistent_category") == []


def test_cross_arm64_excluded_from_aggregate() -> None:
    aggregate = set(get_debian_packages())
    cross = get_debian_packages("cross_arm64")
    assert cross
    assert not (set(cross) & aggregate & {"libc6", "libstdc++6", "libgcc-s1"})


def test_sysroot_script_single_sources_cross_arm64() -> None:
    script = (REPO_ROOT / "deploy" / "docker" / "install-sysroot-aarch64.sh").read_text()
    assert "--category cross_arm64" in script
    for pkg in ("libxcb1-dev", "libgstreamer1.0-dev", "libssl-dev"):
        assert f"{pkg}:arm64" not in script, (
            f"{pkg} should be sourced from cross_arm64, not hardcoded"
        )


def test_get_debian_packages_no_duplicates() -> None:
    pkgs = get_debian_packages()
    assert len(pkgs) == len(set(pkgs))


def test_get_available_debian_packages_filters_unavailable() -> None:
    with patch(
        "setup.install_dependencies._common.check_apt_package_available",
        side_effect=lambda pkg: pkg == "cmake",
    ):
        assert get_available_debian_packages("core") == ["cmake"]


def test_resolve_package_alternatives_keeps_available_primary() -> None:
    with patch(
        "setup.install_dependencies._common.check_apt_package_available",
        side_effect=lambda pkg: True,
    ):
        assert resolve_package_alternatives(["libgstreamer-plugins-good1.0-dev"]) == [
            "libgstreamer-plugins-good1.0-dev"
        ]


def test_resolve_package_alternatives_swaps_to_available_alternative() -> None:
    with patch(
        "setup.install_dependencies._common.check_apt_package_available",
        side_effect=lambda pkg: pkg == "libgstreamer-plugins-extra1.0-dev",
    ):
        assert resolve_package_alternatives(["libgstreamer-plugins-good1.0-dev"]) == [
            "libgstreamer-plugins-extra1.0-dev"
        ]


def test_resolve_package_alternatives_drops_when_none_available() -> None:
    # Regression: Debian bookworm has neither the good-dev package nor its
    # alternative; keeping the unknown name made apt abort the whole install.
    with patch(
        "setup.install_dependencies._common.check_apt_package_available",
        side_effect=lambda pkg: False,
    ):
        assert resolve_package_alternatives(["cmake", "libgstreamer-plugins-good1.0-dev"]) == [
            "cmake"
        ]


def test_get_macos_packages() -> None:
    pkgs = get_macos_packages()
    assert "cmake" in pkgs
    assert "ninja" in pkgs
    assert "ccache" in pkgs


@pytest.mark.parametrize(
    ("table", "getter"),
    [(FEDORA_PACKAGES, get_fedora_packages), (ARCH_PACKAGES, get_arch_packages)],
)
def test_linux_package_tables_well_formed(table, getter) -> None:
    assert table
    for category, pkgs in table.items():
        assert pkgs, f"Category '{category}' is empty"
        assert len(pkgs) == len(set(pkgs)), f"Duplicates in category '{category}'"
        for pkg in pkgs:
            assert PACKAGE_NAME_RE.match(pkg), f"Invalid package name '{pkg}'"
    aggregate = getter()
    assert len(aggregate) == len(set(aggregate))


def test_get_fedora_packages_categories() -> None:
    assert "cmake" in get_fedora_packages("core")
    assert "just" in get_fedora_packages("core")
    assert any("gstreamer1" in p for p in get_fedora_packages("gstreamer"))
    assert get_fedora_packages("nonexistent") == []


def test_get_arch_packages_categories() -> None:
    assert "cmake" in get_arch_packages("core")
    assert "base-devel" in get_arch_packages("core")
    assert "gstreamer" in get_arch_packages("gstreamer")
    assert get_arch_packages("nonexistent") == []


_OS_RELEASE = "setup.install_dependencies._common._os_release_ids"


def test_detect_platform_linux_debian() -> None:
    with (
        patch("sys.platform", "linux"),
        patch("pathlib.Path.exists", return_value=True),
        patch(_OS_RELEASE, return_value={"debian"}),
    ):
        assert detect_platform() == "debian"


def test_detect_platform_linux_ubuntu() -> None:
    with (
        patch("sys.platform", "linux"),
        patch("pathlib.Path.exists", return_value=True),
        patch(_OS_RELEASE, return_value={"ubuntu", "debian"}),
    ):
        assert detect_platform() == "debian"


def test_detect_platform_linux_fedora() -> None:
    with (
        patch("sys.platform", "linux"),
        patch("pathlib.Path.exists", return_value=False),
        patch(_OS_RELEASE, return_value={"fedora"}),
    ):
        assert detect_platform() == "fedora"


def test_detect_platform_linux_arch() -> None:
    with (
        patch("sys.platform", "linux"),
        patch("pathlib.Path.exists", return_value=False),
        patch(_OS_RELEASE, return_value={"arch"}),
    ):
        assert detect_platform() == "arch"


def test_detect_platform_macos() -> None:
    with patch("sys.platform", "darwin"):
        assert detect_platform() == "macos"


def test_detect_platform_windows() -> None:
    with patch("sys.platform", "win32"):
        assert detect_platform() == "windows"


def test_detect_platform_unknown_linux() -> None:
    with (
        patch("sys.platform", "linux"),
        patch("pathlib.Path.exists", return_value=False),
        patch(_OS_RELEASE, return_value=set()),
    ):
        assert detect_platform() == "linux"


def test_parse_args_defaults() -> None:
    args = parse_args([])
    assert args.platform is None
    assert args.dry_run is False
    assert args.list_packages is False
    assert args.category is None


def test_parse_args_platform_debian() -> None:
    args = parse_args(["--platform", "debian"])
    assert args.platform == "debian"


def test_parse_args_platform_windows() -> None:
    args = parse_args(["--platform", "windows"])
    assert args.platform == "windows"


def test_parse_args_dry_run() -> None:
    args = parse_args(["--dry-run"])
    assert args.dry_run is True


def test_parse_args_list() -> None:
    args = parse_args(["--list"])
    assert args.list_packages is True


def test_parse_args_category() -> None:
    args = parse_args(["--category", "qt"])
    assert args.category == "qt"


def test_parse_args_validate_extra_packages() -> None:
    args = parse_args(["--validate-extra-packages", "foo", "bar"])
    assert args.validate_extra_packages == ["foo", "bar"]


def test_parse_args_gstreamer_version() -> None:
    args = parse_args(["--platform", "windows", "--gstreamer-version", "1.24.0"])
    assert args.gstreamer_version == "1.24.0"


def test_parse_args_skip_gstreamer() -> None:
    args = parse_args(["--platform", "windows", "--skip-gstreamer"])
    assert args.skip_gstreamer is True


def test_parse_args_vulkan() -> None:
    args = parse_args(["--platform", "windows", "--vulkan"])
    assert args.vulkan is True


def test_parse_args_nsis() -> None:
    args = parse_args(["--platform", "windows", "--nsis"])
    assert args.nsis is True


def test_validate_extra_packages_accepts_valid_names() -> None:
    assert validate_extra_packages(["foo", "libbar-dev", "baz+1"]) == ["foo", "libbar-dev", "baz+1"]


def test_validate_extra_packages_rejects_invalid_names() -> None:
    with pytest.raises(ValueError, match="Invalid package name"):
        validate_extra_packages(["good", "bad;rm"])


def test_download_file_dry_run(tmp_path: Path) -> None:
    from setup.install_dependencies import download_file

    dest = tmp_path / "test.bin"
    result = download_file("https://example.com/test.bin", dest, dry_run=True)
    assert result is True
    assert not dest.exists()


def test_download_file_network_error(tmp_path: Path) -> None:
    from setup.install_dependencies import download_file

    dest = tmp_path / "test.bin"
    # Mock httpx to raise, then fall through to urllib which also raises
    with patch("urllib.request.urlopen", side_effect=OSError("unreachable")):
        result = download_file("https://example.com/test.bin", dest, dry_run=False)
    assert result is False


def test_run_apt_install_with_retry_success_first_try() -> None:
    with patch("setup.install_dependencies._common.run_command", return_value=True) as mock_run:
        result = run_apt_install_with_retry(["cmake"], dry_run=False, sudo=True, max_attempts=2)

    assert result is True
    mock_run.assert_called_once_with(get_apt_install_command(["cmake"]), False, sudo=True)


def test_run_apt_install_with_retry_refreshes_index_then_retries() -> None:
    with patch(
        "setup.install_dependencies._common.run_command", side_effect=[False, True, True]
    ) as mock_run:
        result = run_apt_install_with_retry(["cmake"], dry_run=False, sudo=True, max_attempts=2)

    assert result is True
    mock_run.assert_has_calls(
        [
            call(get_apt_install_command(["cmake"]), False, sudo=True),
            call(get_apt_update_command(), False, sudo=True),
            call(get_apt_install_command(["cmake"]), False, sudo=True),
        ]
    )


def test_get_brew_install_command_filters_already_installed() -> None:
    with patch(
        "setup.install_dependencies._common.subprocess.run",
        return_value=completed("pkgconf\nqt\ncmake\n"),
    ):
        cmd = get_brew_install_command(["pkgconf", "ninja", "qt"])
    assert cmd == ["brew", "install", "--quiet", "ninja"]


def test_get_brew_install_command_all_installed_returns_noop() -> None:
    with patch(
        "setup.install_dependencies._common.subprocess.run",
        return_value=completed("pkgconf\nninja\n"),
    ):
        cmd = get_brew_install_command(["pkgconf", "ninja"])
    assert cmd == ["true"]


def test_get_brew_install_command_empty_input_returns_noop() -> None:
    cmd = get_brew_install_command([])
    assert cmd == ["true"]


def test_get_brew_install_command_brew_list_failure_passes_all() -> None:
    # brew list failure (e.g. brew not yet on PATH) shouldn't suppress installs.
    with patch(
        "setup.install_dependencies._common.subprocess.run",
        return_value=completed(returncode=1),
    ):
        cmd = get_brew_install_command(["pkgconf", "ninja"])
    assert cmd == ["brew", "install", "--quiet", "pkgconf", "ninja"]


def test_detect_just_version_absent() -> None:
    with patch("setup.install_dependencies._common.has_command", return_value=False):
        assert _detect_just_version() is None


def test_detect_just_version_parses_output() -> None:
    result = completed("just 1.36.0\n")
    with (
        patch("setup.install_dependencies._common.has_command", return_value=True),
        patch("setup.install_dependencies._debian.subprocess.run", return_value=result),
    ):
        assert _detect_just_version() == (1, 36, 0)


def test_detect_just_version_handles_missing_patch() -> None:
    result = completed("just 1.30\n")
    with (
        patch("setup.install_dependencies._common.has_command", return_value=True),
        patch("setup.install_dependencies._debian.subprocess.run", return_value=result),
    ):
        assert _detect_just_version() == (1, 30, 0)


def test_install_just_debian_skips_when_current_version_meets_minimum() -> None:
    with (
        patch(
            "setup.install_dependencies._debian._detect_just_version",
            return_value=tuple(v + 1 for v in JUST_MIN_VERSION),
        ),
        patch("setup.install_dependencies._common.run_apt_install_with_retry") as mock_apt,
        patch("setup.install_dependencies._common.download_file") as mock_dl,
    ):
        assert install_just_debian() is True
    mock_apt.assert_not_called()
    mock_dl.assert_not_called()


def test_install_just_debian_upgrades_when_existing_too_old() -> None:
    """Stale apt-installed just (1.21) must trigger upstream binary install."""
    versions = iter([(1, 21, 0), (1, 21, 0)])
    with (
        patch(
            "setup.install_dependencies._debian._detect_just_version",
            side_effect=lambda: next(versions),
        ),
        patch("setup.install_dependencies._common.check_apt_package_available", return_value=False),
        patch("setup.install_dependencies._common.download_file", return_value=False) as mock_dl,
    ):
        install_just_debian()
    mock_dl.assert_called_once()


def test_install_just_debian_falls_back_when_apt_version_too_old() -> None:
    """apt's just may itself be too old (Ubuntu 22.04); fall through to upstream."""
    versions = iter([None, (1, 21, 0)])
    with (
        patch(
            "setup.install_dependencies._debian._detect_just_version",
            side_effect=lambda: next(versions),
        ),
        patch("setup.install_dependencies._common.check_apt_package_available", return_value=True),
        patch("setup.install_dependencies._common.run_apt_install_with_retry", return_value=True),
        patch("setup.install_dependencies._common.download_file", return_value=False) as mock_dl,
    ):
        install_just_debian()
    mock_dl.assert_called_once()


def test_install_windows_nsis_dry_run() -> None:
    with (
        patch.object(_windows._c, "download_file", return_value=True) as dl,
        patch.object(_windows._c, "run_command", return_value=True) as rc,
    ):
        assert _windows.install_windows_nsis(dry_run=True) is True
    dl.assert_called_once()
    rc.assert_called_once()


def test_install_windows_nsis_already_installed(monkeypatch, tmp_path: Path) -> None:
    makensis = tmp_path / "NSIS" / "makensis.exe"
    makensis.parent.mkdir(parents=True)
    makensis.write_text("")
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    with patch.object(_windows._c, "download_file") as dl:
        assert _windows.install_windows_nsis() is True
    dl.assert_not_called()


def test_install_windows_nsis_missing_after_install(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    with (
        patch.object(_windows._c, "download_file", return_value=True),
        patch.object(_windows._c, "run_command", return_value=True),
    ):
        assert _windows.install_windows_nsis(dry_run=False) is False


def test_install_windows_msvc_arm64_adds_component() -> None:
    captured: list[list[str]] = []
    with (
        patch.object(_windows._c, "download_file", return_value=True),
        patch.object(
            _windows._c,
            "run_command",
            side_effect=lambda cmd, *a, **kw: captured.append(cmd) or True,
        ),
        patch.object(_windows, "_verify_msvc", return_value=True),
    ):
        assert _windows.install_windows_msvc(dry_run=False, arm64=True) is True
    assert "Microsoft.VisualStudio.Workload.VCTools" in captured[0]
    assert "Microsoft.VisualStudio.Component.VC.Tools.ARM64" in captured[0]


def test_install_windows_msvc_verification_failure() -> None:
    with (
        patch.object(_windows._c, "download_file", return_value=True),
        patch.object(_windows._c, "run_command", return_value=True),
        patch.object(_windows, "_verify_msvc", return_value=False),
    ):
        assert _windows.install_windows_msvc(dry_run=False) is False


def test_verify_msvc_skips_when_vswhere_absent(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    assert _windows._verify_msvc(arm64=False) is True


def test_verify_msvc_fails_when_component_missing(monkeypatch, tmp_path: Path) -> None:
    vswhere = tmp_path / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    vswhere.parent.mkdir(parents=True)
    vswhere.write_text("")
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    with patch.object(
        _windows.subprocess,
        "run",
        return_value=completed(),
    ):
        assert _windows._verify_msvc(arm64=False) is False


def test_install_windows_dispatch_invokes_msvc_and_nsis() -> None:
    with (
        patch.object(_windows, "install_windows_msvc", return_value=True) as msvc,
        patch.object(_windows, "install_windows_nsis", return_value=True) as nsis,
        patch.object(_windows, "install_windows_gstreamer", return_value=True),
        patch.object(_windows, "install_windows_vulkan", return_value=True),
    ):
        assert (
            _windows.install_windows(
                dry_run=False,
                skip_gstreamer=True,
                msvc=True,
                msvc_arm64=True,
                nsis=True,
            )
            is True
        )
    msvc.assert_called_once_with(False, True)
    nsis.assert_called_once_with(False)


def test_install_fedora_installs_packages_pipx_then_cleans() -> None:
    with (
        patch.object(_fedora, "get_fedora_packages", return_value=["cmake"]),
        patch.object(_fedora._c, "run_dnf_install_with_retry", return_value=True) as dnf,
        patch.object(_fedora._c, "run_pipx_install", return_value=True) as pipx,
        patch.object(_fedora._c, "run_command", return_value=True) as cleanup,
    ):
        assert _fedora.install_fedora(dry_run=False) is True
    dnf.assert_called_once_with(["cmake"], False, sudo=True)
    pipx.assert_called_once_with(False)
    cleanup.assert_called_once_with(["dnf", "clean", "all"], False, sudo=True)


def test_install_fedora_skip_system_packages_skips_dnf_and_cleanup() -> None:
    with (
        patch.object(_fedora._c, "run_dnf_install_with_retry", return_value=True) as dnf,
        patch.object(_fedora._c, "run_pipx_install", return_value=True) as pipx,
        patch.object(_fedora._c, "run_command", return_value=True) as cleanup,
    ):
        assert _fedora.install_fedora(dry_run=False, skip_system_packages=True) is True
    dnf.assert_not_called()
    cleanup.assert_not_called()
    pipx.assert_called_once_with(False)


def test_install_fedora_returns_false_when_dnf_fails() -> None:
    with (
        patch.object(_fedora, "get_fedora_packages", return_value=["cmake"]),
        patch.object(_fedora._c, "run_dnf_install_with_retry", return_value=False),
        patch.object(_fedora._c, "run_pipx_install", return_value=True) as pipx,
    ):
        assert _fedora.install_fedora(dry_run=False) is False
    pipx.assert_not_called()


def test_install_fedora_unknown_category_returns_false() -> None:
    with (
        patch.object(_fedora, "get_fedora_packages", return_value=[]),
        patch.object(_fedora._c, "run_dnf_install_with_retry", return_value=True) as dnf,
    ):
        assert _fedora.install_fedora(dry_run=False, category="bogus") is False
    dnf.assert_not_called()


def test_install_arch_syncs_installs_then_cleans() -> None:
    with (
        patch.object(_arch, "get_arch_packages", return_value=["cmake"]),
        patch.object(_arch._c, "run_command", return_value=True) as run_command,
        patch.object(_arch._c, "run_pacman_install_with_retry", return_value=True) as pac,
        patch.object(_arch._c, "run_pipx_install", return_value=True) as pipx,
    ):
        assert _arch.install_arch(dry_run=False) is True
    run_command.assert_any_call(["pacman", "-Syu", "--noconfirm"], False, sudo=True)
    run_command.assert_any_call(["pacman", "-Sc", "--noconfirm"], False, sudo=True)
    pac.assert_called_once_with(["cmake"], False, sudo=True)
    pipx.assert_called_once_with(False)


def test_install_arch_aborts_when_sync_fails() -> None:
    with (
        patch.object(_arch._c, "run_command", return_value=False),
        patch.object(_arch._c, "run_pacman_install_with_retry", return_value=True) as pac,
    ):
        assert _arch.install_arch(dry_run=False) is False
    pac.assert_not_called()


def test_install_arch_category_skips_pipx_and_cleanup() -> None:
    with (
        patch.object(_arch, "get_arch_packages", return_value=["cmake"]),
        patch.object(_arch._c, "run_command", return_value=True) as run_command,
        patch.object(_arch._c, "run_pacman_install_with_retry", return_value=True),
        patch.object(_arch._c, "run_pipx_install", return_value=True) as pipx,
    ):
        assert _arch.install_arch(dry_run=False, category="gstreamer") is True
    pipx.assert_not_called()
    cleanup_calls = [c for c in run_command.call_args_list if c.args[0][:2] == ["pacman", "-Sc"]]
    assert cleanup_calls == []
