"""Tests for build_setup.py, build-setup's input validation and step planner."""

from __future__ import annotations

import json
from dataclasses import replace
from typing import TYPE_CHECKING

import pytest
from ci.runner.build_setup import SetupInputs, main, msvc_arch, plan_setup, validate
from common.build_config import load_build_config

from .._helpers import read_github_file

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows

CONFIG = {
    "qt": {"version": "6.12.0", "modules": "qtlocation qtserialport qtsensors"},
    "android": {"platform": "36", "min_sdk": "28", "cmdline_tools": "147", "java_version": "21"},
    "apple": {"xcode_version": "26.1", "xcode_ios_version": "26.2"},
    "build": {"cmake_version": "4.4.2", "ninja_version": "1.13.2"},
}
TOOL_OUTPUTS = {
    "qt_modules": "qtlocation qtserialport qtsensors",
    "qt_modules_ios": "qtlocation qtsensors",
    "cmake_version": "4.4.2",
    "ninja_version": "1.13.2",
    "java_version": "21",
    "cmdline_tools": "147",
    "android_platform": "36",
}
NO_ABIS = {"android_arm64": "false", "android_armv7": "false", "android_x86_64": "false"}
TEMP = "D:\\a\\_temp"


def _desktop(host: str, arch: str, msvc: str, runner_os: str, host_arch: str = "") -> tuple:
    inputs = SetupInputs("desktop", host, arch, "arm64-v8a;armeabi-v7a", runner_os, TEMP)
    outputs = {"qt_host": host, "qt_arch": arch, "cache_target": arch, "host_qt_arch": host_arch}
    return inputs, {**outputs, "msvc_arch": msvc, "xcode_version": "26.1", **NO_ABIS}, {}


def _android(host: str, arch: str, abis: str, runner_os: str, flags: str) -> tuple:
    inputs = SetupInputs("android", host, arch, abis, runner_os, TEMP)
    outputs = {"qt_host": host, "qt_arch": arch, "cache_target": "android", "host_qt_arch": ""}
    selected = dict(zip(NO_ABIS, flags.split(","), strict=True))
    env = {"ANDROID_MIN_SDK": "28"}
    if runner_os != "macOS":
        env |= {"ANDROID_SDK_ROOT": f"{TEMP}/android-sdk", "ANDROID_HOME": f"{TEMP}/android-sdk"}
    return inputs, {**outputs, "msvc_arch": "", "xcode_version": "26.1", **selected}, env


def _ios(arch: str) -> tuple:
    inputs = SetupInputs("ios", "", arch, "arm64-v8a;armeabi-v7a", "macOS", TEMP)
    outputs = {"qt_host": "mac", "qt_arch": "clang_64", "cache_target": "ios", "host_qt_arch": ""}
    return inputs, {**outputs, "msvc_arch": "", "xcode_version": "26.2", **NO_ABIS}, {}


# Every build-setup caller's inputs, with the values the action's former expressions produced:
# cache host `mode == 'ios' && 'mac' || qt-host`, cache target `mode == 'desktop' && qt-arch ||
# mode`, Qt arch `mode == 'ios' && 'clang_64' || qt-arch`, the Xcode ternary,
# `contains(format(';{0};', abis), ';<abi>;')`, and the Android environment steps. The MSVC
# `endsWith` chain and the host Qt arch only fed desktop-mode steps.
CALLERS = {
    "linux": _desktop("linux", "linux_gcc_64", "x64", "Linux"),
    "linux-arm64": _desktop("linux_arm64", "linux_gcc_arm64", "arm64", "Linux"),
    "macos": _desktop("mac", "clang_64", "x64", "macOS"),
    "windows": _desktop("windows", "win64_msvc2022_64", "x64", "Windows"),
    "windows-arm64": _desktop("windows_arm64", "win64_msvc2022_arm64", "arm64", "Windows"),
    "windows-arm64-cross": _desktop(
        "windows",
        "win64_msvc2022_arm64_cross_compiled",
        "amd64_arm64",
        "Windows",
        "win64_msvc2022_64",
    ),
    "android-linux": _android(
        "linux", "linux_gcc_64", "arm64-v8a;armeabi-v7a", "Linux", "true,true,false"
    ),
    "android-emulator": _android("linux", "linux_gcc_64", "x86_64", "Linux", "false,false,true"),
    "android-mac": _android("mac", "clang_64", "arm64-v8a", "macOS", "true,false,false"),
    "android-windows": _android(
        "windows", "win64_msvc2022_64", "arm64-v8a", "Windows", "true,false,false"
    ),
    "ios-device": _ios("ios_device"),
    "ios-simulator": _ios("ios_simulator_x86_64"),
}


