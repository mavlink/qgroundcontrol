#!/usr/bin/env python3
"""Android SDK/NDK setup helpers for CI."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env, gh_error
from common.net import run_with_retries


def _find_sdkmanager(sdk_root: str) -> str:
    """Locate sdkmanager.bat under cmdline-tools/{latest,<version>}, preferring latest."""
    cmdline_tools = Path(sdk_root) / "cmdline-tools"
    default = cmdline_tools / "latest" / "bin" / "sdkmanager.bat"
    # Prefer latest; fall back to highest numeric version (9.0 < 10.0, not lexicographic).
    versioned = sorted(
        (p for p in cmdline_tools.glob("*/bin/sdkmanager.bat") if p.parent.parent.name != "latest"),
        key=lambda p: [int(n) if n.isdigit() else -1 for n in p.parent.parent.name.split(".")],
        reverse=True,
    )
    found = next((c for c in (default, *versioned) if c.is_file()), None)
    if found is None:
        gh_error(f"sdkmanager.bat not found under {cmdline_tools}")
        sys.exit(1)
    return str(found)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ndk-version", required=True)
    parser.add_argument("--workspace", default=os.environ.get("GITHUB_WORKSPACE", "."))
    args = parser.parse_args()

    sdk_root = os.environ.get("ANDROID_SDK_ROOT", "")
    if not sdk_root:
        gh_error("ANDROID_SDK_ROOT not set")
        sys.exit(1)

    sdk_root_unix = sdk_root.replace("\\", "/")
    ndk_path = f"{sdk_root_unix}/ndk/{args.ndk_version}"

    if not Path(ndk_path).is_dir():
        gh_error(f"NDK path not found: {ndk_path}")
        sys.exit(1)

    append_github_env(
        {
            "ANDROID_NDK_ROOT": ndk_path,
            "ANDROID_NDK_HOME": ndk_path,
            "ANDROID_NDK": ndk_path,
        }
    )

    is_windows = os.environ.get("RUNNER_OS") == "Windows"

    if is_windows:
        sdkmanager = _find_sdkmanager(sdk_root)
        gradlew = os.path.join(args.workspace, "android", "gradlew.bat")
    else:
        sdkmanager = "sdkmanager"
        gradlew = os.path.join(args.workspace, "android", "gradlew")

    run_with_retries([sdkmanager, "--update"])
    run_with_retries([gradlew, "--version"])


if __name__ == "__main__":
    main()
