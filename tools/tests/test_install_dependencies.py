"""Tests for tools/setup/install_dependencies."""

from __future__ import annotations

import io
import os
from email.message import Message
from typing import TYPE_CHECKING
from unittest.mock import MagicMock, call, patch
from urllib.error import HTTPError

import pytest
from setup.install_dependencies import _cli, _debian, _linux, _macos, _windows
from setup.install_dependencies import _common as install_common
from setup.install_dependencies._cli import parse_args
from setup.install_dependencies._common import (
    detect_platform,
    download_file,
    get_apt_install_command,
    get_apt_update_command,
    get_brew_install_command,
    run_apt_install_with_retry,
)
from setup.install_dependencies._debian import drop_unavailable_optional
from setup.install_dependencies._packages import (
    ARCH_PACKAGES,
    DEBIAN_PACKAGES,
    FEDORA_PACKAGES,
    MACOS_PACKAGES,
    PACKAGE_NAME_RE,
    PACKAGE_TABLES,
    debian_jdk_package,
    get_debian_packages,
    get_packages,
    validate_extra_packages,
)

from ._helpers import completed

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows


def test_debian_packages_not_empty() -> None:
    assert DEBIAN_PACKAGES
    for category, pkgs in DEBIAN_PACKAGES.items():
        assert pkgs, f"Category '{category}' is empty"


def test_debian_packages_no_duplicates_within_category() -> None:
    for category, pkgs in DEBIAN_PACKAGES.items():
        assert len(pkgs) == len(set(pkgs)), f"Duplicates in category '{category}'"


def test_macos_packages_not_empty() -> None:
    assert MACOS_PACKAGES


def test_get_debian_packages_android_is_core_plus_the_build_config_jdk() -> None:
    from common.build_config import get_build_config_value

    jdk = f"openjdk-{get_build_config_value('android.java_version')}-jdk"
    assert debian_jdk_package() == jdk
    assert get_debian_packages("android") == [*DEBIAN_PACKAGES["core"], jdk]
    assert jdk not in get_debian_packages()


def test_debian_jdk_package_rejects_a_missing_java_version() -> None:
    with (
        patch("common.build_config.get_build_config_value", return_value=""),
        pytest.raises(ValueError, match="java_version"),
    ):
        debian_jdk_package()


def test_install_debian_android_installs_core_and_jdk_with_build_tools() -> None:
    with (
        patch.object(_debian, "enable_universe") as universe,
        patch.object(_debian._c, "run_command", return_value=True) as run_command,
        patch.object(_debian._c, "run_apt_install_with_retry", return_value=True) as apt,
        patch.object(_debian._c, "install_build_tools", return_value=True) as build_tools,
    ):
        assert _debian.install_debian(dry_run=False, category="android") is True
    universe.assert_not_called()
    apt.assert_called_once_with(get_debian_packages("android"), False, sudo=True)
    run_command.assert_called_once_with(_debian._c.get_apt_update_command(), False, sudo=True)
    build_tools.assert_called_once_with(False)


def test_install_debian_enables_universe_then_updates_once_and_cleans() -> None:
    events: list[object] = []
    with (
        patch.object(
            _debian,
            "enable_universe",
            side_effect=lambda dry_run: events.append("universe") or True,
        ),
        patch.object(
            _debian._c,
            "run_command",
            side_effect=lambda command, *args, **kwargs: events.append(command) or True,
        ),
        patch.object(_debian._c, "check_apt_package_available", return_value=False),
        patch.object(_debian._c, "run_apt_install_with_retry", return_value=True) as apt,
        patch.object(_debian._c, "install_build_tools", return_value=True),
    ):
        assert _debian.install_debian() is True
    assert events == ["universe", get_apt_update_command(), ["apt-get", "clean"]]
    optional = "libgstreamer-plugins-good1.0-dev"
    assert apt.call_args.args[0] == [pkg for pkg in get_debian_packages() if pkg != optional]


def test_install_debian_stops_when_universe_cannot_be_enabled() -> None:
    with (
        patch.object(_debian, "enable_universe", return_value=False),
        patch.object(_debian._c, "run_command") as run_command,
    ):
        assert _debian.install_debian() is False
    run_command.assert_not_called()


