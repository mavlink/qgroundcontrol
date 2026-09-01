"""Source contracts shared by QGC's native package generators."""

from __future__ import annotations

import json
import re
import subprocess

from _helpers import REPO_ROOT

CPACK_DIR = REPO_ROOT / "cmake/install/CPack"


def test_cpack_generators_share_runtime_only_metadata_and_checksums() -> None:
    common = (CPACK_DIR / "CreateCPackCommon.cmake").read_text()
    assert "set(CPACK_COMPONENTS_ALL Runtime)" in common
    assert 'set(CPACK_PACKAGE_CHECKSUM "SHA256")' in common
    assert "ValidatePackageVersion.cmake" in common
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


def test_package_version_guard_covers_target_and_cpack_entrypoints() -> None:
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

    for variable in ("QGC_NATIVE_PACKAGE_VERSION", "CPACK_PACKAGE_VERSION"):
        for version, expected_status in (("0.0.0", 1), ("v0.0.0-12-gabcdef", 1), ("5.0.3", 0)):
            result = subprocess.run(
                ["cmake", f"-D{variable}={version}", "-P", str(validator)],
                check=False,
                capture_output=True,
                text=True,
            )
            assert (result.returncode == 0) is (expected_status == 0)
            if expected_status:
                assert "Fetch Git history and tags before building a native package" in " ".join(
                    result.stderr.split()
                )


def test_windows_installer_uses_only_the_cpack_nsis_path() -> None:
    install_dir = REPO_ROOT / "cmake/install"
    install = (install_dir / "Install.cmake").read_text()
    nsis = (CPACK_DIR / "CreateCPackNSIS.cmake").read_text()
    workflow = (REPO_ROOT / ".github/workflows/windows.yml").read_text()
    upload_action = (REPO_ROOT / ".github/actions/attest-and-upload/action.yml").read_text()

    assert not (install_dir / "CreateWinInstaller.cmake").exists()
    assert not (REPO_ROOT / "deploy/windows/nullsoft_installer.nsi").exists()
    assert 'elseif(WIN32 AND QGC_CPACK_GENERATOR STREQUAL "")' in install
    assert 'set(QGC_CPACK_GENERATOR "NSIS")' in install
    assert "CreateWinInstaller.cmake" not in install
    assert "nullsoft_installer.nsi" not in install

    assert 'set(CPACK_GENERATOR "NSIS")' in nsis
    assert (
        'set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-installer-${_qgc_nsis_arch}")' in nsis
    )
    assert "CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS" in nsis
    assert "-LEAVE_DATA=1" in nsis
    assert "QGCCrashDumps" in nsis
    assert "GPU Safe Mode" in nsis
    assert "set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL OFF)" in nsis

    assert "fetch-depth: 0" in workflow
    assert "-DQGC_CPACK_GENERATOR=NSIS" in workflow
    assert "target: qgc-package" in workflow
    assert "uses: ./.github/actions/cmake-install" not in workflow
    assert "Install, upgrade, and verify installer" in workflow
    assert "Uninstall and verify cleanup" in workflow
    assert "[Environment+SpecialFolder]::ApplicationData" in workflow
    assert "APPDATA: ${{ runner.temp }}\\qgc-appdata" not in workflow
    assert "Verify installer checksum" in workflow
    assert "Get-FileHash -LiteralPath $installerPath -Algorithm SHA256" in workflow
    assert "$checksumFields = @($checksumLine -split '\\s+', 2)" in workflow
    assert "$checksumInstallerName -cne $installerName" in workflow
    assert "Checksum filename mismatch" in workflow
    assert "additional-artifact-paths: ${{ steps.installer.outputs.checksum_path }}" in workflow
    assert "additional-aws-artifact-name: ${{ matrix.package }}.exe.sha256" in workflow
    assert "additional-aws-artifact-path: ${{ steps.installer.outputs.checksum_path }}" in workflow
    assert "Upload additional artifact to AWS" in upload_action
    assert "artifact-name: ${{ inputs.additional-aws-artifact-name }}" in upload_action
    assert "artifact-path: ${{ inputs.additional-aws-artifact-path }}" in upload_action
    assert "InstallLocation mismatch" in workflow
    assert "Windows Error Reporting registry key not found" in workflow
    assert "QGroundControl (GPU Safe Mode).lnk" in workflow
    assert "Start Menu directory remains after uninstall" in workflow


