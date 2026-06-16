#!/usr/bin/env python3
"""
Cross-compile OpenSSL as Qt-style Android libraries (libssl_3.so / libcrypto_3.so).

Faithful to KDAB android_openssl/build_ssl.sh (same Configure flags, ssl_3.patch symver
workaround, SHLIB_VERSION_NUMBER= , 16 KB page alignment, llvm-strip) but pinned to a single
currently-supported OpenSSL version instead of the EOL 3.1 branch KDAB ships.

Driven from cmake/modules/AndroidOpenSSL.cmake; only the targeted ABIs are passed via --archs.

Usage:
    build_android_openssl.py --version 3.5.6 --tarball <path> --ndk <dir> --patch <ssl_3.patch>
                             --output-root <dir> [--build-dir <dir>] [--api 28]
                             [--archs arm64 arm x86_64 x86]

Requires: perl (always, for OpenSSL's perlasm) and — for the x86/x86_64 *asm* variant — nasm.
arm/arm64 need no extra assembler.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

_tools_dir = Path(__file__).resolve().parents[1]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from _bootstrap import ensure_tools_dir  # noqa: E402

ensure_tools_dir(__file__)

from common.logging import log_error, log_info, log_ok  # noqa: E402

_QT_ABI = {"arm64": "arm64-v8a", "arm": "armeabi-v7a"}
_NEEDS_16K = {"arm64", "x86_64"}
_HOST_TOOLCHAINS = ("linux-x86_64", "darwin-x86_64", "darwin-arm64", "linux-x86")


@dataclass(frozen=True)
class Config:
    version: str
    tarball: Path
    ndk: Path
    patch: Path
    output_root: Path
    build_dir: Path
    api: int
    archs: list[str]
    jobs: int


def qt_abi(arch: str) -> str:
    return _QT_ABI.get(arch, arch)


def find_toolchain(ndk: Path) -> Path:
    """Return the NDK's prebuilt LLVM toolchain bin dir for this host."""
    for host in _HOST_TOOLCHAINS:
        candidate = ndk / "toolchains" / "llvm" / "prebuilt" / host / "bin"
        if candidate.is_dir():
            return candidate
    raise FileNotFoundError(f"No LLVM toolchain under {ndk}/toolchains/llvm/prebuilt")


def run(cmd: list[str], cwd: Path, env: dict[str, str], stdin: int | None = None) -> None:
    log_info(f"  $ {' '.join(cmd)}")
    proc = subprocess.run(
        cmd, cwd=cwd, env=env, stdin=stdin,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout or "")
        raise subprocess.CalledProcessError(proc.returncode, cmd, output=proc.stdout)


def configure_params(arch: str, build_type: str, api: int) -> list[str]:
    params = [build_type] if build_type else []
    params += ["shared", f"android-{arch}", "-U__ANDROID_API__", f"-D__ANDROID_API__={api}"]
    if arch in _NEEDS_16K:
        params += ["-Wl,-z,max-page-size=16384", "-Wl,-z,common-page-size=16384"]
    return params


def stage_output(src: Path, out: Path) -> None:
    """Copy the freshly built libs into out/ and add unversioned compat symlinks."""
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    for pattern in ("libcrypto_*.so", "libssl_*.so", "libcrypto.a", "libssl.a"):
        for f in src.glob(pattern):
            shutil.copy2(f, out / f.name)
    for stem in ("libcrypto", "libssl"):
        versioned = next(iter(src.glob(f"{stem}_*.so")))
        link = out / f"{stem}.so"
        link.unlink(missing_ok=True)
        link.symlink_to(versioned.name)


def copy_headers(src: Path, ssl_root: Path) -> None:
    """Copy ABI-independent headers once per build_type."""
    dest = ssl_root / "include"
    if dest.is_dir():
        return
    shutil.copytree(src / "include", dest)
    for junk in (*dest.rglob("*.in"), *dest.rglob("*.def")):
        junk.unlink()


def build_one(cfg: Config, build_type: str, arch: str, env: dict[str, str]) -> None:
    out_base = cfg.output_root / build_type if build_type else cfg.output_root
    src = cfg.build_dir / f"src-{build_type or 'asm'}-{arch}"
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    subprocess.run(
        ["tar", "xf", str(cfg.tarball), "-C", str(src), "--strip-components=1"], check=True
    )

    with cfg.patch.open("rb") as patch_fh:
        run(["patch", "-p0"], cwd=src, env=env, stdin=patch_fh.fileno())

    label = build_type or "asm"
    log_info(f"QGC: Configuring OpenSSL {cfg.version} {label} {arch} (API {cfg.api})")
    run(["./Configure", *configure_params(arch, build_type, cfg.api)], cwd=src, env=env)
    run(["make", "depend"], cwd=src, env=env)
    run(["make", f"-j{cfg.jobs}", "SHLIB_VERSION_NUMBER=", "build_libs"], cwd=src, env=env)

    libs = [str(p) for p in (*src.glob("libcrypto_*.so"), *src.glob("libssl_*.so"))]
    run(["llvm-strip", "--strip-all", *libs], cwd=src, env=env)

    stage_output(src, out_base / "ssl_3" / qt_abi(arch))
    copy_headers(src, out_base / "ssl_3")
    shutil.rmtree(src)


def parse_args(argv: list[str] | None = None) -> Config:
    p = argparse.ArgumentParser(description="Cross-compile OpenSSL for Android (Qt-style).")
    p.add_argument("--version", required=True)
    p.add_argument("--tarball", required=True, type=Path)
    p.add_argument("--ndk", required=True, type=Path)
    p.add_argument("--patch", required=True, type=Path)
    p.add_argument("--output-root", required=True, type=Path)
    p.add_argument("--build-dir", type=Path, default=Path.cwd() / "openssl-build")
    p.add_argument("--api", type=int, default=23)
    p.add_argument("--archs", nargs="+", default=["arm64", "arm", "x86_64", "x86"])
    p.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    a = p.parse_args(argv)
    return Config(
        version=a.version,
        tarball=a.tarball,
        ndk=a.ndk,
        patch=a.patch,
        output_root=a.output_root,
        build_dir=a.build_dir,
        api=a.api or 23,
        archs=a.archs,
        jobs=a.jobs,
    )


def main(argv: list[str] | None = None) -> int:
    cfg = parse_args(argv)
    for label, path in (("tarball", cfg.tarball), ("patch", cfg.patch)):
        if not path.is_file():
            log_error(f"{label} not found: {path}")
            return 1
    if not cfg.ndk.is_dir():
        log_error(f"NDK not found: {cfg.ndk}")
        return 1

    toolchain = find_toolchain(cfg.ndk)
    env = {
        **os.environ,
        "PATH": f"{toolchain}{os.pathsep}{os.environ.get('PATH', '')}",
        "ANDROID_NDK_ROOT": str(cfg.ndk),
        "ANDROID_NDK_HOME": str(cfg.ndk),
    }

    if cfg.build_dir.exists():
        shutil.rmtree(cfg.build_dir)
    cfg.build_dir.mkdir(parents=True)

    for build_type in ("", "no-asm"):
        for arch in cfg.archs:
            build_one(cfg, build_type, arch, env)

    log_ok(f"QGC: OpenSSL {cfg.version} Android libraries built into {cfg.output_root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