def test_install_debian_unknown_category_returns_false() -> None:
    with patch.object(_debian._c, "run_command") as run_command:
        assert _debian.install_debian(category="bogus") is False
    run_command.assert_not_called()


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


def test_get_debian_packages_no_duplicates() -> None:
    pkgs = get_debian_packages()
    assert len(pkgs) == len(set(pkgs))


def test_gstreamer_build_excluded_from_aggregate() -> None:
    others = {
        pkg
        for category, pkgs in DEBIAN_PACKAGES.items()
        if category != "gstreamer_build"
        for pkg in pkgs
    }
    only_gstreamer_build = set(get_debian_packages("gstreamer_build")) - others
    assert {"libva-dev", "yasm"} <= only_gstreamer_build
    assert not only_gstreamer_build & set(get_debian_packages())


def test_gstreamer_build_android_is_opt_in() -> None:
    android = set(get_debian_packages("gstreamer_build_android"))
    assert {"python3-venv", "bison", "flex", "autoconf", "automake"} <= android
    assert not {"python3-venv", "bison", "flex"} & set(get_debian_packages())


@pytest.mark.parametrize("available", [True, False])
def test_unavailable_optional_packages_are_dropped(available: bool) -> None:
    # Debian 12 has no libgstreamer-plugins-good1.0-dev; an unknown name makes apt abort the install.
    packages = ["cmake", "libgstreamer-plugins-good1.0-dev"]
    with patch.object(_debian._c, "check_apt_package_available", return_value=available) as check:
        assert drop_unavailable_optional(packages) == (packages if available else ["cmake"])
    check.assert_called_once_with("libgstreamer-plugins-good1.0-dev")


def test_dry_run_keeps_optional_packages_without_querying_apt() -> None:
    with patch.object(_debian._c, "check_apt_package_available") as check:
        assert drop_unavailable_optional(["libgstreamer-plugins-good1.0-dev"], dry_run=True) == [
            "libgstreamer-plugins-good1.0-dev"
        ]
    check.assert_not_called()


class TestEnableUniverse:
    @pytest.fixture(autouse=True)
    def ubuntu(self):
        with patch.object(_debian._c, "is_ubuntu", return_value=True):
            yield

    def test_existing_repository_skips_apt(self, tmp_path):
        (tmp_path / "sources.list.d").mkdir()
        (tmp_path / "sources.list.d" / "ubuntu.sources").write_text(
            "Components: main universe restricted\n"
        )
        with patch.object(_debian._c, "run_command") as run:
            assert _debian.enable_universe(apt_dir=tmp_path) is True
        run.assert_not_called()

    @pytest.mark.parametrize("has_add_apt_repository", [True, False])
    def test_missing_repository_is_added_without_a_refresh(self, tmp_path, has_add_apt_repository):
        (tmp_path / "sources.list").write_text("deb http://archive.ubuntu.com/ubuntu noble main\n")
        with (
            patch.object(_debian._c, "has_command", return_value=has_add_apt_repository),
            patch.object(_debian._c, "run_apt_install_with_retry", return_value=True) as apt,
            patch.object(_debian._c, "run_command", return_value=True) as run,
        ):
            assert _debian.enable_universe(apt_dir=tmp_path) is True
        run.assert_called_once_with(
            ["add-apt-repository", "-y", "-n", "universe"], False, sudo=True
        )
        assert apt.called is not has_add_apt_repository

    def test_plain_debian_has_no_universe(self, tmp_path):
        with (
            patch.object(_debian._c, "is_ubuntu", return_value=False),
            patch.object(_debian._c, "run_command") as run,
        ):
            assert _debian.enable_universe(apt_dir=tmp_path) is True
        run.assert_not_called()


def test_get_macos_packages() -> None:
    pkgs = get_packages("macos")
    assert "cmake" in pkgs
    assert "ninja" in pkgs
    assert "ccache" in pkgs
    # Homebrew's mold links only ELF, and CMake picks no alternative linker on Apple platforms.
    assert "mold" not in pkgs


