"""Source contracts shared by QGC's native package generators."""

from __future__ import annotations

import json
import subprocess

from _helpers import REPO_ROOT

CPACK_DIR = REPO_ROOT / "cmake/install/CPack"


def test_cpack_generators_share_runtime_only_metadata_and_checksums() -> None:
    common = (CPACK_DIR / "CreateCPackCommon.cmake").read_text()
    assert "set(CPACK_COMPONENTS_ALL Runtime)" in common
    assert 'set(CPACK_PACKAGE_CHECKSUM "SHA256")' in common
    assert "native package version resolved to 0.0.0" not in common

    generators = sorted(CPACK_DIR.glob("CreateCPack*.cmake"))
    generators.remove(CPACK_DIR / "CreateCPackCommon.cmake")
    assert generators
    for generator in generators:
        contents = generator.read_text()
        assert "include(CreateCPackCommon)" in contents, generator.name
        assert "include(CPack)" in contents, generator.name


def test_linux_native_generators_use_stable_identity_and_finalization() -> None:
    deb = (CPACK_DIR / "CreateCPackDeb.cmake").read_text()
    rpm = (CPACK_DIR / "CreateCPackRPM.cmake").read_text()
    arch = (REPO_ROOT / "cmake/install/CreateArchPackage.cmake").read_text()

    assert 'set(CPACK_DEBIAN_PACKAGE_NAME "qgroundcontrol")' in deb
    assert 'set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")' in deb
    assert "FinalizeNativePackage.cmake" in deb
    assert 'set(CPACK_RPM_PACKAGE_NAME "qgroundcontrol")' in rpm
    assert 'set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")' in rpm
    assert "FinalizeNativePackage.cmake" in rpm
    assert "makepkg -f --noconfirm --nodeps --skipinteg" in " ".join(arch.split())
    assert "||" not in arch
    assert "ValidatePackageVersion.cmake" in arch


def test_package_version_guard_runs_only_from_package_targets() -> None:
    source_dir = REPO_ROOT / "cmake/install"
    common = CPACK_DIR / "CreateCPackCommon.cmake"
    validator = source_dir / "ValidatePackageVersion.cmake"
    install = (source_dir / "Install.cmake").read_text()

    configure_result = subprocess.run(
        ["cmake", "-DPROJECT_VERSION=0.0.0", "-P", str(common)],
        check=False,
        capture_output=True,
        text=True,
    )
    assert configure_result.returncode == 0, configure_result.stderr
    assert "ValidatePackageVersion.cmake" in install

    for version, expected_status in (("0.0.0", 1), ("v0.0.0-12-gabcdef", 1), ("5.0.3", 0)):
        result = subprocess.run(
            ["cmake", f"-DQGC_NATIVE_PACKAGE_VERSION={version}", "-P", str(validator)],
            check=False,
            capture_output=True,
            text=True,
        )
        assert (result.returncode == 0) is (expected_status == 0)
        if expected_status:
            assert "Fetch Git history and tags before building qgc-package" in " ".join(
                result.stderr.split()
            )


def test_linux_package_defaults_do_not_leak_into_mobile_cross_compiles() -> None:
    install = (REPO_ROOT / "cmake/install/Install.cmake").read_text()
    for preset_file, base_name in (("Android.json", "android-base"), ("iOS.json", "ios-base")):
        presets = json.loads((REPO_ROOT / "cmake/presets" / preset_file).read_text())[
            "configurePresets"
        ]
        base = next(preset for preset in presets if preset["name"] == base_name)
        assert base["cacheVariables"]["QGC_CPACK_GENERATOR"] == ""

    assert "if(NOT QGC_BUILD_INSTALLER OR ANDROID OR IOS)" in install


def test_macos_cpack_staging_excludes_standalone_dmg_creation() -> None:
    install = (REPO_ROOT / "cmake/install/Install.cmake").read_text()
    productbuild = (CPACK_DIR / "CreateCPackProductBuild.cmake").read_text()

    assert 'CreateMacDMG.cmake" COMPONENT dmg' in install
    assert 'set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")' in productbuild
