#!/usr/bin/env python3
"""Android SDK/NDK setup helpers for CI.

android_sdk_helper.py --ndk-version V --platform P --build-tools B  # install SDK packages
android_sdk_helper.py debug-keystore [--keystore PATH]                # throwaway signing key
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from packaging.version import InvalidVersion, Version

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import append_github_env, gh_error
from common.proc import run_with_retry


def _cmdline_tools_version(sdkmanager: Path) -> Version:
    try:
        return Version(sdkmanager.parent.parent.name)
    except InvalidVersion:
        return Version("0")


def _find_sdkmanager(sdk_root: str) -> str:
    """Locate sdkmanager.bat under cmdline-tools/{latest,<version>}, preferring latest."""
    cmdline_tools = Path(sdk_root) / "cmdline-tools"
    default = cmdline_tools / "latest" / "bin" / "sdkmanager.bat"
    # Prefer latest; fall back to the newest version directory (10.0 after 9.0).
    versioned = sorted(
        (p for p in cmdline_tools.glob("*/bin/sdkmanager.bat") if p.parent.parent.name != "latest"),
        key=_cmdline_tools_version,
        reverse=True,
    )
    found = next((c for c in (default, *versioned) if c.is_file()), None)
    if found is None:
        gh_error(f"sdkmanager.bat not found under {cmdline_tools}")
        sys.exit(1)
    return str(found)


def _install_packages(sdkmanager: str, packages: list[str]) -> None:
    run_with_retry(
        [sdkmanager, *packages],
        max_attempts=3,
        retry_backoff_seconds=15,
        timeout=1800,
    )


def _install_ndk(sdkmanager: str, ndk_version: str, ndk_path: Path) -> None:
    def remove_partial_ndk() -> None:
        if ndk_path.exists():
            shutil.rmtree(ndk_path)

    run_with_retry(
        [sdkmanager, f"ndk;{ndk_version}"],
        max_attempts=3,
        retry_backoff_seconds=15,
        before_retry=remove_partial_ndk,
        timeout=1800,
    )


DEBUG_KEYSTORE_ALIAS = "androiddebugkey"
DEBUG_KEYSTORE_PASSWORD = "android"


def create_debug_keystore(keystore: Path) -> None:
    """Generate a throwaway signing key and point Qt's Android signing variables at it."""
    keytool = shutil.which("keytool")
    if keytool is None:
        gh_error("keytool not found on PATH")
        sys.exit(1)
    keystore_path = keystore.as_posix()
    subprocess.run(
        [
            keytool,
            "-genkey",
            "-v",
            "-keystore",
            keystore_path,
            "-storepass",
            DEBUG_KEYSTORE_PASSWORD,
            "-alias",
            DEBUG_KEYSTORE_ALIAS,
            "-keypass",
            DEBUG_KEYSTORE_PASSWORD,
            "-keyalg",
            "RSA",
            "-keysize",
            "2048",
            "-validity",
            "10000",
            "-dname",
            "CN=Android Debug,O=Android,C=US",
        ],
        check=True,
    )
    append_github_env(
        {
            "QT_ANDROID_KEYSTORE_PATH": keystore_path,
            "QT_ANDROID_KEYSTORE_ALIAS": DEBUG_KEYSTORE_ALIAS,
            "QT_ANDROID_KEYSTORE_STORE_PASS": DEBUG_KEYSTORE_PASSWORD,
            "QT_ANDROID_KEYSTORE_KEY_PASS": DEBUG_KEYSTORE_PASSWORD,
        }
    )


def _debug_keystore_main(argv: list[str]) -> None:
    parser = argparse.ArgumentParser(
        prog="android_sdk_helper.py debug-keystore",
        description="Create a debug keystore and export Qt's Android signing variables",
    )
    parser.add_argument(
        "--keystore", type=Path, help="Keystore to create (default: $RUNNER_TEMP/debug.keystore)"
    )
    args = parser.parse_args(argv)
    keystore = args.keystore
    if keystore is None:
        runner_temp = os.environ.get("RUNNER_TEMP")
        if not runner_temp:
            parser.error("--keystore is required when RUNNER_TEMP is unset")
        keystore = Path(runner_temp) / "debug.keystore"
    create_debug_keystore(keystore)


def main(argv: list[str] | None = None) -> None:
    argv = sys.argv[1:] if argv is None else argv
    if argv[:1] == ["debug-keystore"]:
        _debug_keystore_main(argv[1:])
        return

    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--ndk-version", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--build-tools", required=True)
    parser.add_argument("--workspace", default=os.environ.get("GITHUB_WORKSPACE", "."))
    args = parser.parse_args(argv)

    sdk_root = os.environ.get("ANDROID_SDK_ROOT", "")
    if not sdk_root:
        gh_error("ANDROID_SDK_ROOT not set")
        sys.exit(1)

    is_windows = os.environ.get("RUNNER_OS") == "Windows"

    if is_windows:
        sdkmanager = _find_sdkmanager(sdk_root)
        gradlew = os.path.join(args.workspace, "android", "gradlew.bat")
    else:
        sdkmanager = "sdkmanager"
        gradlew = os.path.join(args.workspace, "android", "gradlew")

    ndk_path = Path(sdk_root) / "ndk" / args.ndk_version
    _install_packages(
        sdkmanager,
        [
            "platform-tools",
            f"platforms;android-{args.platform}",
            f"build-tools;{args.build_tools}",
        ],
    )
    _install_ndk(sdkmanager, args.ndk_version, ndk_path)

    if not ndk_path.is_dir():
        gh_error(f"NDK path not found after installation: {ndk_path}")
        sys.exit(1)

    ndk_path_string = str(ndk_path).replace("\\", "/")
    append_github_env(
        {
            "ANDROID_NDK_ROOT": ndk_path_string,
            "ANDROID_NDK_HOME": ndk_path_string,
            "ANDROID_NDK": ndk_path_string,
        }
    )

    run_with_retry([gradlew, "--version"], timeout=300)


if __name__ == "__main__":
    main()
