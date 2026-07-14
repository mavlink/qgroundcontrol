"""Contract tests for the cross-platform dependency installer."""

from __future__ import annotations

from unittest.mock import MagicMock, call, patch

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
    _common,
    _detect_just_version,
    _fedora,
    _windows,
    detect_platform,
    get_apt_install_command,
    get_apt_update_command,
    get_arch_packages,
    get_brew_install_command,
    get_debian_packages,
    get_fedora_packages,
    install_just_debian,
    parse_args,
    resolve_package_alternatives,
    run_apt_install_with_retry,
    validate_extra_packages,
)

from ._helpers import REPO_ROOT, completed


def test_package_tables_are_well_formed_and_deduplicated() -> None:
    for table, aggregate in (
        (DEBIAN_PACKAGES, get_debian_packages()),
        (FEDORA_PACKAGES, get_fedora_packages()),
        (ARCH_PACKAGES, get_arch_packages()),
    ):
        assert table
        for category, packages in table.items():
            assert packages, category
            assert len(packages) == len(set(packages)), category
            assert all(PACKAGE_NAME_RE.fullmatch(package) for package in packages)
        assert len(aggregate) == len(set(aggregate))

    assert MACOS_PACKAGES
    assert PIPX_PACKAGES


def test_platform_package_contracts() -> None:
    assert {"cmake", "git", "ninja-build"} <= set(get_debian_packages("core"))
    assert "just" in get_fedora_packages("core")
    assert "base-devel" in get_arch_packages("core")
    assert get_debian_packages("unknown") == []
    assert get_fedora_packages("unknown") == []
    assert get_arch_packages("unknown") == []

    aggregate = set(get_debian_packages())
    assert "gstreamer1.0-qt6" not in aggregate
    assert get_debian_packages("cross_arm64")
    assert not (
        {"libc6", "libstdc++6", "libgcc-s1"} & aggregate & set(get_debian_packages("cross_arm64"))
    )

    sysroot_script = (REPO_ROOT / "deploy/docker/install-sysroot-aarch64.sh").read_text()
    assert "--category cross_arm64" in sysroot_script
    assert "libgstreamer1.0-dev:arm64" not in sysroot_script


def test_resolve_debian_package_alternatives() -> None:
    package = "libgstreamer-plugins-good1.0-dev"
    alternative = "libgstreamer-plugins-extra1.0-dev"

    with patch(
        "setup.install_dependencies._common.check_apt_package_available",
        side_effect=lambda name: name == alternative,
    ):
        assert resolve_package_alternatives(["cmake", package]) == ["cmake", alternative]

    with patch(
        "setup.install_dependencies._common.check_apt_package_available", return_value=False
    ):
        assert resolve_package_alternatives(["cmake", package]) == ["cmake"]


def test_detect_platform() -> None:
    for sys_platform, os_ids, expected in (
        ("linux", {"debian"}, "debian"),
        ("linux", {"ubuntu", "debian"}, "debian"),
        ("linux", {"fedora"}, "fedora"),
        ("linux", {"arch"}, "arch"),
        ("linux", set(), "linux"),
        ("darwin", set(), "macos"),
        ("win32", set(), "windows"),
    ):
        with (
            patch("sys.platform", sys_platform),
            patch("pathlib.Path.exists", return_value="debian" in os_ids),
            patch("setup.install_dependencies._common._os_release_ids", return_value=os_ids),
        ):
            assert detect_platform() == expected


def test_local_environment_write_uses_windows_registry_boundary() -> None:
    registry = MagicMock()
    registry.HKEY_LOCAL_MACHINE = 1
    registry.KEY_SET_VALUE = 2
    registry.REG_EXPAND_SZ = 3
    key = object()
    registry.OpenKey.return_value.__enter__.return_value = key

    with (
        patch.object(_common, "is_windows", return_value=True),
        patch.object(_common, "_load_winreg", return_value=registry),
    ):
        _common._set_env_var_local("QGC_TEST", "value")

    registry.OpenKey.assert_called_once_with(
        registry.HKEY_LOCAL_MACHINE,
        r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment",
        0,
        registry.KEY_SET_VALUE,
    )
    registry.SetValueEx.assert_called_once_with(key, "QGC_TEST", 0, registry.REG_EXPAND_SZ, "value")


