"""Exercise deployment orchestration without containers, VMs, or system mutations."""

from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path

import pytest
from _helpers import REPO_ROOT


@pytest.fixture
def module(monkeypatch):
    monkeypatch.syspath_prepend(str(REPO_ROOT / "deploy/docker"))

    def load(path):
        spec = importlib.util.spec_from_file_location("deployment_test", REPO_ROOT / path)
        assert spec is not None and spec.loader is not None
        loaded = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(loaded)
        return loaded

    return load


@pytest.mark.parametrize("variant", ["ubuntu", "fedora", "aarch64", "android"])
def test_docker_cli_uses_variants_without_shell_evaluation(tmp_path, variant):
    binary = tmp_path / "docker"
    binary.write_text(
        f"#!{sys.executable}\nimport json, os, sys\n"
        "with open(os.environ['COMMAND_LOG'], 'a') as stream:\n"
        "    stream.write(json.dumps(sys.argv[1:]) + '\\n')\n"
    )
    binary.chmod(0o755)
    log = tmp_path / "commands"
    subprocess.run(
        [sys.executable, str(REPO_ROOT / "deploy/docker/run_docker.py"), "build", variant],
        env={
            **os.environ,
            "PATH": str(tmp_path),
            "COMMAND_LOG": str(log),
            "BUILD_DIR": str(tmp_path / "output with spaces"),
        },
        check=True,
    )
    build, run = [json.loads(line) for line in log.read_text().splitlines()]
    variants = json.loads((REPO_ROOT / "deploy/docker/variants.json").read_text())["variants"]
    config = next(item for item in variants if item["id"] == variant)
    assert build[build.index("--target") + 1] == config["target"]
    for key, value in config["build_args"].items():
        assert f"{key}={value}" in build
    assert ("/dev/fuse" in run) == config["fuse"]
    assert run[run.index("--user") + 1] == f"{os.getuid()}:{os.getgid()}"
    assert f"{tmp_path / 'output with spaces'}:/project/build" in run
    assert run[-2:] == [config["image"], "Release"]


@pytest.mark.parametrize("target", ["native", "android", "cross"])
@pytest.mark.parametrize("build_type", ["Release", "Debug", "RelWithDebInfo", "MinSizeRel"])
def test_container_entrypoint_builds_the_selected_target(module, monkeypatch, target, build_type):
    script = module("deploy/docker/entrypoint.py")
    monkeypatch.setenv("APPIMAGE_EXTRACT_AND_RUN", "")
    for name in ("ANDROID_SDK_ROOT", "SYSROOT", "CLEAN_BUILD", "JOBS"):
        monkeypatch.delenv(name, raising=False)
    monkeypatch.setenv("QT_HOST_PATH", "/host")
    monkeypatch.setenv("QT_ROOT_DIR", "/target")
    if target == "android":
        for name in ("ANDROID_SDK_ROOT", "ANDROID_NDK_ROOT", "QT_ROOT_DIR_ARM64"):
            monkeypatch.setenv(name, f"/{name}")
        monkeypatch.setenv("ANDROID_PLATFORM", "35")
        monkeypatch.setenv("ANDROID_MIN_SDK", "28")
    if target == "cross":
        monkeypatch.setenv("SYSROOT", "/sysroot")
        monkeypatch.setattr(Path, "is_file", lambda self: True)
    monkeypatch.setenv("JOBS", "3")
    calls = []
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))
    script.build(build_type)
    configure, build = calls[:2]
    assert "-DPython3_EXECUTABLE=/opt/qgc-venv/bin/python" in configure
    assert build[-2:] == ["--parallel", "3"]
    assert build[build.index("--config") + 1] == build_type
    assert os.environ["APPIMAGE_EXTRACT_AND_RUN"] == "1"
    assert any("qgc-package" in call for call in calls) == (target == "native")
    assert any("--install" in call for call in calls) == (target != "android")
    if target == "android":
        assert "-DQT_ANDROID_SIGN_APK=OFF" in configure
        assert ("--preset" in configure) == (build_type in ("Release", "Debug"))
    elif target == "cross":
        assert "-DQGC_AARCH64_SYSROOT=/sysroot" in configure
        assert "-DQGC_CREATE_APPIMAGE=OFF" in configure
    else:
        assert ("--preset" in configure) == (build_type != "MinSizeRel")