@pytest.mark.parametrize("platform", ["fedora", "arch", "macos"])
def test_just_comes_from_the_locked_build_group_not_system_packages(platform: str) -> None:
    assert "just" not in get_packages(platform)


def test_install_macos_installs_the_requested_homebrew_packages() -> None:
    with (
        patch.object(_macos._c, "has_command", return_value=True),
        patch.object(_macos._c, "is_ci", return_value=True),
        patch.object(_macos._c, "run_command", return_value=False),
        patch.object(_macos._c, "get_brew_install_command", return_value=["brew"]) as brew,
    ):
        assert _macos.install_macos(packages=["nasm"]) is False

    brew.assert_called_once_with(["nasm"])


@pytest.mark.parametrize(
    ("table", "platform"), [(FEDORA_PACKAGES, "fedora"), (ARCH_PACKAGES, "arch")]
)
def test_linux_package_tables_well_formed(table, platform) -> None:
    assert table
    for category, pkgs in table.items():
        assert pkgs, f"Category '{category}' is empty"
        assert len(pkgs) == len(set(pkgs)), f"Duplicates in category '{category}'"
        for pkg in pkgs:
            assert PACKAGE_NAME_RE.match(pkg), f"Invalid package name '{pkg}'"
    aggregate = get_packages(platform)
    assert len(aggregate) == len(set(aggregate))


def test_get_fedora_packages_categories() -> None:
    assert "cmake" in get_packages("fedora", "core")
    assert any("gstreamer1" in p for p in get_packages("fedora", "gstreamer"))
    assert get_packages("fedora", "nonexistent") == []


def test_get_arch_packages_categories() -> None:
    assert "cmake" in get_packages("arch", "core")
    assert "base-devel" in get_packages("arch", "core")
    assert "gstreamer" in get_packages("arch", "gstreamer")
    assert get_packages("arch", "nonexistent") == []


@pytest.mark.parametrize("platform", list(PACKAGE_TABLES))
def test_list_prints_every_platform(platform: str, capsys) -> None:
    _cli.list_packages(platform)
    out = capsys.readouterr().out
    assert out.startswith(f"{_cli.PLATFORM_LABELS[platform]}:\n")
    for packages in PACKAGE_TABLES[platform].values():
        assert all(f"  - {package}\n" in out for package in packages)
    assert out.strip().splitlines()[-1]


def test_list_without_platform_covers_them_all(capsys) -> None:
    _cli.list_packages()
    out = capsys.readouterr().out
    assert [label for label in _cli.PLATFORM_LABELS.values() if f"{label}:\n" in out] == list(
        _cli.PLATFORM_LABELS.values()
    )
    assert _cli.PLATFORM_LABELS.keys() == PACKAGE_TABLES.keys()


@pytest.mark.parametrize("platform", ["fedora", "arch", "macos"])
def test_print_packages_uses_the_platform_table(platform: str, capsys) -> None:
    _cli.print_packages(platform)
    assert capsys.readouterr().out.split() == get_packages(platform)


def test_print_packages_rejects_windows(capsys) -> None:
    with pytest.raises(SystemExit):
        _cli.print_packages("windows")


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
    dest = tmp_path / "test.bin"
    result = download_file("https://example.com/test.bin", dest, dry_run=True)
    assert result is True
    assert not dest.exists()


@pytest.mark.parametrize("warn_on_failure", [False, True])
def test_download_file_network_error(tmp_path: Path, warn_on_failure: bool) -> None:
    dest = tmp_path / "test.bin"
    with (
        patch("urllib.request.urlopen", side_effect=OSError("unreachable")) as request,
        patch("common.net.time.sleep") as sleep,
        patch.object(install_common, "log_warn") as warn,
        patch.object(install_common, "log_error") as error,
    ):
        result = download_file(
            "https://example.com/test.bin", dest, retries=2, warn_on_failure=warn_on_failure
        )
    assert result is False
    assert request.call_count == 3
    assert sleep.call_count == 2
    assert warn.call_count == int(warn_on_failure)
    assert error.call_count == int(not warn_on_failure)
    assert not dest.exists()