def test_cli_parses_supported_install_options() -> None:
    defaults = parse_args([])
    assert (defaults.platform, defaults.category, defaults.dry_run) == (None, None, False)

    args = parse_args(
        [
            "--platform",
            "windows",
            "--category",
            "qt",
            "--dry-run",
            "--list",
            "--gstreamer-version",
            "1.28.4",
            "--skip-gstreamer",
            "--vulkan",
            "--nsis",
            "--validate-extra-packages",
            "foo",
            "bar",
        ]
    )
    assert args.platform == "windows"
    assert args.category == "qt"
    assert args.dry_run and args.list_packages and args.skip_gstreamer and args.vulkan and args.nsis
    assert args.gstreamer_version == "1.28.4"
    assert args.validate_extra_packages == ["foo", "bar"]


def test_extra_package_names_are_validated() -> None:
    assert validate_extra_packages(["foo", "libbar-dev", "baz+1"]) == [
        "foo",
        "libbar-dev",
        "baz+1",
    ]
    with pytest.raises(ValueError, match="Invalid package name"):
        validate_extra_packages(["good", "bad;rm"])


def test_apt_install_refreshes_index_before_retry() -> None:
    with patch(
        "setup.install_dependencies._common.run_command", side_effect=[False, True, True]
    ) as run:
        assert run_apt_install_with_retry(["cmake"], dry_run=False, sudo=True, max_attempts=2)

    assert run.call_args_list == [
        call(get_apt_install_command(["cmake"]), False, sudo=True),
        call(get_apt_update_command(), False, sudo=True),
        call(get_apt_install_command(["cmake"]), False, sudo=True),
    ]


def test_brew_command_skips_installed_packages() -> None:
    for installed, packages, expected in (
        ("pkgconf\nqt\n", ["pkgconf", "ninja", "qt"], ["brew", "install", "--quiet", "ninja"]),
        ("pkgconf\nninja\n", ["pkgconf", "ninja"], ["true"]),
        ("", [], ["true"]),
    ):
        with patch(
            "setup.install_dependencies._common.subprocess.run", return_value=completed(installed)
        ):
            assert get_brew_install_command(packages) == expected


def test_just_version_detection_and_install_policy() -> None:
    with patch("setup.install_dependencies._common.has_command", return_value=False):
        assert _detect_just_version() is None

    with (
        patch("setup.install_dependencies._common.has_command", return_value=True),
        patch(
            "setup.install_dependencies._debian.subprocess.run",
            return_value=completed("just 1.36.0\n"),
        ),
    ):
        assert _detect_just_version() == (1, 36, 0)

    with (
        patch(
            "setup.install_dependencies._debian._detect_just_version",
            return_value=tuple(version + 1 for version in JUST_MIN_VERSION),
        ),
        patch("setup.install_dependencies._common.run_apt_install_with_retry") as apt,
        patch("setup.install_dependencies._common.download_file") as download,
    ):
        assert install_just_debian()
    apt.assert_not_called()
    download.assert_not_called()


def test_old_just_falls_back_to_upstream_binary() -> None:
    versions = iter([(1, 21, 0), (1, 21, 0)])
    with (
        patch(
            "setup.install_dependencies._debian._detect_just_version",
            side_effect=lambda: next(versions),
        ),
        patch("setup.install_dependencies._common.check_apt_package_available", return_value=False),
        patch("setup.install_dependencies._common.download_file", return_value=False) as download,
    ):
        assert not install_just_debian()
    download.assert_called_once()


