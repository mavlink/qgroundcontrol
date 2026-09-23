#!/usr/bin/env python3
"""Provision the shared Android builder/development toolchain at image build time."""

from __future__ import annotations

import argparse
import platform
import shlex
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from common.build_config import load_build_config
from common.io import sha256_file
from common.net import download_file

ANDROID_KITS = {
    "arm64-v8a": "android_arm64_v8a",
    "armeabi-v7a": "android_armv7",
    "x86_64": "android_x86_64",
}
CMDLINE_SHA256 = {
    "14742923": "04453066b540409d975c676d781da1477479dde3761310f1a7eb92a1dfb15af7",
}
PREFIX = Path("/opt")


def run(*args: str) -> None:
    subprocess.run(args, check=True)


def install(tools: Path, abis: list[str]) -> None:
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise RuntimeError(
            "The official Android NDK requires Linux x86_64. "
            "Use qgc-dev with --platform linux/amd64 (Docker Desktop emulation on Apple Silicon)."
        )
    config = load_build_config(tools / "setup/build-config.json")
    android = config["android"]
    checksum = CMDLINE_SHA256[android["cmdline_tools"]]
    sdk = PREFIX / "android-sdk"
    qt = PREFIX / "Qt"
    run("apt-get", "update")
    run(
        "apt-get",
        "install",
        "-y",
        "--no-install-recommends",
        f"openjdk-{android['java_version']}-jdk",
    )
    archive = sdk / "cmdline-tools.zip"
    cmdline = sdk / "cmdline-tools"
    cmdline.mkdir(parents=True, exist_ok=True)
    download_file(
        "https://dl.google.com/android/repository/"
        f"commandlinetools-linux-{android['cmdline_tools']}_latest.zip",
        archive,
    )
    if sha256_file(archive) != checksum:
        raise RuntimeError("Android command-line tools SHA256 mismatch")
    run("unzip", "-q", str(archive), "-d", str(cmdline))
    (cmdline / "cmdline-tools").rename(cmdline / "latest")
    archive.unlink()
    sdkmanager = str(cmdline / "latest/bin/sdkmanager")
    # Supply finite input instead of hiding sdkmanager failures behind yes's SIGPIPE.
    subprocess.run(
        [sdkmanager, f"--sdk_root={sdk}", "--licenses"],
        input="y\n" * 100,
        text=True,
        check=True,
    )
    run(
        sdkmanager,
        f"--sdk_root={sdk}",
        "platform-tools",
        f"platforms;android-{android['platform']}",
        f"build-tools;{android['build_tools']}",
        f"ndk;{android['ndk_full_version']}",
    )
    installer = str(tools / "setup/install_qt.py")
    host = qt / config["qt"]["version"] / "gcc_64"
    if not (host / "bin/qt-cmake").is_file():
        run(
            sys.executable,
            installer,
            "install",
            "--from-config",
            "--host",
            "linux",
            "--target",
            "desktop",
            "--arch",
            "linux_gcc_64",
            "--outdir",
            str(qt),
        )
    kits = PREFIX / "qt-android"
    kits.mkdir(exist_ok=True)
    for abi in abis:
        arch = ANDROID_KITS[abi]
        run(
            sys.executable,
            installer,
            "install",
            "--from-config",
            "--host",
            "all_os",
            "--target",
            "android",
            "--arch",
            arch,
            "--outdir",
            str(qt),
            "--autodesktop",
        )
        (kits / abi).symlink_to(qt / config["qt"]["version"] / arch)
    java = Path(subprocess.check_output(["which", "javac"], text=True).strip()).resolve().parents[1]
    ndk = sdk / "ndk" / android["ndk_full_version"]
    build_tools = sdk / "build-tools" / android["build_tools"]
    for link, target in (
        (PREFIX / "java", java),
        (PREFIX / "android-ndk", ndk),
        (PREFIX / "android-build-tools", build_tools),
    ):
        link.symlink_to(target)
    env = {f"ANDROID_{key.upper()}": str(value) for key, value in android.items()}
    env.update(
        JAVA_HOME=str(java),
        ANDROID_HOME=str(sdk),
        ANDROID_SDK_ROOT=str(sdk),
        ANDROID_NDK_ROOT=str(ndk),
        ANDROID_NDK=str(ndk),
        ANDROID_NDK_HOME=str(ndk),
        ANDROID_BUILD_TOOLS_DIR=str(build_tools),
        QT_VERSION=config["qt"]["version"],
        QT_HOST_PATH=str(host),
        QT_ROOT_DIR_ARM64=str(qt / config["qt"]["version"] / ANDROID_KITS["arm64-v8a"]),
    )
    env_dir = PREFIX / "qgc-android"
    env_dir.mkdir(exist_ok=True)
    (env_dir / "env.sh").write_text(
        "".join(f"export {key}={shlex.quote(value)}\n" for key, value in env.items())
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tools", type=Path, default=Path("/opt/qgc-bootstrap/tools"))
    parser.add_argument("--abis", nargs="+", choices=ANDROID_KITS, default=["arm64-v8a"])
    args = parser.parse_args()
    install(args.tools, args.abis)


if __name__ == "__main__":
    main()