@pytest.mark.parametrize("recover", [False, True])
def test_download_file_retries_http_error(tmp_path: Path, recover: bool) -> None:
    url = "https://example.com/just.tar.gz"
    dest = tmp_path / "just.tar.gz"
    failure = HTTPError(url, 500, "Internal Server Error", Message(), None)
    response = io.BytesIO(b"archive") if recover else failure
    with (
        patch("urllib.request.urlopen", side_effect=[failure, response]) as request,
        patch("common.net.time.sleep") as sleep,
    ):
        assert download_file(url, dest, retries=1, timeout=1.5) is recover
    assert request.call_count == 2
    assert all(call.kwargs["timeout"] == 1.5 for call in request.call_args_list)
    sleep.assert_called_once_with(5.0)
    if recover:
        assert dest.read_bytes() == b"archive"
    else:
        assert not dest.exists()


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


@pytest.mark.parametrize(
    ("install", "refresh", "message"),
    [
        (
            install_common.run_dnf_install_with_retry,
            ["dnf", "makecache"],
            "  dnf install failed (attempt 1/2); retrying...",
        ),
        (
            install_common.run_pacman_install_with_retry,
            ["pacman", "-Syy", "--noconfirm"],
            "  pacman install failed (attempt 1/2); refreshing...",
        ),
    ],
)
def test_install_with_retry_stops_when_refresh_fails(install, refresh, message, capsys) -> None:
    with patch(
        "setup.install_dependencies._common.run_command", side_effect=[False, False]
    ) as mock_run:
        assert install(["cmake"], dry_run=False, sudo=True) is False

    assert mock_run.call_count == 2
    assert mock_run.call_args_list[1] == call(refresh, False, sudo=True)
    assert capsys.readouterr().out.splitlines() == [message]


def test_install_build_tools_uses_locked_profile(tmp_path: Path) -> None:
    with patch("qgc_tools.python_env.sync_groups", return_value=tmp_path) as sync:
        assert install_common.install_build_tools(dry_run=True)
    sync.assert_called_once_with("build", dry_run=True)


def test_install_build_tools_propagates_setup_failure() -> None:
    with patch("qgc_tools.python_env.sync_groups", side_effect=FileNotFoundError("uv missing")):
        assert not install_common.install_build_tools()


@pytest.mark.parametrize("ci", [True, False])
def test_install_build_tools_puts_the_environment_first_on_path(
    tmp_path: Path, monkeypatch, capsys, ci: bool
) -> None:
    from qgc_tools.python_env import executable

    scripts = str(executable("cmake", tmp_path).parent)
    monkeypatch.setenv("PATH", "/usr/bin")
    with (
        patch("qgc_tools.python_env.sync_groups", return_value=tmp_path),
        patch.object(install_common, "is_ci", return_value=ci),
        patch.object(install_common, "append_github_path") as github_path,
    ):
        assert install_common.install_build_tools()
    assert install_common.os.environ["PATH"] == f"{scripts}{install_common.os.pathsep}/usr/bin"
    assert github_path.called is ci
    assert (f"activate it with . {scripts}/activate" in capsys.readouterr().out) is not ci


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


