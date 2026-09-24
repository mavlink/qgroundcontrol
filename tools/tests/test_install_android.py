"""Android development image contracts without downloading SDKs."""

from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
import zipfile
from pathlib import Path
from unittest.mock import Mock

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[2]


def load_module(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / f"deploy/docker/{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


installer = load_module("install_android")
smoke = load_module("smoke_dev")


@pytest.fixture
def provision(tmp_path, monkeypatch):
    config = json.loads((ROOT / ".github/build-config.json").read_text())
    config["qt"]["version"] = "6.99.1"
    config["android"].update(
        platform="99",
        min_sdk="30",
        build_tools="99.1.0",
        ndk_full_version="99.2.3",
        java_version="25",
    )
    prefix = tmp_path / "opt"
    prefix.mkdir()
    monkeypatch.setattr(installer, "PREFIX", prefix)
    monkeypatch.setattr(installer.platform, "machine", lambda: "x86_64")
    monkeypatch.setattr(installer.platform, "system", lambda: "Linux")
    monkeypatch.setattr(installer, "load_build_config", lambda path: config)
    monkeypatch.setattr(installer, "download_file", lambda url, path: path.write_text("archive"))
    monkeypatch.setattr(installer, "sha256_file", lambda path: installer.CMDLINE_SHA256["14742923"])
    calls = []

    def run(args, **kwargs):
        calls.append((args, kwargs))
        if args[0] == "unzip":
            (Path(args[-1]) / "cmdline-tools").mkdir()

    monkeypatch.setattr(installer.subprocess, "run", run)
    monkeypatch.setattr(
        installer.subprocess, "check_output", lambda *args, **kwargs: f"{prefix}/jdk/bin/javac\n"
    )
    return prefix, config, calls


@pytest.mark.parametrize("host_installed", [False, True])
def test_shared_provisioning_uses_config_and_reuses_desktop(provision, host_installed):
    prefix, config, calls = provision
    host = prefix / "Qt" / config["qt"]["version"] / "gcc_64"
    if host_installed:
        (host / "bin").mkdir(parents=True)
        (host / "bin/qt-cmake").touch()
    installer.install(prefix / "tools", list(installer.ANDROID_KITS))
    commands = [args for args, _ in calls]
    assert "openjdk-25-jdk" in commands[1]
    packages = next(args for args in commands if "platform-tools" in args)
    assert list(packages[2:]) == [
        "platform-tools",
        "platforms;android-99",
        "build-tools;99.1.0",
        "ndk;99.2.3",
    ]
    qt = [args for args in commands if "--from-config" in args]
    assert len(qt) == (3 if host_installed else 4)
    assert sum("desktop" in args for args in qt) == (0 if host_installed else 1)
    for abi, arch in installer.ANDROID_KITS.items():
        assert (prefix / "qt-android" / abi).readlink() == prefix / "Qt/6.99.1" / arch
        command = next(args for args in qt if arch in args)
        assert "--autodesktop" in command
        assert command[command.index("--host") + 1] == "all_os"
    assert (prefix / "android-ndk").readlink() == prefix / "android-sdk/ndk/99.2.3"
    assert (prefix / "android-build-tools").readlink() == prefix / "android-sdk/build-tools/99.1.0"
    assert (prefix / "java").readlink() == prefix / "jdk"
    env = (prefix / "qgc-android/env.sh").read_text()
    assert "export ANDROID_MIN_SDK=30\n" in env
    assert f"export QT_HOST_PATH={host}\n" in env
    assert "export QT_ROOT_DIR=" not in env
    assert "CMAKE_TOOLCHAIN_FILE" not in env
    assert all(kwargs["check"] for _, kwargs in calls)


@pytest.mark.parametrize("machine", ["aarch64", "riscv64"])
def test_unsupported_hosts_fail_before_any_install(monkeypatch, machine):
    monkeypatch.setattr(installer.platform, "machine", lambda: machine)
    run = Mock()
    monkeypatch.setattr(installer.subprocess, "run", run)
    with pytest.raises(RuntimeError, match="--platform linux/amd64"):
        installer.install(Path("unused"), ["arm64-v8a"])
    run.assert_not_called()


def test_checksum_failure_stops_before_sdk_or_qt_install(provision, monkeypatch):
    prefix, _, calls = provision
    monkeypatch.setattr(installer, "sha256_file", lambda path: "wrong")
    with pytest.raises(RuntimeError, match="SHA256 mismatch"):
        installer.install(prefix / "tools", ["arm64-v8a"])
    assert len(calls) == 2
    assert not (prefix / "qgc-android/env.sh").exists()


def test_sdk_failure_is_not_hidden(provision, monkeypatch):
    prefix, _, _ = provision
    original = installer.subprocess.run

    def fail_license(args, **kwargs):
        if "--licenses" in args:
            raise subprocess.CalledProcessError(1, args)
        return original(args, **kwargs)

    monkeypatch.setattr(installer.subprocess, "run", fail_license)
    with pytest.raises(subprocess.CalledProcessError):
        installer.install(prefix / "tools", ["arm64-v8a"])
    assert not (prefix / "qgc-android/env.sh").exists()


def test_android_stages_preserve_builder_and_desktop_contracts():
    text = (ROOT / "deploy/docker/Dockerfile").read_text()
    amd64 = text.split("FROM linux AS qgc-dev-android-amd64", 1)[1].split("FROM linux AS", 1)[0]
    arm64 = text.split("FROM linux AS qgc-dev-android-arm64", 1)[1].split("FROM qgc-dev", 1)[0]
    dev = text.split("FROM qgc-dev-android-${TARGETARCH} AS qgc-dev", 1)[1].split("FROM base", 1)[0]
    builder = text.split("FROM base AS android", 1)[1]
    for stage in (amd64, builder):
        assert "install_android.py" in stage
        assert "ANDROID_SDK_ROOT=/opt/android-sdk" in stage
    assert "--abis arm64-v8a armeabi-v7a x86_64" in amd64
    assert "install_android.py" not in arm64
    assert "ENV ANDROID" not in arm64
    assert "--platform linux/amd64" in arm64
    assert "ENV QT_ROOT_DIR=/opt/qt" in dev
    assert "ENTRYPOINT []" in dev
    assert "USER ${DEV_USER}" in dev
    assert 'SHELL ["/bin/bash", "-o", "pipefail", "-c"]' in dev
    assert "cat /opt/qgc-android/env.sh >> /etc/profile.d/qgc.sh" in builder
    assert 'ENTRYPOINT ["/bin/bash", "-l", "/entrypoint.sh"]' in builder
    assert "--abis" not in builder  # Existing application builder remains arm64-only.
    for version in ("6.11.1", "27.2.12479018", "36.0.0"):
        assert version not in text
    action = yaml.safe_load((ROOT / ".github/actions/qt-android/action.yml").read_text())
    for arch in installer.ANDROID_KITS.values():
        assert any(step.get("with", {}).get("arch") == arch for step in action["runs"]["steps"])


@pytest.mark.parametrize("abi", ["arm64-v8a", "armeabi-v7a", "x86_64"])
def test_selector_nonlogin_environment_keeps_desktop_default(tmp_path, abi):
    opt = tmp_path / "opt"
    kit = opt / "qt-android" / abi / "lib/cmake/Qt6"
    kit.mkdir(parents=True)
    (kit / "qt.toolchain.cmake").touch()
    (opt / "qgc-android").mkdir()
    (opt / "qgc-android/env.sh").write_text(
        "export QT_HOST_PATH=/opt/qt\nexport ANDROID_MIN_SDK=28\n"
    )
    script = tmp_path / "qgc-android"
    script.write_text(
        (ROOT / "deploy/docker/qgc-android.sh").read_text().replace("/opt/", f"{opt}/")
    )
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    uname = bin_dir / "uname"
    uname.write_text("#!/bin/sh\nprintf 'x86_64\\n'\n")
    uname.chmod(0o755)
    env = dict(os.environ, PATH=f"{bin_dir}:{os.environ['PATH']}", QT_ROOT_DIR="/opt/qt")
    result = subprocess.run(
        [
            "bash",
            str(script),
            abi,
            sys.executable,
            "-c",
            "import json, os; print(json.dumps(dict(os.environ)))",
        ],
        env=env,
        check=True,
        text=True,
        capture_output=True,
    )
    selected = json.loads(result.stdout)
    assert selected["QT_ROOT_DIR"] == "/opt/qt"
    assert selected["QT_TARGET_ROOT_DIR"] == str(opt / "qt-android" / abi)
    assert selected["ANDROID_MIN_SDK"] == "28"
    assert selected["ANDROID_ABIS"] == abi
    assert "CMAKE_TOOLCHAIN_FILE" not in selected


def test_smoke_requires_nonroot(monkeypatch):
    monkeypatch.setattr(smoke.os, "getuid", lambda: 0)
    with pytest.raises(RuntimeError, match="non-root"):
        smoke.main()


def test_arm_smoke_checks_explicit_omission(monkeypatch, tmp_path):
    monkeypatch.setattr(smoke.platform, "machine", lambda: "aarch64")
    monkeypatch.delenv("ANDROID_SDK_ROOT", raising=False)
    run = Mock(return_value=subprocess.CompletedProcess([], 1, "", "--platform linux/amd64"))
    monkeypatch.setattr(smoke.subprocess, "run", run)
    smoke.android_smoke({}, tmp_path)
    assert run.call_args.args[0] == ["qgc-android", "arm64-v8a", "true"]
    monkeypatch.setenv("ANDROID_SDK_ROOT", "/opt/android-sdk")
    with pytest.raises(RuntimeError, match="unusable"):
        smoke.android_smoke({}, tmp_path)


def test_android_smoke_builds_every_abi_and_packages_offline(tmp_path, monkeypatch):
    config = json.loads((ROOT / ".github/build-config.json").read_text())
    android = config["android"]
    sdk = tmp_path / "sdk"
    monkeypatch.setenv("ANDROID_SDK_ROOT", str(sdk))
    monkeypatch.setenv("ANDROID_NDK_ROOT", str(sdk / "ndk" / android["ndk_full_version"]))
    monkeypatch.setenv("ANDROID_BUILD_TOOLS_DIR", str(sdk / "build-tools" / android["build_tools"]))
    monkeypatch.setattr(smoke.platform, "machine", lambda: "x86_64")
    monkeypatch.setattr(
        smoke.subprocess,
        "check_output",
        lambda *args, **kwargs: f"javac {android['java_version']}.0.1",
    )
    deployment = Mock()
    monkeypatch.setattr(smoke, "android_deployment_tool_smoke", deployment)
    commands = []

    def run(*args):
        commands.append(args)
        if args[:2] == ("cmake", "--build"):
            build = Path(args[2])
            build.mkdir()
            machine = {"arm64-v8a": 183, "armeabi-v7a": 40, "x86_64": 62}[build.name]
            (build / "libandroid_smoke.so").write_bytes(
                b"\x7fELF" + bytes(14) + machine.to_bytes(2, "little")
            )
        if args[:2] == ("aapt2", "link"):
            with zipfile.ZipFile(args[args.index("-o") + 1], "w"):
                pass

    monkeypatch.setattr(smoke, "run", run)
    smoke.android_smoke(config, tmp_path / "smoke")
    for abi in installer.ANDROID_KITS:
        command = next(args for args in commands if args[:2] == ("qgc-android", abi))
        assert f"-DANDROID_ABI={abi}" in command
        assert f"-DCMAKE_SYSTEM_VERSION={android['min_sdk']}" in command
    assert ("sdkmanager", "--version") in commands
    deployment.assert_called_once_with(tmp_path / "smoke/deployment")
    assert any(args[:2] == ("aapt2", "compile") for args in commands)
    assert any(args[:2] == ("apksigner", "verify") for args in commands)
    assert any(args[:2] == ("zipalign", "-c") for args in commands)
    assert all("--licenses" not in args and "--update" not in args for args in commands)
    with zipfile.ZipFile(tmp_path / "smoke/unsigned.apk") as apk:
        assert len(apk.namelist()) == 3


@pytest.mark.parametrize(
    ("code", "stderr", "valid"),
    [
        (1, "Syntax: androiddeployqt --output <destination> [options]", True),
        (1, "error while loading shared libraries", False),
        (127, "Syntax: androiddeployqt --output", False),
        (0, "", False),
    ],
)
def test_android_deployment_tool_requires_documented_help_result(
    tmp_path, monkeypatch, code, stderr, valid
):
    run = Mock(return_value=subprocess.CompletedProcess([], code, "", stderr))
    monkeypatch.setattr(smoke.subprocess, "run", run)
    if valid:
        smoke.android_deployment_tool_smoke(tmp_path)
    else:
        with pytest.raises(RuntimeError, match="androiddeployqt help failed"):
            smoke.android_deployment_tool_smoke(tmp_path)
    assert run.call_args.args[0] == [
        "/opt/qt/bin/androiddeployqt",
        "--output",
        str(tmp_path),
        "--help",
    ]