@pytest.mark.parametrize(("inputs", "outputs", "env"), CALLERS.values(), ids=CALLERS.keys())
def test_plan_matches_the_former_action_expressions(
    inputs: SetupInputs, outputs: dict[str, str], env: dict[str, str]
) -> None:
    assert validate(inputs) == []
    plan = plan_setup(inputs, CONFIG)
    assert plan.outputs == {**outputs, **TOOL_OUTPUTS}
    assert plan.env == {"QT_VERSION": "6.12.0", **env}


@pytest.mark.parametrize(
    ("arch", "expected"),
    [
        ("win64_msvc2022_64", "x64"),
        ("WIN64_MSVC2022_ARM64", "arm64"),
        ("win64_msvc2022_arm64_cross_compiled", "amd64_arm64"),
        ("android_x86_64", "x64"),
    ],
)
def test_msvc_arch_matches_endswith_case_insensitively(arch: str, expected: str) -> None:
    assert msvc_arch(arch) == expected


def test_android_abis_match_whole_entries_case_insensitively() -> None:
    inputs = SetupInputs("android", "linux", "linux_gcc_64", "X86_64;arm64-v8a-extra;armeabi-v7a")
    assert validate(inputs) == []
    outputs = plan_setup(inputs, CONFIG).outputs
    assert (outputs["android_arm64"], outputs["android_armv7"], outputs["android_x86_64"]) == (
        "false",
        "true",
        "true",
    )


@pytest.mark.parametrize(
    ("inputs", "errors"),
    [
        (
            SetupInputs("desktop"),
            ["build-setup mode=desktop requires inputs: qt-host qt-arch"],
        ),
        (
            SetupInputs("android", qt_arch="linux_gcc_64", abis="x86_64"),
            ["build-setup mode=android requires inputs: qt-host"],
        ),
        (SetupInputs("ios"), ["build-setup mode=ios requires inputs: qt-arch"]),
        (
            SetupInputs("android", "linux", "linux_gcc_64", abis="x86; mips"),
            [
                "build-setup mode=android requires an arm64-v8a, armeabi-v7a, or x86_64 ABI "
                "(got 'x86; mips')"
            ],
        ),
        (
            SetupInputs("android", abis=""),
            [
                "build-setup mode=android requires inputs: qt-host qt-arch",
                "build-setup mode=android requires an arm64-v8a, armeabi-v7a, or x86_64 ABI "
                "(got '')",
            ],
        ),
        (
            SetupInputs("Linux", "linux", "linux_gcc_64"),
            ["build-setup mode must be one of desktop, android, ios (got 'Linux')"],
        ),
    ],
)
def test_validate_reports_missing_and_unsupported_inputs(
    inputs: SetupInputs, errors: list[str]
) -> None:
    assert validate(inputs) == errors


def test_ios_ignores_qt_host_and_desktop_ignores_abis() -> None:
    ios = SetupInputs("ios", "linux", "ios_device")
    assert validate(ios) == []
    assert plan_setup(ios, CONFIG).outputs["qt_host"] == "mac"
    assert validate(SetupInputs("desktop", "linux", "linux_gcc_64", abis="")) == []