def test_sysroot_sources_are_idempotent_and_pin_each_deb822_stanza(module, tmp_path):
    script = module("deploy/docker/install_sysroot_aarch64.py")
    sources = tmp_path / "sources.list.d"
    sources.mkdir()
    legacy = tmp_path / "sources.list"
    legacy.write_text("deb https://example.org noble main\n# untouched\n")
    modern = sources / "ubuntu.sources"
    modern.write_text(
        "Types: deb\nArchitectures: amd64\nURIs: https://one\n\nTypes: deb-src\nURIs: https://two\n"
    )
    script.configure_sources(tmp_path, "noble", "https://ports.example.org/ubuntu")
    first = {
        path: path.read_text() for path in (legacy, modern, sources / "ubuntu-arm64-ports.list")
    }
    script.configure_sources(tmp_path, "noble", "https://ports.example.org/ubuntu")
    assert first == {path: path.read_text() for path in first}
    assert "deb [arch=amd64] https://example.org" in first[legacy]
    assert first[modern].count("Architectures: amd64") == 2
    assert first[sources / "ubuntu-arm64-ports.list"].count("[arch=arm64]") == 3
    assert script.arm64_closure(
        "libc6:arm64\n  Depends: other:arm64\n<virtual:arm64>\nlibc6:arm64\nother:arm64\n"
    ) == ["libc6:arm64", "other:arm64"]
    with pytest.raises(ValueError):
        script.configure_sources(tmp_path, "noble\ninjected", "https://ports.example.org")


def test_sysroot_retry_stops_after_three_attempts(module, monkeypatch):
    script = module("deploy/docker/install_sysroot_aarch64.py")
    calls = []

    def fail(command, **kwargs):
        calls.append(command)
        raise subprocess.CalledProcessError(7, command)

    monkeypatch.setattr(script.subprocess, "run", fail)
    monkeypatch.setattr("common.proc.time.sleep", lambda _: None)
    with pytest.raises(subprocess.CalledProcessError):
        script.run_checked_with_retry(["apt-get", "download", "libc6:arm64"])
    assert len(calls) == 3


def test_missing_container_sdk_stops_before_building(module, monkeypatch):
    script = module("deploy/docker/entrypoint.py")
    monkeypatch.setenv("ANDROID_SDK_ROOT", "/android")
    monkeypatch.delenv("QT_HOST_PATH", raising=False)
    calls = []
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))
    monkeypatch.setattr(script, "initialize_caches", lambda _: None)
    monkeypatch.setattr(script, "write_performance_report", lambda *args: None)
    assert script.main(["Release"]) == 1
    assert not calls


def test_ios_compiler_failure_keeps_metadata_and_cleans_temporary_files(
    module, monkeypatch, tmp_path
):
    script = module("deploy/ios/prepare_bundle.py")
    bundle = tmp_path / "Application.app"
    bundle.mkdir()
    info = bundle / "Info.plist"
    info.write_bytes(b"original metadata")

    def fail(command, **kwargs):
        raise subprocess.CalledProcessError(1, command)

    monkeypatch.setattr(script.subprocess, "run", fail)
    assert script.main([str(bundle), "17.0", "iphoneos"]) == 1
    assert info.read_bytes() == b"original metadata"
    assert list(bundle.iterdir()) == [info]


@pytest.mark.parametrize("suffix", [".deb", ".rpm", ".pkg.tar.zst"])
def test_native_package_uninstalls_after_failed_smoke_test(module, monkeypatch, tmp_path, suffix):
    script = module("deploy/docker/validate_native_package.py")
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        if command[0] == "/usr/bin/QGroundControl":
            raise subprocess.TimeoutExpired(command, 30)
        return subprocess.CompletedProcess(command, 0, "qgroundcontrol 1.0\n")

    monkeypatch.setattr(script.subprocess, "run", run)
    monkeypatch.setattr(script.os, "access", lambda *args: True)
    monkeypatch.setattr(Path, "is_symlink", lambda self: True)
    assert script.main([str(tmp_path / f"qgroundcontrol{suffix}")]) == 1
    assert (
        calls[-1]
        == {
            ".deb": ["apt-get", "remove", "-y", "qgroundcontrol"],
            ".rpm": ["dnf", "remove", "-y", "qgroundcontrol"],
            ".pkg.tar.zst": ["pacman", "-R", "--noconfirm", "qgroundcontrol"],
        }[suffix]
    )


