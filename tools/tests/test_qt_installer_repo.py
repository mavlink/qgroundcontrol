"""Tests for tools/setup/qt_installer/repo.py: repository layout and package selection.

The fixtures are Qt 6.12.0 Updates.xml files from download.qt.io, trimmed to the base packages and
a few addons (qtlocation, qtquick3d, qtshadertools, qthttpserver, qtwebsockets, qtserialport).
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from setup.qt_installer import repo
from setup.qt_installer.repo import (
    QtRequest,
    effective_host,
    host_desktop_arch,
    is_cross_arch,
    resolve_plan,
)

from ._helpers import TOOLS_DIR

if TYPE_CHECKING:
    from syrupy.assertion import SnapshotAssertion

pytestmark = pytest.mark.windows

FIXTURES = TOOLS_DIR / "tests" / "fixtures" / "qt_installer"
MODULES = (
    "qtlocation",
    "qtquick3d",
    "qtshadertools",
    "qthttpserver",
    "qtwebsockets",
    "qtserialport",
)
IOS_MODULES = tuple(module for module in MODULES if module != "qtserialport")
ROOT = "online/qtsdkrepository"

# Every combination CI installs: (host, target, arch) -> (Updates.xml directory, arch directory).
COMBINATIONS = {
    ("linux", "desktop", "linux_gcc_64"): ("linux_x64/desktop/qt6_6120/qt6_6120", "gcc_64"),
    ("linux_arm64", "desktop", "linux_gcc_arm64"): (
        "linux_arm64/desktop/qt6_6120/qt6_6120",
        "gcc_arm64",
    ),
    ("mac", "desktop", "clang_64"): ("mac_x64/desktop/qt6_6120/qt6_6120", "macos"),
    ("mac", "ios", "ios_device"): ("mac_x64/ios/qt6_6120/qt6_6120_device", "ios_device"),
    ("mac", "ios", "ios_simulator_x86_64"): (
        "mac_x64/ios/qt6_6120/qt6_6120_simulator_x86_64",
        "ios_simulator_x86_64",
    ),
    ("mac", "ios", "ios_simulator_arm64"): (
        "mac_x64/ios/qt6_6120/qt6_6120_simulator_arm64",
        "ios_simulator_arm64",
    ),
    ("windows", "desktop", "win64_msvc2022_64"): (
        "windows_x86/desktop/qt6_6120/qt6_6120_msvc2022_64",
        "msvc2022_64",
    ),
    ("windows", "desktop", "win64_msvc2022_arm64_cross_compiled"): (
        "windows_x86/desktop/qt6_6120/qt6_6120_msvc2022_arm64_cross_compiled",
        "msvc2022_arm64",
    ),
    ("windows_arm64", "desktop", "win64_msvc2022_arm64"): (
        "windows_arm64/desktop/qt6_6120/qt6_6120",
        "msvc2022_arm64",
    ),
    ("linux", "android", "android_arm64_v8a"): (
        "all_os/android/qt6_6120/qt6_6120_arm64_v8a",
        "android_arm64_v8a",
    ),
    ("mac", "android", "android_arm64_v8a"): (
        "all_os/android/qt6_6120/qt6_6120_arm64_v8a",
        "android_arm64_v8a",
    ),
    ("windows", "android", "android_arm64_v8a"): (
        "all_os/android/qt6_6120/qt6_6120_arm64_v8a",
        "android_arm64_v8a",
    ),
    ("all_os", "android", "android_arm64_v8a"): (
        "all_os/android/qt6_6120/qt6_6120_arm64_v8a",
        "android_arm64_v8a",
    ),
    ("linux", "android", "android_armv7"): (
        "all_os/android/qt6_6120/qt6_6120_armv7",
        "android_armv7",
    ),
    ("linux", "android", "android_x86_64"): (
        "all_os/android/qt6_6120/qt6_6120_x86_64",
        "android_x86_64",
    ),
}
# One fixture per repository directory; Android's other hosts share the linux entries' fixtures.
FIXTURE_COMBINATIONS = [
    key
    for key in COMBINATIONS
    if key[0] != "all_os"
    and not (key[1] == "android" and key[0] != "linux")
    and key[2] != "ios_simulator_arm64"
]


def _request(host: str, target: str, arch: str, **kwargs) -> QtRequest:
    return QtRequest(host, target, "6.12.0", arch, **kwargs)


def _updates(request: QtRequest) -> bytes:
    name = request.repo_dir.removeprefix(f"{ROOT}/").replace("/", "_")
    return (FIXTURES / f"{name}.Updates.xml").read_bytes()


def _modules(target: str) -> tuple[str, ...]:
    return IOS_MODULES if target == "ios" else MODULES


@pytest.mark.parametrize(("host", "target", "arch"), list(COMBINATIONS))
def test_every_ci_combination_maps_to_its_repository(host: str, target: str, arch: str) -> None:
    directory, arch_dir = COMBINATIONS[(host, target, arch)]
    assert _request(host, target, arch).repo_dir == f"{ROOT}/{directory}"
    assert repo.resolve_arch_dir(arch) == arch_dir


@pytest.mark.parametrize(
    ("arch", "names"),
    [
        ("linux_gcc_64", ("qt.qt6.6120.linux_gcc_64",)),
        ("ios_device", ("qt.qt6.6120.ios_device", "qt.qt6.6120.qtforios.device")),
        (
            "ios_simulator_x86_64",
            ("qt.qt6.6120.ios_simulator_x86_64", "qt.qt6.6120.qtforios.simulator_x86_64"),
        ),
        ("android_armv7", ("qt.qt6.6120.android_armv7",)),
    ],
)
def test_base_package_names_include_the_split_ios_spelling(arch: str, names: tuple) -> None:
    target = (
        "ios" if arch.startswith("ios") else "android" if arch.startswith("android") else "desktop"
    )
    assert _request("mac" if target == "ios" else "linux", target, arch).base_packages() == names


@pytest.mark.parametrize(
    ("host", "target", "arch", "version", "directory"),
    [
        # The Windows x64 desktop repository split per arch in 6.11, iOS per SDK in 6.12.
        (
            "windows",
            "desktop",
            "win64_msvc2022_64",
            "6.10.2",
            "windows_x86/desktop/qt6_6102/qt6_6102",
        ),
        (
            "windows",
            "desktop",
            "win64_msvc2022_64",
            "6.11.0",
            "windows_x86/desktop/qt6_6110/qt6_6110_msvc2022_64",
        ),
        ("mac", "ios", "ios", "6.11.0", "mac_x64/ios/qt6_6110/qt6_6110"),
        ("linux", "android", "android_x86_64", "6.8.3", "all_os/android/qt6_683/qt6_683_x86_64"),
    ],
)
def test_repository_splits_follow_the_qt_version(host, target, arch, version, directory) -> None:
    assert QtRequest(host, target, version, arch).repo_dir == f"{ROOT}/{directory}"


@pytest.mark.parametrize("version", ["6.7.3", "6.12", "6.12.0-beta1", "latest"])
def test_rejects_unsupported_versions(version: str) -> None:
    with pytest.raises(ValueError, match=r"Qt 6\.8\+"):
        QtRequest("linux", "desktop", version, "linux_gcc_64")


def test_rejects_unknown_desktop_hosts() -> None:
    with pytest.raises(ValueError, match="Unknown Qt host"):
        _request("all_os", "desktop", "linux_gcc_64")
    with pytest.raises(ValueError, match="No desktop Qt"):
        host_desktop_arch("solaris", "android_armv7")


@pytest.mark.parametrize(
    ("platform", "expected"), [("linux", "linux"), ("darwin", "mac"), ("win32", "windows")]
)
def test_all_os_resolves_to_the_running_platform(monkeypatch, platform: str, expected: str) -> None:
    monkeypatch.setattr(repo.sys, "platform", platform)
    assert effective_host("all_os") == expected
    assert effective_host("linux_arm64") == "linux_arm64"


@pytest.mark.parametrize(
    ("host", "arch", "desktop"),
    [
        ("linux", "android_arm64_v8a", "linux_gcc_64"),
        ("mac", "ios_device", "clang_64"),
        ("mac", "android_arm64_v8a", "clang_64"),
        # aqt would pick MinGW; QGC installs and builds Android on Windows against MSVC.
        ("windows", "android_arm64_v8a", "win64_msvc2022_64"),
        ("windows", "win64_msvc2022_arm64_cross_compiled", "win64_msvc2022_64"),
    ],
)
def test_host_desktop_arch(host: str, arch: str, desktop: str) -> None:
    assert host_desktop_arch(host, arch) == desktop


def test_cross_arches() -> None:
    assert is_cross_arch("android", "android_armv7")
    assert is_cross_arch("ios", "ios_device")
    assert is_cross_arch("desktop", "win64_msvc2022_arm64_cross_compiled")
    assert not is_cross_arch("desktop", "win64_msvc2022_arm64")


@pytest.mark.parametrize(("host", "target", "arch"), FIXTURE_COMBINATIONS)
def test_resolved_archives(host, target, arch, snapshot: SnapshotAssertion) -> None:
    """Review archive-list changes in the .ambr diff; refresh with --snapshot-update."""
    request = _request(host, target, arch, modules=_modules(target))
    plan = resolve_plan(request, _updates(request))

    prefix = f"{request.repo_dir}/"
    assert all(archive.url_path.startswith(prefix) for archive in plan.archives)
    assert not any("debug" in archive.filename for archive in plan.archives)
    assert plan.unlisted_dependencies == (("qtquick3d", "qtquicktimeline"),)
    assert [f"{a.dest} <- {a.package}/{a.filename}" for a in plan.archives] == snapshot


def test_archive_url_joins_package_version_and_file() -> None:
    request = _request("linux", "desktop", "linux_gcc_64", modules=("qtlocation",))
    archive = resolve_plan(request, _updates(request)).archives[-1]
    assert archive.url_path == (
        f"{ROOT}/linux_x64/desktop/qt6_6120/qt6_6120/qt.qt6.6120.addons.qtlocation.linux_gcc_64/"
        "6.12.0-0-202609280346qtlocation-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z"
    )


@pytest.mark.parametrize(
    ("archives", "expected"),
    [
        # pre-commit's qmllint subset.
        (("qtbase", "qtdeclarative", "icu"), ["qtbase", "qtdeclarative", "icu"]),
        # lupdate's subset; --modules archives are always installed.
        (
            ("qtbase", "qttools", "qtdeclarative", "icu"),
            ["qtbase", "qtdeclarative", "qttools", "icu"],
        ),
    ],
)
def test_archive_subset_filters_only_the_base_package(archives, expected) -> None:
    request = _request(
        "linux", "desktop", "linux_gcc_64", modules=("qtlocation",), archives=archives
    )
    plan = resolve_plan(request, _updates(request))
    names = [archive.filename.split("-", 1)[0] for archive in plan.archives]
    assert names == [*expected, "qtlocation"]
    icu = next(archive for archive in plan.archives if archive.filename.startswith("icu-"))
    assert icu.dest == "6.12.0/gcc_64/lib"


@pytest.mark.parametrize(
    ("modules", "unlisted"),
    [
        (("qthttpserver",), (("qthttpserver", "qtwebsockets"),)),
        (("qthttpserver", "qtwebsockets"), ()),
    ],
)
def test_only_unrequested_addon_dependencies_are_reported(modules, unlisted) -> None:
    request = _request("linux", "desktop", "linux_gcc_64", modules=modules)
    plan = resolve_plan(request, _updates(request))
    assert plan.unlisted_dependencies == unlisted
    assert [archive.filename.split("-", 1)[0] for archive in plan.archives][-len(modules) :] == [
        *modules
    ]


def test_unknown_modules_fail() -> None:
    request = _request("mac", "ios", "ios_device", modules=("qtlocation", "qtserialport", "qtnope"))
    with pytest.raises(LookupError, match="has no modules qtserialport qtnope"):
        resolve_plan(request, _updates(request))


def test_missing_base_package_fails() -> None:
    request = _request("linux", "desktop", "linux_gcc_64")
    with pytest.raises(LookupError, match="No linux_gcc_64 base package"):
        resolve_plan(
            request, b"<Updates><PackageUpdate><Name>other</Name></PackageUpdate></Updates>"
        )


@pytest.mark.parametrize(
    "target_dir", ["@TargetDir@/6.12.0/msvc2022_64", "@TargetDir@/6.12.0/gcc_64/../../..", ""]
)
def test_archives_extracting_outside_the_arch_directory_fail(target_dir: str) -> None:
    operation = (
        f'<Operations><Operation name="Extract"><Argument>{target_dir}</Argument>'
        "<Argument>qtbase.7z</Argument></Operation></Operations>"
        if target_dir
        else ""
    )
    xml = (
        "<Updates><PackageUpdate><Name>qt.qt6.6120.linux_gcc_64</Name><Version>6.12.0-0-1</Version>"
        f"<DownloadableArchives>qtbase.7z</DownloadableArchives>{operation}</PackageUpdate></Updates>"
    )
    with pytest.raises(LookupError, match=r"outside 6\.12\.0/gcc_64"):
        resolve_plan(_request("linux", "desktop", "linux_gcc_64"), xml.encode())