def test_cross_compiled_arch_needs_a_windows_host_qt() -> None:
    inputs = SetupInputs("desktop", "linux", "linux_gcc_arm64_cross_compiled")
    with pytest.raises(ValueError, match="Not a Windows ARM64 cross-compiled Qt architecture"):
        plan_setup(inputs, CONFIG)
    # Only desktop builds install a separate host Qt.
    android = replace(inputs, mode="android", abis="x86_64")
    assert plan_setup(android, CONFIG).outputs["host_qt_arch"] == ""


def test_repository_build_config_supplies_every_value() -> None:
    config = load_build_config()
    plan = plan_setup(SetupInputs("android", "linux", "linux_gcc_64", "x86_64"), config)
    assert plan.outputs["cmake_version"] == config["build"]["cmake_version"]
    assert plan.outputs["ninja_version"] == config["build"]["ninja_version"]
    assert plan.outputs["cmdline_tools"] == config["android"]["cmdline_tools"]
    assert plan.env["ANDROID_MIN_SDK"] == config["android"]["min_sdk"]
    assert plan.env["QT_VERSION"] == config["qt"]["version"]


@pytest.fixture
def action_env(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> Path:
    config = tmp_path / "build-config.json"
    config.write_text(json.dumps(CONFIG), encoding="utf-8")
    monkeypatch.setenv("CONFIG_FILE", str(config))
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path / "temp"))
    monkeypatch.setenv("RUNNER_OS", "Linux")
    for name in ("MODE", "QT_HOST", "QT_ARCH", "ABIS"):
        monkeypatch.delenv(f"INPUT_{name}", raising=False)
    return tmp_path / "temp"


def test_main_creates_the_build_directory_and_writes_the_plan(
    monkeypatch: pytest.MonkeyPatch, action_env: Path, gh_output: Path, gh_env: Path
) -> None:
    for name, value in {"MODE": "android", "QT_HOST": "linux", "QT_ARCH": "linux_gcc_64"}.items():
        monkeypatch.setenv(f"INPUT_{name}", value)
    monkeypatch.setenv("INPUT_ABIS", "arm64-v8a;armeabi-v7a")

    assert main([]) == 0

    assert (action_env / "build").is_dir()
    outputs = read_github_file(gh_output)
    assert outputs["cache_target"] == "android"
    assert outputs["android_armv7"] == "true"
    sdk = f"{action_env}/android-sdk"
    assert read_github_file(gh_env) == {
        "QT_VERSION": "6.12.0",
        "ANDROID_MIN_SDK": "28",
        "ANDROID_SDK_ROOT": sdk,
        "ANDROID_HOME": sdk,
    }


def test_main_defaults_to_desktop_and_fails_without_writing(
    monkeypatch: pytest.MonkeyPatch,
    action_env: Path,
    gh_output: Path,
    gh_env: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.setenv("INPUT_QT_ARCH", "linux_gcc_64")

    assert main([]) == 1

    assert capsys.readouterr().out == "::error::build-setup mode=desktop requires inputs: qt-host\n"
    assert not action_env.exists()
    assert gh_output.read_text() == gh_env.read_text() == ""


@pytest.mark.usefixtures("gh_output")
def test_main_reports_missing_config_values_and_unpaired_cross_arches(
    monkeypatch: pytest.MonkeyPatch, action_env: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setenv("INPUT_QT_HOST", "windows")
    monkeypatch.setenv("INPUT_QT_ARCH", "mingw_arm64_cross_compiled")
    assert main([]) == 1
    assert "Not a Windows ARM64 cross-compiled Qt architecture" in capsys.readouterr().out

    monkeypatch.setenv("INPUT_QT_ARCH", "win64_msvc2022_64")
    (action_env.parent / "build-config.json").write_text('{"qt": {}}', encoding="utf-8")
    assert main([]) == 1
    assert capsys.readouterr().out == "::error::build-config.json has no qt.modules\n"
