#!/usr/bin/env python3
"""Plan build-setup: validate its inputs and resolve the values its steps read.

`.github/actions/build-setup` runs this first, before setup-python, with its inputs as INPUT_*
variables. It creates `$RUNNER_TEMP/build`; writes the Qt host, architectures and modules, the
cache target, the Xcode and MSVC selections, the Windows ARM64 cross build's host Qt arch, which
Android ABIs to install, and the build-config.json tool versions to GITHUB_OUTPUT; and exports
QT_VERSION and, for Android, ANDROID_MIN_SDK (the presets' CMAKE_SYSTEM_VERSION) and the SDK
location to GITHUB_ENV.
"""

from __future__ import annotations

import argparse
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import derive_ios_qt_modules, load_build_config, lookup_dotted
from common.gh_actions import append_github_env, gh_error, write_github_output
from setup.qt_installer.repo import resolve_windows_host_arch

if TYPE_CHECKING:
    from collections.abc import Mapping

MODES = ("desktop", "android", "ios")
CROSS_COMPILED_SUFFIX = "_arm64_cross_compiled"
# Each supported Android ABI and the output that selects its Qt install.
ANDROID_ABIS = {
    "arm64-v8a": "android_arm64",
    "armeabi-v7a": "android_armv7",
    "x86_64": "android_x86_64",
}


@dataclass(frozen=True)
class SetupInputs:
    """build-setup's inputs and the runner facts that select its steps."""

    mode: str = "desktop"
    qt_host: str = ""
    qt_arch: str = ""
    abis: str = ""
    runner_os: str = ""
    runner_temp: str = ""

    @classmethod
    def from_env(cls, env: Mapping[str, str]) -> SetupInputs:
        return cls(
            mode=env.get("INPUT_MODE", "desktop"),
            qt_host=env.get("INPUT_QT_HOST", ""),
            qt_arch=env.get("INPUT_QT_ARCH", ""),
            abis=env.get("INPUT_ABIS", ""),
            runner_os=env.get("RUNNER_OS", ""),
            runner_temp=env.get("RUNNER_TEMP", ""),
        )


@dataclass(frozen=True)
class SetupPlan:
    outputs: dict[str, str]
    env: dict[str, str]


def validate(inputs: SetupInputs) -> list[str]:
    """Return an error for each input the mode requires but lacks."""
    if inputs.mode not in MODES:
        return [f"build-setup mode must be one of {', '.join(MODES)} (got '{inputs.mode}')"]
    errors = []
    values = {"qt-host": inputs.qt_host, "qt-arch": inputs.qt_arch}
    required = ("qt-arch",) if inputs.mode == "ios" else ("qt-host", "qt-arch")
    if missing := [name for name in required if not values[name]]:
        errors.append(f"build-setup mode={inputs.mode} requires inputs: {' '.join(missing)}")
    if inputs.mode == "android" and not set(inputs.abis.split(";")) & ANDROID_ABIS.keys():
        errors.append(
            "build-setup mode=android requires an arm64-v8a, armeabi-v7a, or x86_64 ABI "
            f"(got '{inputs.abis}')"
        )
    return errors


def msvc_arch(qt_arch: str) -> str:
    """setup-msvc-dev's toolchain: native x64 or ARM64, else the x64-hosted ARM64 cross compiler."""
    arch = qt_arch.lower()
    if arch.endswith("_64"):
        return "x64"
    if arch.endswith("_arm64"):
        return "arm64"
    return "amd64_arm64"


def plan_setup(inputs: SetupInputs, config: Mapping[str, Any]) -> SetupPlan:
    """Resolve the outputs and environment for validated inputs.

    Raises KeyError for a missing build-config.json value and ValueError for a cross-compiled
    arch without a Windows x64 host Qt.
    """

    def value(key: str) -> str:
        return str(lookup_dotted(config, key))

    ios = inputs.mode == "ios"
    qt_modules = value("qt.modules")
    # The action's former `contains(format(';{0};', abis), ...)` matched case-insensitively.
    abis = inputs.abis.lower().split(";") if inputs.mode == "android" else []
    cross = inputs.mode == "desktop" and inputs.qt_arch.lower().endswith(CROSS_COMPILED_SUFFIX)
    outputs = {
        # iOS keeps its mac/ios cache identity and runs its host tools from the macOS Qt.
        "qt_host": "mac" if ios else inputs.qt_host,
        "qt_arch": "clang_64" if ios else inputs.qt_arch,
        "cache_target": inputs.qt_arch if inputs.mode == "desktop" else inputs.mode,
        "qt_modules": qt_modules,
        "qt_modules_ios": derive_ios_qt_modules(qt_modules),
        "host_qt_arch": resolve_windows_host_arch(inputs.qt_arch) if cross else "",
        "msvc_arch": msvc_arch(inputs.qt_arch) if inputs.mode == "desktop" else "",
        "xcode_version": value("apple.xcode_ios_version" if ios else "apple.xcode_version"),
        **{output: str(abi in abis).lower() for abi, output in ANDROID_ABIS.items()},
        "cmake_version": value("build.cmake_version"),
        "ninja_version": value("build.ninja_version"),
        "java_version": value("android.java_version"),
        "cmdline_tools": value("android.cmdline_tools"),
        "android_platform": value("android.platform"),
    }
    env = {"QT_VERSION": value("qt.version")}
    if inputs.mode == "android":
        env["ANDROID_MIN_SDK"] = value("android.min_sdk")
        # RunsOn images ship no Android SDK and don't preset ANDROID_SDK_ROOT (unlike GitHub-hosted
        # images), so setup-android would have no install target. macOS legs stay GitHub-hosted
        # and keep their preinstalled SDK.
        if inputs.runner_os != "macOS":
            sdk_root = f"{inputs.runner_temp}/android-sdk"
            env.update(ANDROID_SDK_ROOT=sdk_root, ANDROID_HOME=sdk_root)
    return SetupPlan(outputs, env)


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    inputs = SetupInputs.from_env(os.environ)
    errors = validate(inputs)
    plan = None
    if not errors:
        try:
            plan = plan_setup(inputs, load_build_config())
        except KeyError as error:
            errors.append(f"build-config.json has no {error.args[0]}")
        except ValueError as error:
            errors.append(str(error))
    if plan is None:
        for error in errors:
            gh_error(error)
        return 1

    Path(inputs.runner_temp, "build").mkdir(parents=True, exist_ok=True)
    for name, value in {**plan.outputs, **plan.env}.items():
        print(f"{name}={value}")
    write_github_output(plan.outputs)
    append_github_env(plan.env)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
