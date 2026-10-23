"""Source contracts shared by QGC's native package generators."""

from __future__ import annotations

import hashlib
import json
import os
import re
import shlex
import subprocess

import pytest
from ci.compile import docker_helper
from ci.publish import attest_helper

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow

CPACK_DIR = REPO_ROOT / "cmake/install/CPack"


def test_optional_generators_use_cpack_names_and_runtime_component() -> None:
    for module, generator in (
        ("Bundle", "Bundle"),
        ("ProductBuild", "productbuild"),
        ("IFW", "IFW"),
    ):
        content = (CPACK_DIR / f"CreateCPack{module}.cmake").read_text()
        assert f'set(CPACK_GENERATOR "{generator}")' in content
    ifw = (CPACK_DIR / "CreateCPackIFW.cmake").read_text()
    assert "cpack_ifw_configure_component(Runtime" in ifw
    script = re.search(r'SCRIPT "\$\{CMAKE_SOURCE_DIR\}/([^"]+)"', ifw)
    assert script and (REPO_ROOT / script.group(1)).is_file()
    assert 'NOT "$ENV{QT_ROOT_DIR}" STREQUAL ""' in ifw
    assert "NOT DEFINED CPACK_IFW_ROOT" in ifw
    assert not (REPO_ROOT / "cmake/install/CreateQGCInstaller.cmake").exists()


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