@pytest.mark.parametrize(
    ("fallback_package", "mirror_succeeds"),
    [(None, True), ("runtime", True), ("devel", True), ("runtime", False), ("devel", False)],
)
def test_install_macos_gstreamer_fallback(
    fallback_package: str | None, mirror_succeeds: bool
) -> None:
    from common.gstreamer import mirror_url, package_filename, package_url

    version = "1.28.4"
    packages = _macos.GSTREAMER_PACKAGES
    filenames = {label: package_filename(package, version) for label, package in packages.items()}
    checksums = {"macos": "a" * 64, "macos_devel": "b" * 64}

    def download(url: str, dest: Path, *args: object, **kwargs: object) -> bool:
        if fallback_package is None or dest.name != filenames[fallback_package]:
            return True
        return mirror_succeeds and url.startswith(mirror_url("macos", version).rsplit("/", 1)[0])

    with (
        patch.object(_macos._c, "has_command", return_value=True),
        patch.object(_macos._c, "is_ci", return_value=True),
        patch.object(_macos._c, "get_brew_install_command", return_value=["brew", "install"]),
        patch.object(_macos._c, "get_build_config_value", return_value=version),
        patch.object(
            _macos._c, "pinned_checksum", side_effect=lambda _, artifact: checksums[artifact]
        ),
        patch.object(_macos.Path, "exists", return_value=False),
        patch.object(_macos._c, "download_file", side_effect=download) as dl,
        patch.object(_macos._c, "run_command", return_value=True) as run_command,
    ):
        assert _macos.install_macos() is mirror_succeeds

    for label, package in packages.items():
        package_calls = [
            entry for entry in dl.call_args_list if entry.args[1].name == filenames[label]
        ]
        expected_urls = [package_url(package, version)]
        if label == fallback_package:
            expected_urls.append(mirror_url(package, version))
        assert [entry.args[0] for entry in package_calls] == expected_urls
        assert all(entry.kwargs["expected_sha256"] == checksums[package] for entry in package_calls)
        assert package_calls[0].kwargs["warn_on_failure"] is True

    installs = [
        entry.args[0] for entry in run_command.call_args_list if entry.args[0][0] == "installer"
    ]
    if mirror_succeeds:
        assert [os.path.basename(command[2]) for command in installs] == list(filenames.values())
        for entry in run_command.call_args_list[1:]:
            assert entry.kwargs == {"sudo": True}
        assert all(command[3:] == ["-target", "/"] for command in installs)
    else:
        assert installs == []


def test_homebrew_bootstrap_downloads_with_retries_and_runs_non_interactively() -> None:
    def download(url: str, dest: Path, *args: object, **kwargs: object) -> bool:
        dest.write_bytes(b"echo brew")
        return True

    with (
        patch.object(_macos._c, "download_file", side_effect=download) as dl,
        patch.object(_macos.subprocess, "run", return_value=completed()) as run,
    ):
        assert _macos._install_homebrew() is True
    assert dl.call_args.args[0] == _macos.HOMEBREW_INSTALL_URL
    run.assert_called_once_with(["/bin/bash"], input=b"echo brew")


def test_install_windows_nsis_dry_run() -> None:
    with (
        patch.object(_windows._c, "download_file", return_value=True) as dl,
        patch.object(_windows._c, "run_command", return_value=True) as rc,
    ):
        assert _windows.install_windows_nsis(dry_run=True) is True
    dl.assert_called_once()
    rc.assert_called_once()


@pytest.mark.parametrize(
    ("install", "url", "sha256"),
    [
        ("install_windows_nsis", "WINDOWS_NSIS_URL", "WINDOWS_NSIS_SHA256"),
        ("install_windows_vulkan", "WINDOWS_VULKAN_URL", "WINDOWS_VULKAN_SHA256"),
    ],
)
def test_windows_installers_are_pinned_and_verified(install: str, url: str, sha256: str) -> None:
    pinned_url = getattr(_windows, url)
    assert "latest" not in pinned_url
    with patch.object(_windows._c, "download_file", return_value=False) as dl:
        assert getattr(_windows, install)() is False
    assert dl.call_args.args[0] == pinned_url
    assert dl.call_args.kwargs["expected_sha256"] == getattr(_windows, sha256)


