#!/usr/bin/env python3
"""
Install the static-analysis toolchain that Code Analysis uses, on Debian 13+ or Ubuntu 24.04+.

Everything follows .github/build-config.json's `analysis` section:
  - LLVM `llvm_version` from apt: clang, clang-scan-deps and clang-apply-replacements
    (clang-tools), clangd, and the development packages Clazy builds against
  - clang-tidy from the PyPI wheel pinned by `clang_tidy_version` and the machine's
    `clang_tidy_sha256` entry
  - clazy-standalone built from the pinned revision by build_clazy.py

<prefix>/env.sh puts clang-tidy, Clazy and LLVM on PATH in that order, as analysis.yml does, for
`tools/analyze.py`. The devcontainer image installs to /opt/qgc-analysis.

Usage:
    install_analysis_tools.py [--prefix .cache/analysis-tools] [--skip-apt] [--jobs N]
"""

from __future__ import annotations

import argparse
import os
import platform
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.logging import log_error, log_info, log_ok
from common.paths import REPO_ROOT
from setup import build_clazy

DEFAULT_PREFIX = REPO_ROOT / ".cache" / "analysis-tools"


def default_jobs() -> int:
    """One Clazy compile job per CPU, but at most one per 2 GiB of memory: its units are large."""
    cpus = os.cpu_count() or 1
    try:
        memory = os.sysconf("SC_PHYS_PAGES") * os.sysconf("SC_PAGE_SIZE")
    except (AttributeError, OSError, ValueError):
        return cpus
    return max(1, min(cpus, memory // (2 << 30)))


def llvm_packages(llvm_version: str) -> list[str]:
    return [
        f"clang-{llvm_version}",
        f"clang-tools-{llvm_version}",
        f"clangd-{llvm_version}",
        f"llvm-{llvm_version}-dev",
        f"libclang-{llvm_version}-dev",
        f"libclang-cpp{llvm_version}-dev",
    ]


def bin_dirs(prefix: Path) -> list[Path]:
    """PATH entries, highest precedence first: the pinned clang-tidy shadows LLVM's own."""
    return [
        prefix / "clang-tidy" / "clang_tidy" / "data" / "bin",
        prefix / "clazy" / "bin",
        prefix / "llvm" / "bin",
    ]


def _run(command: list[str], *, privileged: bool = False) -> None:
    if privileged and os.geteuid() != 0:
        command = ["sudo", *command]
    log_info(f"$ {shlex.join(command)}")
    subprocess.run(command, check=True)


def install_llvm(llvm_version: str) -> None:
    _run(["apt-get", "update", "-qq"], privileged=True)
    _run(
        [
            "apt-get",
            "install",
            "-y",
            "-qq",
            "--no-install-recommends",
            *llvm_packages(llvm_version),
        ],
        privileged=True,
    )


def install_clang_tidy(version: str, sha256: str, work_dir: Path) -> Path:
    """Install the hash-pinned clang-tidy wheel under work_dir and return its bin directory."""
    requirements = work_dir / "clang-tidy-requirements.txt"
    requirements.write_text(f"clang-tidy=={version} --hash=sha256:{sha256}\n", encoding="utf-8")
    target = work_dir / "clang-tidy"
    subprocess.run(
        [
            "uv",
            "pip",
            "install",
            "--quiet",
            "--require-hashes",
            "--no-deps",
            "--only-binary",
            ":all:",
            "--target",
            str(target),
            "-r",
            str(requirements),
        ],
        check=True,
    )
    return target / "clang_tidy" / "data" / "bin"


def install(prefix: Path, *, skip_apt: bool, jobs: int) -> None:
    analysis = load_build_config()["analysis"]
    llvm_version = str(analysis["llvm_version"])
    machine = platform.machine()
    hashes = analysis["clang_tidy_sha256"]
    if machine not in hashes:
        raise RuntimeError(f"No clang-tidy wheel pinned for {machine}; pinned: {', '.join(hashes)}")
    if not skip_apt:
        install_llvm(llvm_version)
    prefix.mkdir(parents=True, exist_ok=True)
    llvm_link = prefix / "llvm"
    llvm_link.unlink(missing_ok=True)
    llvm_link.symlink_to(build_clazy.llvm_root(llvm_version))
    install_clang_tidy(analysis["clang_tidy_version"], hashes[machine], prefix)
    with tempfile.TemporaryDirectory(prefix="clazy-") as work_dir:
        build_clazy.build_clazy(build_clazy.read_pin(), prefix / "clazy", Path(work_dir), jobs)
    path = ":".join(str(directory) for directory in bin_dirs(prefix))
    (prefix / "env.sh").write_text(f'export PATH="{path}:$PATH"\n', encoding="utf-8")
    tidy, clazy, llvm = bin_dirs(prefix)
    for tool in (tidy / "clang-tidy", clazy / "clazy-standalone", llvm / "clang-scan-deps"):
        _run([str(tool), "--version"])


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
        "--skip-apt", action="store_true", help="LLVM's apt packages are already installed"
    )
    parser.add_argument("--jobs", type=int, default=default_jobs())
    args = parser.parse_args(argv)

    prefix = args.prefix.resolve()
    try:
        install(prefix, skip_apt=args.skip_apt, jobs=args.jobs)
    except (OSError, RuntimeError, KeyError, subprocess.CalledProcessError) as error:
        log_error(f"Installing the analysis toolchain failed: {error}")
        return 1
    log_ok(f"Analysis toolchain installed; for tools/analyze.py: source {prefix / 'env.sh'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