def test_provisioned_vm_build_reuses_environment(module, tmp_path, monkeypatch):
    script = module("deploy/multipass/build_in_vm.py")
    monkeypatch.chdir(tmp_path)
    monkeypatch.setenv("BUILD_DIR", str(tmp_path))
    monkeypatch.setenv("QT_OUT", str(tmp_path / "Qt"))
    monkeypatch.setenv("QGC_PYTHON_ENV", "/opt/qgc-venv")
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    artifact = tmp_path / "QGroundControl.AppImage"
    artifact.touch()
    calls = []
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))
    assert script.main(["--skip-dependencies"]) == 0
    assert calls[0][0] == "/opt/qgc-venv/bin/python"
    assert "tools/setup/install_qt.py" in calls[0]
    assert not any(
        "tools/setup/install_python.py" in call or "tools/setup/install_dependencies" in call
        for call in calls
    )
    assert (tmp_path / "qgc-appimage-path").read_text().strip() == str(artifact)


@pytest.mark.parametrize("ci", [False, True])
def test_vagrant_provisions_as_root_and_builds_as_user(module, tmp_path, monkeypatch, ci):
    script = module("deploy/vagrant/provision.py")
    monkeypatch.setenv("QGC_CI", "1" if ci else "")
    monkeypatch.setenv("QGC_SOURCE_DIR", str(tmp_path))
    monkeypatch.setenv("QT_OUT", "/home/vagrant/Qt with spaces")
    monkeypatch.setattr(Path, "read_text", lambda self: "#user_allow_other\n")
    monkeypatch.setattr(Path, "write_text", lambda self, text: len(text))
    monkeypatch.setattr(Path, "exists", lambda self: True)
    calls = []
    monkeypatch.setattr(
        script.subprocess, "run", lambda command, **kwargs: calls.append((command, kwargs))
    )
    script.provision()
    commands = [command for command, _ in calls]
    build = next(command for command in commands if "--skip-dependencies" in command)
    assert build[:4] == ["runuser", "-u", "vagrant", "--"]
    assert "QT_OUT=/home/vagrant/Qt with spaces" in build
    assert "BUILD_DIR=/home/vagrant/shadow_build" in build
    install = next(
        command for command in commands if command[0] == "apt-get" and "install" in command
    )
    assert ("xubuntu-desktop" in install) == (not ci)
    assert any("rsync" in command and command[0] == "runuser" for command in commands) == (not ci)
    assert not any("su" in command or "-c" in command for command in commands)
    assert calls[0][1]["env"]["DEBIAN_FRONTEND"] == "noninteractive"


@pytest.mark.parametrize("target", ["native", "cross", "android"])
def test_container_compiler_fingerprint_tracks_toolchain(module, monkeypatch, target):
    script = module("deploy/docker/entrypoint.py")
    for name in ("ANDROID_SDK_ROOT", "SYSROOT", "CC", "CXX"):
        monkeypatch.delenv(name, raising=False)
    if target == "cross":
        monkeypatch.setenv("SYSROOT", "/sysroot")
    if target == "android":
        monkeypatch.setenv("ANDROID_SDK_ROOT", "/sdk")
        monkeypatch.setenv("ANDROID_NDK_ROOT", "/ndk")
    calls = []
    version = "one"

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 0, " ".join(command) + version)

    monkeypatch.setattr(script.subprocess, "run", run)
    monkeypatch.setattr(script.shutil, "which", lambda _: None)
    first = script.compiler_fingerprint()
    assert first == script.compiler_fingerprint()
    version = "two"
    assert first != script.compiler_fingerprint()
    expected = {
        "native": "cc",
        "cross": "aarch64-linux-gnu-gcc",
        "android": "/ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang",
    }[target]
    assert calls[0][0] == expected


def test_container_cache_paths_do_not_inherit_host_paths(module, monkeypatch, tmp_path):
    script = module("deploy/docker/entrypoint.py")
    for name in (
        "CCACHE_DIR",
        "CPM_SOURCE_CACHE",
        "MOCCACHE_DIR",
        "CCACHE_BASEDIR",
        "CCACHE_CONFIGPATH",
    ):
        monkeypatch.setenv(name, "/host/only")
    monkeypatch.setenv("CCACHE_MAXSIZE", "3G")
    calls = []
    monkeypatch.setattr(script.shutil, "which", lambda _: "/usr/bin/ccache")
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))
    script.initialize_caches(tmp_path)
    assert os.environ["CCACHE_DIR"] == str(tmp_path / ".ccache")
    assert os.environ["CPM_SOURCE_CACHE"] == str(tmp_path / ".cache/CPM")
    assert os.environ["MOCCACHE_DIR"] == str(tmp_path / ".cache/moccache")
    assert os.environ["CCACHE_MAXSIZE"] == "3G"
    assert calls[0] == ["ccache", "--zero-stats"]


