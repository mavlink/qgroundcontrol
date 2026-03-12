#!/usr/bin/env python3
"""Android SDK/NDK setup helpers for CI."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env  # noqa: E402


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ndk-version", required=True)
    parser.add_argument("--workspace", default=os.environ.get("GITHUB_WORKSPACE", "."))
    args = parser.parse_args()

    sdk_root = os.environ.get("ANDROID_SDK_ROOT", "")
    if not sdk_root:
        print("::error::ANDROID_SDK_ROOT not set", file=sys.stderr)
        sys.exit(1)

    sdk_root_unix = sdk_root.replace("\\", "/")
    ndk_path = f"{sdk_root_unix}/ndk/{args.ndk_version}"

    if not Path(ndk_path).is_dir():
        print(f"::error::NDK path not found: {ndk_path}", file=sys.stderr)
        sys.exit(1)

    append_github_env({
        "ANDROID_NDK_ROOT": ndk_path,
        "ANDROID_NDK_HOME": ndk_path,
        "ANDROID_NDK": ndk_path,
    })

    is_windows = os.environ.get("RUNNER_OS") == "Windows"

    if is_windows:
        sdkmanager = os.path.join(sdk_root, "cmdline-tools", "latest", "bin", "sdkmanager.bat")
        gradlew = os.path.join(args.workspace, "android", "gradlew.bat")
    else:
        sdkmanager = "sdkmanager"
        gradlew = os.path.join(args.workspace, "android", "gradlew")

    subprocess.run([sdkmanager, "--update"], check=True)
    subprocess.run([gradlew, "--version"], check=True)


if __name__ == "__main__":
    main()