def test_windows_nsis_install_handles_existing_and_missing_binary(monkeypatch, tmp_path) -> None:
    program_files = tmp_path / "Program Files (x86)"
    monkeypatch.setenv("PROGRAMFILES(X86)", str(program_files))
    makensis = program_files / "NSIS/makensis.exe"
    makensis.parent.mkdir(parents=True)
    makensis.write_text("")
    with patch.object(_windows._c, "download_file") as download:
        assert _windows.install_windows_nsis()
    download.assert_not_called()

    makensis.unlink()
    with (
        patch.object(_windows._c, "download_file", return_value=True),
        patch.object(_windows._c, "run_command", return_value=True),
    ):
        assert not _windows.install_windows_nsis(dry_run=False)


def test_windows_msvc_arm64_component_and_verification() -> None:
    commands: list[list[str]] = []
    with (
        patch.object(_windows._c, "download_file", return_value=True),
        patch.object(
            _windows._c,
            "run_command",
            side_effect=lambda command, *args, **kwargs: commands.append(command) or True,
        ),
        patch.object(_windows, "_verify_msvc", return_value=True),
    ):
        assert _windows.install_windows_msvc(dry_run=False, arm64=True)

    assert "Microsoft.VisualStudio.Workload.VCTools" in commands[0]
    assert "Microsoft.VisualStudio.Component.VC.Tools.ARM64" in commands[0]


def test_windows_dispatches_selected_installers() -> None:
    with (
        patch.object(_windows, "install_windows_msvc", return_value=True) as msvc,
        patch.object(_windows, "install_windows_nsis", return_value=True) as nsis,
        patch.object(_windows, "install_windows_gstreamer", return_value=True),
        patch.object(_windows, "install_windows_vulkan", return_value=True),
    ):
        assert _windows.install_windows(skip_gstreamer=True, msvc=True, msvc_arm64=True, nsis=True)
    msvc.assert_called_once_with(False, True)
    nsis.assert_called_once_with(False)


def test_fedora_install_order_and_failure_boundary() -> None:
    with (
        patch.object(_fedora, "get_fedora_packages", return_value=["cmake"]),
        patch.object(_fedora._c, "run_dnf_install_with_retry", return_value=True) as dnf,
        patch.object(_fedora._c, "run_pipx_install", return_value=True) as pipx,
        patch.object(_fedora._c, "run_command", return_value=True) as cleanup,
    ):
        assert _fedora.install_fedora()
    dnf.assert_called_once_with(["cmake"], False, sudo=True)
    pipx.assert_called_once_with(False)
    cleanup.assert_called_once_with(["dnf", "clean", "all"], False, sudo=True)

    with (
        patch.object(_fedora, "get_fedora_packages", return_value=["cmake"]),
        patch.object(_fedora._c, "run_dnf_install_with_retry", return_value=False),
        patch.object(_fedora._c, "run_pipx_install") as pipx,
    ):
        assert not _fedora.install_fedora()
    pipx.assert_not_called()


def test_arch_install_order_and_failure_boundary() -> None:
    with (
        patch.object(_arch, "get_arch_packages", return_value=["cmake"]),
        patch.object(_arch._c, "run_command", return_value=True) as run,
        patch.object(_arch._c, "run_pacman_install_with_retry", return_value=True) as pacman,
        patch.object(_arch._c, "run_pipx_install", return_value=True) as pipx,
    ):
        assert _arch.install_arch()
    run.assert_any_call(["pacman", "-Syu", "--noconfirm"], False, sudo=True)
    run.assert_any_call(["pacman", "-Sc", "--noconfirm"], False, sudo=True)
    pacman.assert_called_once_with(["cmake"], False, sudo=True)
    pipx.assert_called_once_with(False)

    with (
        patch.object(_arch._c, "run_command", return_value=False),
        patch.object(_arch._c, "run_pacman_install_with_retry") as pacman,
    ):
        assert not _arch.install_arch()
    pacman.assert_not_called()
