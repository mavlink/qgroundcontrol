#!/usr/bin/env python3
"""Bake the configured LLVM/Clazy toolchain into the development image."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from common.build_config import load_build_config
from common.io import extract_tar_data, sha256_file
from common.net import download_with_retry

IMAGE_ROOT = Path("/opt/qgc-bootstrap")
PREFIX = Path("/opt/clazy")


def analysis_packages(llvm: str) -> list[str]:
    return [
        f"clang-{llvm}",
        f"clang-tidy-{llvm}",
        f"clang-tools-{llvm}",
        f"clangd-{llvm}",
        f"llvm-{llvm}-dev",
        f"libclang-{llvm}-dev",
        f"libclang-cpp{llvm}-dev",
        "iwyu",
        "ninja-build",
    ]


def build_clazy(llvm: str, revision: str, checksum: str, prefix: Path, work_dir: Path) -> None:
    work_dir.mkdir(parents=True, exist_ok=True)
    archive = work_dir / "clazy.tar.gz"
    download_with_retry(f"https://github.com/KDE/clazy/archive/{revision}.tar.gz", archive)
    if sha256_file(archive) != checksum:
        raise RuntimeError("Clazy source SHA256 mismatch")
    source_dir = work_dir / "source"
    source_dir.mkdir(exist_ok=True)
    extract_tar_data(archive, source_dir, mode="r:gz")
    source = source_dir / f"clazy-{revision}"
    build = work_dir / "build"
    affinity = getattr(os, "sched_getaffinity", None)
    workers = len(affinity(0)) if affinity else os.cpu_count() or 1
    subprocess.run(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_C_COMPILER=/usr/bin/clang-{llvm}",
            f"-DCMAKE_CXX_COMPILER=/usr/bin/clang++-{llvm}",
            f"-DCMAKE_PREFIX_PATH=/usr/lib/llvm-{llvm}",
            f"-DLLVM_CONFIG_EXECUTABLE=/usr/lib/llvm-{llvm}/bin/llvm-config",
            "-DCLAZY_LINK_CLANG_DYLIB=ON",
            "-DCLAZY_MAN_PAGE=OFF",
            f"-DCMAKE_INSTALL_PREFIX={prefix}",
        ],
        check=True,
    )
    subprocess.run(
        [
            "cmake",
            "--build",
            str(build),
            "--target",
            "clazy-standalone",
            "--parallel",
            str(workers),
        ],
        check=True,
    )
    subprocess.run(["cmake", "--install", str(build)], check=True)


def verify_toolchain(llvm: str, prefix: Path) -> None:
    for tool in ("clang", "clang-tidy", str(prefix / "bin/clazy-standalone")):
        version = subprocess.check_output([tool, "--version"], text=True)
        print(version, end="")
        if not re.search(rf"(?:clang|LLVM) version {re.escape(llvm)}\.", version):
            raise RuntimeError(f"{tool} does not use configured LLVM {llvm}")
    # Check the dynamic ABI as well as the version compiled into Clazy.
    libraries = subprocess.check_output(["ldd", str(prefix / "bin/clazy-standalone")], text=True)
    if "not found" in libraries or not re.search(
        rf"libclang-cpp\.so\.{re.escape(llvm)}(?:\.|\s)", libraries
    ):
        raise RuntimeError(f"Clazy does not link the configured libclang-cpp:\n{libraries}")
    subprocess.run(
        ["clang++", "-x", "c++", "-fsyntax-only", "-"],
        input="int main() { return 0; }\n",
        text=True,
        check=True,
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args(argv)
    config = load_build_config(IMAGE_ROOT / "tools/setup/build-config.json")
    analysis = config["analysis"]
    llvm = analysis["llvm_version"]
    if args.verify:
        verify_toolchain(llvm, PREFIX)
        return 0
    subprocess.run(["apt-get", "update"], check=True)
    subprocess.run(
        ["apt-get", "install", "-y", "--no-install-recommends", *analysis_packages(llvm)],
        check=True,
    )
    build_clazy(
        llvm,
        analysis["clazy_revision"],
        analysis["clazy_sha256"],
        PREFIX,
        IMAGE_ROOT / "clazy-build",
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
