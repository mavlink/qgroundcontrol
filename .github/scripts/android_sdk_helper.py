"""Android SDK/NDK setup helpers for CI."""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env, gh_error
from common.proc import run_with_retry


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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ndk-version", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--build-tools", required=True)
    parser.add_argument("--workspace", default=os.environ.get("GITHUB_WORKSPACE", "."))
    args = parser.parse_args()

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