def test_arch_package_depends_on_system_gstreamer_plugins_when_not_bundled(tmp_path) -> None:
    plugin_packages = {"gst-plugins-good", "gst-plugins-bad-libs", "gst-libav"}
    for bundle, expected in (("ON", set()), ("OFF", plugin_packages)):
        source_dir = tmp_path / bundle / "source"
        build_dir = tmp_path / bundle / "build"
        (source_dir / "deploy/linux").mkdir(parents=True)
        (source_dir / "deploy/linux/PKGBUILD.in").write_text(
            (REPO_ROOT / "deploy/linux/PKGBUILD.in").read_text()
        )
        (source_dir / "CMakeLists.txt").write_text(
            f"""
cmake_minimum_required(VERSION 3.25)
project(QGroundControl LANGUAGES NONE HOMEPAGE_URL "https://qgroundcontrol.com")
add_custom_target(QGroundControl)
set(QGC_APP_VERSION_STR v5.0.0)
set(QGC_ENABLE_GST_VIDEOSTREAMING ON)
set(QGC_GST_BUNDLE_PLUGINS {bundle})
include("{(REPO_ROOT / "cmake/install/CreateArchPackage.cmake").as_posix()}")
""".lstrip()
        )

        result = subprocess.run(
            ["cmake", "-S", str(source_dir), "-B", str(build_dir)],
            check=False,
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0, result.stderr

        pkgbuild = (build_dir / "arch-pkg/PKGBUILD").read_text()
        depends = re.search(r"^depends=\((.*)\)$", pkgbuild, re.MULTILINE)
        assert depends, pkgbuild
        packages = set(re.findall(r"'([^']+)'", depends.group(1)))
        assert {"gstreamer", "gst-plugins-base-libs"} <= packages
        assert packages & plugin_packages == expected, bundle


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
    steps = get_steps(load_workflow("windows.yml"), "build")

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
    assert "DisableX64FSRedirection" in nsis
    assert "EnableX64FSRedirection" in nsis
    assert "QGCCrashDumps" in nsis
    assert "GPU Safe Mode" in nsis
    assert "set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL OFF)" in nsis

    assert get_step(steps, "Checkout repo")["with"]["fetch-depth"] == 0
    assert "-DQGC_CPACK_GENERATOR=NSIS" in get_step(steps, "Configure")["with"]["extra-args"]
    assert get_step(steps, "Create Installer")["with"]["target"] == "qgc-package"
    assert "./.github/actions/cmake-install" not in [step.get("uses") for step in steps]
    installer_test = (REPO_ROOT / "deploy/windows/verify-installer.ps1").read_text()
    install = get_step(steps, "Install, upgrade, and verify installer")
    assert get_step(steps, "Uninstall and verify cleanup")["run"].endswith("-Phase Uninstall")
    assert "[Environment+SpecialFolder]::ApplicationData" in installer_test
    assert "APPDATA: ${{ runner.temp }}\\qgc-appdata" not in workflow
    # attest-and-upload's prepare verifies (or writes) the CPack sidecar at its default
    # artifact path, which is the installer the install test runs.
    attest = get_step(steps, "Attest and Upload")
    assert (
        install["env"]["INSTALLER_PATH"] == "${{ runner.temp }}\\build\\${{ matrix.package }}.exe"
    )
    assert attest["with"]["artifact-name"] == "${{ matrix.package }}.exe"
    assert "artifact-source-path" not in attest["with"]
    assert "Get-FileHash" not in workflow
    assert attest["with"]["subject-name"] == "${{ matrix.package }}-windows"
    upload_steps = get_steps(load_action("attest-and-upload"))
    prepare = get_step(upload_steps, "prepare")
    assert prepare["env"]["ARTIFACT_NAME"] == "${{ inputs.artifact-name }}"  # in $RUNNER_TEMP/build
    assert 'attest_helper.py" prepare' in prepare["run"]
    upload = get_step(upload_steps, "Upload artifact and checksum to AWS")
    assert upload["env"]["CHECKSUM_PATH"] == "${{ steps.prepare.outputs.checksum }}"
    assert '--artifact "${CHECKSUM_PATH}:${ARTIFACT_NAME}.sha256"' in upload["run"]
    assert "InstallLocation mismatch" in installer_test
    assert "Windows Error Reporting registry key not found" in installer_test
    assert "QGroundControl (GPU Safe Mode).lnk" in installer_test
    assert "Start Menu directory remains after uninstall" in installer_test


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
project(QGroundControl VERSION 5.0.0 DESCRIPTION "QGC" HOMEPAGE_URL "https://qgroundcontrol.com"
        LANGUAGES NONE)
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
        assert "DisableX64FSRedirection" in config
        assert "EnableX64FSRedirection" in config
        assert "QGCCrashDumps" in config
        assert "GPU Safe Mode" in config


def test_docker_workflow_requires_and_validates_native_packages() -> None:
    steps = get_steps(load_workflow("docker.yml"), "build")

    assert "find_artifact.py" in get_step(steps, "artifact")["run"]
    package = get_step(steps, "package")
    assert "find_artifact.py" in package["run"]
    assert "--top-level" in package["run"]
    lifecycle = get_step(steps, "Validate native package lifecycle")
    assert lifecycle["env"]["PACKAGE_PATH"] == "${{ steps.package.outputs.path }}"
    checksum = get_step(steps, "Verify native package checksum")
    assert checksum["id"] == "package-checksum"
    upload = get_step(steps, "Upload native package")
    assert "${{ steps.package-checksum.outputs.checksum }}" in upload["with"]["path"].splitlines()


def _docker_step_argv(name: str, env: dict[str, str]) -> tuple[list[str], str]:
    """Expand a docker.yml build step's `python3 <script> ...` command; return its argv and output file."""
    step = get_step(get_steps(load_workflow("docker.yml"), "build"), name)
    values = {"GITHUB_WORKSPACE": str(REPO_ROOT), **env}
    assert set(step.get("env", {})) | {"GITHUB_WORKSPACE"} >= set(
        re.findall(r"\$\{(\w+)\}", step["run"])
    )
    command, _, output = step["run"].partition(" > ")
    argv = shlex.split(re.sub(r"\$\{(\w+)\}", lambda match: values[match[1]], command))
    assert argv[0] == "python3"
    return argv[1:], output.strip()


def _builder_command(monkeypatch, argv: list[str]) -> list[str]:
    """Run docker_helper.py's argv and return the docker command it would run."""
    assert argv[0] == f"{REPO_ROOT}/tools/ci/compile/docker_helper.py"
    commands = []

    def run(command, **_):
        commands.append(command)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setenv("GITHUB_WORKSPACE", str(REPO_ROOT))
    monkeypatch.setattr(docker_helper.subprocess, "run", run)
    assert docker_helper.main(argv[1:]) == 0
    (command,) = commands
    return command


def test_docker_package_validation_installs_as_root_in_the_builder(monkeypatch) -> None:
    step = get_step(
        get_steps(load_workflow("docker.yml"), "build"), "Validate native package lifecycle"
    )
    assert step["env"]["IMAGE"] == "${{ steps.docker-build.outputs.image }}"
    package = REPO_ROOT / "build/qgc test.deb"
    argv, output = _docker_step_argv(
        "Validate native package lifecycle",
        {"IMAGE": "qgc-test:latest", "PACKAGE_PATH": str(package)},
    )
    assert output == ""
    assert _builder_command(monkeypatch, argv) == [
        "docker",
        "run",
        "--rm",
        "--entrypoint",
        "/opt/qgc-venv/bin/python",
        "-v",
        f"{REPO_ROOT}:/project/source:ro",
        "qgc-test:latest",
        "/project/source/tools/ci/packaging/validate_native_package.py",
        "--install",
        "/project/source/build/qgc test.deb",
    ]


@pytest.mark.parametrize(
    ("name", "output", "args"),
    [
        ("Generate CPM dependency SBOM", "cpm-sbom.cdx.json", []),
        ("Generate CPM dependency snapshot", "cpm-sbom.spdx.json", ["--format", "spdx"]),
    ],
)
def test_docker_cpm_sboms_read_the_build_tree_where_it_was_built(
    monkeypatch, name: str, output: str, args: list[str]
) -> None:
    step = get_step(get_steps(load_workflow("docker.yml"), "build"), name)
    assert step["if"].startswith("github.event_name != 'pull_request'")
    assert step["env"]["IMAGE"] == "${{ steps.docker-build.outputs.image }}"
    argv, redirect = _docker_step_argv(name, {"IMAGE": "qgc-test:latest"})
    assert redirect == output
    command = _builder_command(monkeypatch, argv)
    assert command[command.index("--user") + 1] == f"{os.getuid()}:{os.getgid()}"
    assert command[command.index("qgc-test:latest") - 4 : command.index("qgc-test:latest")] == [
        "-v",
        f"{REPO_ROOT}:/project/source:ro",
        "-v",
        f"{REPO_ROOT}/build:/project/build:ro",
    ]
    assert command[command.index("qgc-test:latest") + 1 :] == [
        "/project/source/tools/ci/security/generate_cpm_sbom.py",
        "--build-dir",
        "/project/build",
        *args,
        "--require-components",
    ]


def test_docker_package_checksum_requires_cpack_sidecars_and_writes_arch(
    tmp_path, gh_output
) -> None:
    def run(package):
        gh_output.write_text("")
        argv, _ = _docker_step_argv(
            "Verify native package checksum", {"PACKAGE_PATH": str(package)}
        )
        assert argv[0] == f"{REPO_ROOT}/tools/ci/publish/attest_helper.py"
        try:
            attest_helper.main(argv[1:])
        except SystemExit as exit_:
            return exit_.code, gh_output.read_text()
        return 0, gh_output.read_text()

    packages = tmp_path / "build dir"
    packages.mkdir()
    for name in ("qgroundcontrol_5.0.0_amd64.deb", "qgroundcontrol-5.0.0-1.x86_64.rpm"):
        package = packages / name
        package.write_bytes(name.encode())
        sidecar = packages / f"{name}.sha256"

        assert run(package) == (1, "")
        assert not sidecar.exists()

        cpack_sidecar = f"{hashlib.sha256(name.encode()).hexdigest()}  {name}\n"
        sidecar.write_text(cpack_sidecar)
        assert run(package) == (0, f"checksum={sidecar}\n")
        assert sidecar.read_text() == cpack_sidecar

        sidecar.write_text(f"{'0' * 64}  {name}\n")
        assert run(package) == (1, "")

    arch = packages / "qgroundcontrol-5.0.0-1-x86_64.pkg.tar.zst"
    arch.write_bytes(b"arch")
    sidecar = packages / f"{arch.name}.sha256"
    assert run(arch) == (0, f"checksum={sidecar}\n")
    assert sidecar.read_text() == f"{hashlib.sha256(b'arch').hexdigest()}  {arch.name}\n"


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