def test_windows_nsis_module_generates_cpack_config(tmp_path) -> None:
    architectures = (
        ("amd64", "FALSE", "AMD64", "AMD64", "AMD64"),
        ("arm64", "FALSE", "ARM64", "arm64", "ARM64"),
        ("arm64-cross", "TRUE", "AMD64", "arm64", "AMD64-ARM64"),
        ("aarch64-cross", "TRUE", "x86_64", "aarch64", "AMD64-ARM64"),
    )

    for case, cross_compiling, host_processor, target_processor, package_arch in architectures:
        source_dir = tmp_path / case / "source"
        build_dir = tmp_path / case / "build"
        (source_dir / ".github").mkdir(parents=True)
        (source_dir / ".github/COPYING.md").write_text("license")
        (source_dir / "README.md").write_text("readme")
        (source_dir / "payload.txt").write_text("payload")
        (source_dir / "CMakeLists.txt").write_text(
            f"""
cmake_minimum_required(VERSION 3.25)
project(QGroundControl VERSION 5.0.0 DESCRIPTION "QGC" HOMEPAGE_URL "https://qgroundcontrol.com")
list(APPEND CMAKE_MODULE_PATH "{CPACK_DIR.as_posix()}")
set(CMAKE_CROSSCOMPILING {cross_compiling})
set(CMAKE_HOST_SYSTEM_PROCESSOR {host_processor})
set(CMAKE_SYSTEM_PROCESSOR {target_processor})
set(QGC_ORG_NAME QGroundControl)
set(QGC_WINDOWS_ICON_PATH "{(REPO_ROOT / "deploy/windows/WindowsQGC.ico").as_posix()}")
set(QGC_WINDOWS_INSTALL_HEADER_PATH "{(REPO_ROOT / "deploy/windows/installheader.bmp").as_posix()}")
install(FILES payload.txt DESTINATION . COMPONENT Runtime)
include(CreateCPackNSIS)
""".lstrip()
        )

        result = subprocess.run(
            ["cmake", "-S", str(source_dir), "-B", str(build_dir)],
            check=False,
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0, result.stderr

        config = (build_dir / "CPackConfig.cmake").read_text()
        assert 'set(CPACK_GENERATOR "NSIS")' in config
        assert f'set(CPACK_PACKAGE_FILE_NAME "QGroundControl-installer-{package_arch}")' in config
        assert "-LEAVE_DATA=1" in config
        assert "QGCCrashDumps" in config
        assert "GPU Safe Mode" in config


def test_docker_workflow_requires_and_validates_native_packages() -> None:
    workflow = (REPO_ROOT / ".github/workflows/docker.yml").read_text()
    validator = (REPO_ROOT / "deploy/docker/validate-native-package.sh").read_text()

    assert "--top-level" in workflow
    assert workflow.count("--required") >= 2
    assert "validate-native-package.sh" in workflow
    assert "Validate native package lifecycle" in workflow
    assert "Verify native package checksum" in workflow
    assert 'sha256sum --check "${checksum_name}"' in workflow
    assert '"${package_name}" != *.pkg.tar.zst' in workflow
    assert "${{ steps.package-checksum.outputs.path }}" in workflow
    assert "*.deb)" in validator
    assert "*.rpm)" in validator
    assert "*.pkg.tar.zst)" in validator
    assert "test ! -e /opt/QGroundControl" in validator


def test_linux_package_defaults_do_not_leak_into_mobile_cross_compiles() -> None:
    install = (REPO_ROOT / "cmake/install/Install.cmake").read_text()
    gstreamer = (REPO_ROOT / "src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt").read_text()
    for preset_file, base_name in (("Android.json", "android-base"), ("iOS.json", "ios-base")):
        presets = json.loads((REPO_ROOT / "cmake/presets" / preset_file).read_text())[
            "configurePresets"
        ]
        base = next(preset for preset in presets if preset["name"] == base_name)
        assert base["cacheVariables"]["QGC_CPACK_GENERATOR"] == ""

    assert re.search(r"if\(NOT QGC_BUILD_INSTALLER\s+OR ANDROID\s+OR IOS\s*\)", install)
    assert re.search(
        r"elseif\(\s*QGC_BUILD_INSTALLER\s+AND LINUX\s+AND NOT ANDROID\s+"
        r'AND NOT IOS\s+AND QGC_LINUX_DISTRO_FAMILY STREQUAL "arch"\s*\)',
        install,
    )

    runtime_component = "set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)"
    assert runtime_component in install
    assert runtime_component in gstreamer
    assert gstreamer.index(runtime_component) < gstreamer.index("gstreamer_install_platform_sdk")