def test_install_windows_gstreamer(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("PROCESSOR_ARCHITECTURE", "AMD64")
    monkeypatch.setattr(_windows, "WINDOWS_GSTREAMER_PREFIX", str(tmp_path))
    with (
        patch.object(_windows._c, "download_file", side_effect=[False, True]) as dl,
        patch.object(_windows._c, "run_command", return_value=True) as rc,
        patch.object(_windows._c, "set_env_var"),
        patch.object(_windows._c, "add_to_path"),
    ):
        assert _windows.install_windows_gstreamer("1.28.4") is True

    installer = dl.call_args_list[0].args[1]
    assert installer.name == "gstreamer-1.0-msvc-x86_64-1.28.4.exe"
    assert [call.args[0] for call in dl.call_args_list] == [
        (
            "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows/"
            "gstreamer-1.0-msvc-x86_64-1.28.4.exe"
        ),
        (
            "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.4/msvc/"
            "gstreamer-1.0-msvc-x86_64-1.28.4.exe"
        ),
    ]
    expected_sha256 = _windows._c.pinned_checksum("1.28.4", "windows_msvc_x64")
    assert dl.call_args_list[0].kwargs == {
        "warn_on_failure": True,
        "expected_sha256": expected_sha256,
    }
    assert dl.call_args_list[1].kwargs == {
        "warn_on_failure": False,
        "expected_sha256": expected_sha256,
    }
    assert rc.call_args.args[0] == [
        str(installer),
        "/VERYSILENT",
        "/SUPPRESSMSGBOXES",
        "/SP-",
        "/NORESTART",
        "/TYPE=devel",
        f"/DIR={_windows.WINDOWS_GSTREAMER_PREFIX}",
    ]


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
        patch.object(_windows, "_msvc_preinstalled", return_value=False),
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


def test_install_windows_msvc_skips_preinstalled_toolchain() -> None:
    with (
        patch.object(_windows, "_msvc_preinstalled", return_value=True) as preinstalled,
        patch.object(_windows, "_windows_sdk_installed", return_value=True) as sdk,
        patch.object(_windows._c, "download_file") as dl,
    ):
        assert _windows.install_windows_msvc(dry_run=False, arm64=True) is True
    preinstalled.assert_called_once_with(True)
    sdk.assert_called_once_with(True)
    dl.assert_not_called()


def test_install_windows_msvc_installs_when_compiler_lacks_sdk() -> None:
    with (
        patch.object(_windows, "_msvc_preinstalled", return_value=True),
        patch.object(_windows, "_windows_sdk_installed", return_value=False),
        patch.object(_windows._c, "download_file", return_value=False) as dl,
    ):
        assert _windows.install_windows_msvc(dry_run=False, arm64=False) is False
    dl.assert_called_once()


def _write_windows_sdk(root: Path, version: str, targets: list[str], rc: bool = True) -> None:
    kits = root / "Windows Kits" / "10"
    (kits / "Include" / version / "um").mkdir(parents=True)
    (kits / "Include" / version / "um" / "Windows.h").write_text("")
    if rc:
        (kits / "bin" / version / "x64").mkdir(parents=True)
        (kits / "bin" / version / "x64" / "rc.exe").write_text("")
    for target in targets:
        (kits / "Lib" / version / "um" / target).mkdir(parents=True)
        (kits / "Lib" / version / "um" / target / "kernel32.Lib").write_text("")


@pytest.mark.parametrize(
    ("targets", "rc", "arm64", "expected"),
    [
        (["x64"], True, False, True),
        (["x64"], True, True, False),
        (["x64", "arm64"], True, True, True),
        (["x64"], False, False, False),
    ],
)
def test_windows_sdk_installed_requires_headers_rc_and_target_libraries(
    monkeypatch, tmp_path: Path, targets: list[str], rc: bool, arm64: bool, expected: bool
) -> None:
    _write_windows_sdk(tmp_path, "10.0.22621.0", targets, rc)
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    assert _windows._windows_sdk_installed(arm64) is expected


def test_windows_sdk_installed_false_without_kits(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    assert _windows._windows_sdk_installed(arm64=False) is False


@pytest.mark.parametrize(("stdout", "expected"), [("C:\\VS\n", True), ("", False)])
def test_msvc_preinstalled_requires_all_components_in_one_instance(
    monkeypatch, tmp_path: Path, stdout: str, expected: bool
) -> None:
    vswhere = tmp_path / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    vswhere.parent.mkdir(parents=True)
    vswhere.write_text("")
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    with patch.object(_windows._c, "run_captured", return_value=completed(stdout)) as run:
        assert _windows._msvc_preinstalled(arm64=True) is expected
    args = run.call_args.args[0]
    version = args.index("-version")
    assert args[version + 1] == "[17.0,"
    requires = args.index("-requires")
    assert args[requires + 1 : requires + 3] == [
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "Microsoft.VisualStudio.Component.VC.Tools.ARM64",
    ]


def test_msvc_preinstalled_false_without_vswhere(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("PROGRAMFILES(X86)", str(tmp_path))
    with patch.object(_windows._c, "run_captured") as run:
        assert _windows._msvc_preinstalled(arm64=False) is False
    run.assert_not_called()


def test_install_windows_msvc_verification_failure() -> None:
    with (
        patch.object(_windows, "_msvc_preinstalled", return_value=False),
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
    with patch.object(_windows._c, "run_captured", return_value=completed()):
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


@pytest.mark.parametrize(
    ("platform", "refresh", "clean"),
    [
        ("fedora", None, ["dnf", "clean", "all"]),
        ("arch", ["pacman", "-Syu", "--noconfirm"], ["pacman", "-Sc", "--noconfirm"]),
    ],
)
def test_install_linux_refreshes_installs_builds_then_cleans(platform, refresh, clean) -> None:
    installer = MagicMock(return_value=True)
    manager = _linux.PACKAGE_MANAGERS[platform]
    with (
        patch.dict(_linux.PACKAGE_MANAGERS, {platform: manager._replace(install=installer)}),
        patch.object(_linux._c, "install_build_tools", return_value=True) as build_tools,
        patch.object(_linux._c, "run_command", return_value=True) as run_command,
    ):
        assert _linux.install_linux(platform) is True
    installer.assert_called_once_with(get_packages(platform), False, sudo=True)
    build_tools.assert_called_once_with(False)
    assert [entry.args[0] for entry in run_command.call_args_list] == [
        *([refresh] if refresh else []),
        clean,
    ]


@pytest.mark.parametrize(
    ("platform", "install"),
    [("fedora", "run_dnf_install_with_retry"), ("arch", "run_pacman_install_with_retry")],
)
def test_package_managers_install_with_retry(platform, install) -> None:
    assert _linux.PACKAGE_MANAGERS[platform].install is getattr(_linux._c, install)


@pytest.mark.parametrize("platform", ["fedora", "arch"])
def test_install_linux_category_skips_build_tools_and_cleanup(platform) -> None:
    manager = _linux.PACKAGE_MANAGERS[platform]
    with (
        patch.dict(
            _linux.PACKAGE_MANAGERS, {platform: manager._replace(install=lambda *a, **k: True)}
        ),
        patch.object(_linux._c, "install_build_tools") as build_tools,
        patch.object(_linux._c, "run_command", return_value=True) as run_command,
    ):
        assert _linux.install_linux(platform, category="gstreamer") is True
    build_tools.assert_not_called()
    assert manager.clean not in [entry.args[0] for entry in run_command.call_args_list]


@pytest.mark.parametrize("platform", ["fedora", "arch"])
def test_install_linux_stops_when_the_package_manager_fails(platform) -> None:
    manager = _linux.PACKAGE_MANAGERS[platform]
    with (
        patch.dict(
            _linux.PACKAGE_MANAGERS, {platform: manager._replace(install=lambda *a, **k: False)}
        ),
        patch.object(_linux._c, "install_build_tools") as build_tools,
        patch.object(_linux._c, "run_command", return_value=True),
    ):
        assert _linux.install_linux(platform) is False
    build_tools.assert_not_called()


def test_install_linux_unknown_category_returns_false() -> None:
    with patch.object(_linux._c, "run_command") as run_command:
        assert _linux.install_linux("arch", category="bogus") is False
    run_command.assert_not_called()


def test_install_arch_aborts_when_sync_fails() -> None:
    manager = _linux.PACKAGE_MANAGERS["arch"]
    with (
        patch.dict(
            _linux.PACKAGE_MANAGERS, {"arch": manager._replace(install=lambda *a, **k: True)}
        ),
        patch.object(_linux._c, "run_command", return_value=False),
        patch.object(_linux._c, "install_build_tools") as build_tools,
    ):
        assert _linux.install_linux("arch") is False
    build_tools.assert_not_called()
