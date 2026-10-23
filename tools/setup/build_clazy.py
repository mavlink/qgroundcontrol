#!/usr/bin/env python3
"""
Build clazy-standalone from the pinned source against the analysis LLVM.

Ubuntu's Clazy package links an older LLVM than QGC's analysis compiler, so the
Code Analysis workflow builds the Clazy revision pinned in
.github/build-config.json (`analysis.clazy_revision`, verified against
`analysis.clazy_sha256`) with `analysis.llvm_version`. Run it locally to get the
same binary.

Usage:
    build_clazy.py [--prefix .cache/clazy] [--work-dir DIR] [--jobs N]

Requires cmake, ninja and, on Debian/Ubuntu, for LLVM version N:
    clang-N llvm-N-dev libclang-N-dev libclang-cppN-dev
Then put <prefix>/bin on PATH for `tools/analyze.py --tool clazy`.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.io import extract_tar_data
from common.logging import log_error, log_info, log_ok
from common.net import download_file
from common.paths import REPO_ROOT

ARCHIVE_URL = "https://github.com/KDE/clazy/archive/{revision}.tar.gz"
DEFAULT_PREFIX = REPO_ROOT / ".cache" / "clazy"


@dataclass(frozen=True)
class ClazyPin:
    llvm_version: str
    revision: str
    sha256: str


def read_pin(config_file: Path | None = None) -> ClazyPin:
    analysis = load_build_config(config_file)["analysis"]
    return ClazyPin(
        llvm_version=str(analysis["llvm_version"]),
        revision=analysis["clazy_revision"],
        sha256=analysis["clazy_sha256"],
    )


def llvm_root(llvm_version: str) -> Path:
    return Path(f"/usr/lib/llvm-{llvm_version}")


def configure_command(source: Path, build: Path, prefix: Path, llvm_version: str) -> list[str]:
    llvm = llvm_root(llvm_version)
    return [
        "cmake",
        "-S",
        str(source),
        "-B",
        str(build),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_C_COMPILER=/usr/bin/clang-{llvm_version}",
        f"-DCMAKE_CXX_COMPILER=/usr/bin/clang++-{llvm_version}",
        f"-DCMAKE_PREFIX_PATH={llvm}",
        f"-DLLVM_CONFIG_EXECUTABLE={llvm / 'bin' / 'llvm-config'}",
        "-DCLAZY_LINK_CLANG_DYLIB=ON",
        "-DCLAZY_MAN_PAGE=OFF",
        f"-DCMAKE_INSTALL_PREFIX={prefix}",
    ]


def run(command: list[str]) -> None:
    log_info(f"$ {shlex.join(command)}")
    subprocess.run(command, check=True)


def build_clazy(pin: ClazyPin, prefix: Path, work_dir: Path, jobs: int) -> Path:
    """Download, verify, build and install clazy-standalone; return the installed binary."""
    llvm_config = llvm_root(pin.llvm_version) / "bin" / "llvm-config"
    if not llvm_config.is_file():
        version = pin.llvm_version
        raise FileNotFoundError(
            f"{llvm_config} not found; install clang-{version} llvm-{version}-dev "
            f"libclang-{version}-dev libclang-cpp{version}-dev"
        )
    work_dir.mkdir(parents=True, exist_ok=True)
    archive = work_dir / "clazy.tar.gz"
    download_file(ARCHIVE_URL.format(revision=pin.revision), archive, expected_sha256=pin.sha256)
    extracted = work_dir / "src"
    shutil.rmtree(extracted, ignore_errors=True)
    extract_tar_data(archive, extracted)
    # GitHub source archives hold a single clazy-<revision>/ directory.
    (source,) = extracted.iterdir()
    build = work_dir / "build"
    run(configure_command(source, build, prefix, pin.llvm_version))
    run(["cmake", "--build", str(build), "--target", "clazy-standalone", "--parallel", str(jobs)])
    run(["cmake", "--install", str(build)])
    return prefix / "bin" / "clazy-standalone"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--prefix",
        type=Path,
        default=DEFAULT_PREFIX,
        help=f"Install prefix (default: {DEFAULT_PREFIX})",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        help="Download and build directory (default: a temporary directory)",
    )
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    args = parser.parse_args(argv)

    pin = read_pin()
    prefix = args.prefix.resolve()
    try:
        if args.work_dir:
            binary = build_clazy(pin, prefix, args.work_dir.resolve(), args.jobs)
        else:
            with tempfile.TemporaryDirectory(prefix="clazy-") as work_dir:
                binary = build_clazy(pin, prefix, Path(work_dir), args.jobs)
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        log_error(str(error))
        return 1
    log_ok(f"Built {binary} (Clazy {pin.revision[:12]}, LLVM {pin.llvm_version})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
