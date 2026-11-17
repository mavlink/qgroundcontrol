"""Source contracts shared by QGC's native package generators."""

from __future__ import annotations

import hashlib
import os
import re
import shlex
import subprocess

import pytest
from ci.compile import docker_helper
from ci.publish import attest_helper

from ..._helpers import REPO_ROOT, get_step, get_steps, load_workflow

CPACK_DIR = REPO_ROOT / "cmake/install/CPack"


def test_every_cpack_generator_shares_the_common_package_metadata() -> None:
    generators = sorted(CPACK_DIR.glob("CreateCPack*.cmake"))
    generators.remove(CPACK_DIR / "CreateCPackCommon.cmake")
    assert generators
    for generator in generators:
        contents = generator.read_text()
        assert "include(CreateCPackCommon)" in contents, generator.name
        assert "include(CPack)" in contents, generator.name


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


@pytest.mark.parametrize("recovers", [True, False])
def test_macos_dmg_retries_once_after_detaching_a_lost_interstitial_image(
    tmp_path, recovers: bool
) -> None:
    build_dir = tmp_path / "build"
    bundle = tmp_path / "staging/QGroundControl.app"
    (bundle / "Contents").mkdir(parents=True)
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    calls = tmp_path / "calls.log"
    create_dmg = bin_dir / "create-dmg"
    # Like create-dmg's APFS race: the image attaches, then its mount cannot be found.
    create_dmg.write_text(
        f"""#!/bin/bash
echo create-dmg >> {shlex.quote(str(calls))}
if [[ {str(recovers).lower()} == true ]] && grep -q failed {shlex.quote(str(calls))}; then
    touch QGroundControl.dmg
    exit 0
fi
echo failed >> {shlex.quote(str(calls))}
touch "rw.$$.QGroundControl.dmg"
echo "Device name:     /dev/disk9s1"
echo "ERROR: the interstitial disk image was not found." >&2
exit 1
"""
    )
    hdiutil = bin_dir / "hdiutil"
    hdiutil.write_text(f'#!/bin/bash\necho "hdiutil $*" >> {shlex.quote(str(calls))}\n')
    for program in (create_dmg, hdiutil):
        program.chmod(0o755)
    build_dir.mkdir()

    result = subprocess.run(
        [
            "cmake",
            f"-DQGC_STAGING_BUNDLE_PATH={bundle}",
            f"-DCREATE_DMG_PROGRAM={create_dmg}",
            "-P",
            str(REPO_ROOT / "cmake/install/CreateMacDMG.cmake"),
        ],
        cwd=build_dir,
        env={**os.environ, "PATH": f"{bin_dir}{os.pathsep}{os.environ['PATH']}"},
        check=False,
        capture_output=True,
        text=True,
    )

    log = calls.read_text().splitlines()
    assert log.count("create-dmg") == 2
    assert "hdiutil detach /dev/disk9 -force" in log
    leftovers = sorted(path.name for path in build_dir.glob("rw.*"))
    if recovers:
        assert result.returncode == 0, result.stderr
        assert (build_dir / "QGroundControl.dmg").is_file()
        # The first attempt's read-write image is removed before the retry.
        assert leftovers == []
    else:
        assert result.returncode != 0
        assert "create-dmg failed" in result.stderr


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