def test_failed_build_keeps_failed_phase_time_and_diagnostics(module, monkeypatch, tmp_path):
    script = module("deploy/docker/entrypoint.py")
    monkeypatch.setenv("ANDROID_SDK_ROOT", "")
    monkeypatch.setenv("SYSROOT", "")
    ticks = iter([0.0, 1.0, 1.0, 4.0])
    monkeypatch.setattr(script.time, "monotonic", lambda: next(ticks))
    timings = {}

    def fail(command, **kwargs):
        if "--build" in command:
            raise subprocess.CalledProcessError(7, command)
        return subprocess.CompletedProcess(command, 0, "cache stats\n", "")

    monkeypatch.setattr(script.subprocess, "run", fail)
    with pytest.raises(subprocess.CalledProcessError):
        script.build("Release", timings)
    script.write_performance_report(tmp_path, tmp_path / "build", timings, False)
    report = json.loads((tmp_path / "build/docker-build-report.json").read_text())
    assert report["seconds"] == {"configure": 1.0, "build": 3.0}
    assert report["success"] is False
    assert report["ccache"] == "cache stats\n"


def test_docker_forwards_budgets_but_not_host_paths(module, monkeypatch, tmp_path):
    script = module("deploy/docker/run_docker.py")
    monkeypatch.setenv("SOURCE_DIR", str(tmp_path))
    monkeypatch.setenv("CCACHE_MAXSIZE", "2G")
    monkeypatch.setenv("CCACHE_DIR", "/host/cache")
    monkeypatch.setenv("CPM_SOURCE_CACHE", "/host/cpm")
    monkeypatch.setenv("MOCCACHE_MAX_SIZE", "256M")
    calls = []
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))
    script.run_image("builder", "Release")
    assert "CCACHE_MAXSIZE=2G" in calls[0]
    assert "MOCCACHE_MAX_SIZE=256M" in calls[0]
    assert not any("/host/" in arg for arg in calls[0])
    assert f"{tmp_path}:/project/source" in calls[0]


@pytest.mark.parametrize(
    ("timings", "success", "compiled"),
    [
        ({"configure": 1}, False, False),
        ({"configure": 1, "build": 2}, False, False),
        ({"configure": 1, "build": 2}, True, True),
        ({"configure": 1, "build": 2, "install": 1}, False, True),
        ({"configure": 1, "build": 2, "install": 1, "package": 1}, False, True),
    ],
)
def test_docker_report_preserves_compilation_status(
    module, monkeypatch, tmp_path, timings, success, compiled
):
    script = module("deploy/docker/entrypoint.py")
    monkeypatch.setattr(
        script.subprocess,
        "run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 0, "", ""),
    )
    script.write_performance_report(tmp_path, tmp_path / "build", timings, success)
    report = json.loads((tmp_path / "build/docker-build-report.json").read_text())
    assert report["build_success"] is compiled


@pytest.mark.parametrize("platform,custom", [("mac", False), ("mac", True), ("windows", False)])
def test_deployment_scans_only_application_qml(tmp_path, platform, custom):
    source = tmp_path / "source with spaces"
    source.mkdir()
    (source / "main.cc").touch()
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        "project(DeployProbe NONE)\n"
        "set(CMAKE_INSTALL_LIBDIR lib)\n"
        "set(CMAKE_INSTALL_BINDIR bin)\n"
        "add_executable(DeployProbe main.cc)\n"
        f"set(MACOS {'ON' if platform == 'mac' else 'OFF'})\n"
        f"set(WIN32 {'ON' if platform == 'windows' else 'OFF'})\n"
        f"set(QGC_CUSTOM_BUILD {'ON' if custom else 'OFF'})\n"
        'set(QGC_CUSTOM_DIR "custom overlay")\n'
        "function(qt_generate_deploy_qml_app_script)\n"
        '  file(WRITE "${CMAKE_BINARY_DIR}/options.txt" "${ARGV}")\n'
        '  message(FATAL_ERROR "Deployment options captured")\n'
        "endfunction()\n"
        f'include("{REPO_ROOT / "cmake/install/Install.cmake"}")\n'
    )
    result = subprocess.run(
        ["cmake", "-S", str(source), "-B", str(tmp_path / "build")],
        capture_output=True,
        text=True,
    )
    assert "Deployment options captured" in result.stderr
    options = (tmp_path / "build/options.txt").read_text().split(";")
    assert f"-qmldir={source}" not in options
    if platform == "mac":
        assert f"-qmldir={source}/src" in options
        assert (f"-qmldir={source}/custom overlay" in options) is custom
        assert "-no-codesign" in options
    else:
        assert "-no-quick-import" in options
        assert not any(option.startswith("-qmldir=") for option in options)
