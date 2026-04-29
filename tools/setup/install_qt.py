#!/usr/bin/env python3
"""
Install Qt SDK using aqtinstall with architecture resolution.

Wraps aqtinstall with QGC-specific arch-directory mapping, cache key
generation, and path resolution. Used by the qt-install GitHub Action.

Usage:
    python tools/setup/install_qt.py --version 6.8.3 --host linux --arch linux_gcc_64
    python tools/setup/install_qt.py --version 6.8.3 --host mac --arch clang_64 --modules "qtgraphs qtlocation"
    python tools/setup/install_qt.py cache-key --arch linux_gcc_64 --modules "qtgraphs"
    python tools/setup/install_qt.py resolve-arch --arch win64_msvc2022_64
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

_tools_dir = str(Path(__file__).resolve().parent.parent)
if _tools_dir not in sys.path:
    sys.path.insert(0, _tools_dir)

from common.gh_actions import write_github_output

# aqtinstall creates directories that differ from the arch parameter.
# This mapping resolves the actual on-disk directory name.
_ARCH_DIR_MAP: dict[str, str] = {
    "win64_msvc2022_arm64_cross_compiled": "msvc2022_arm64",
}

_ARCH_DIR_PREFIXES = [
    ("linux_", ""),
    ("win64_", ""),
]


def resolve_arch_dir(arch: str) -> str:
    """Map a Qt arch identifier to the on-disk directory name aqtinstall creates."""
    if arch in _ARCH_DIR_MAP:
        return _ARCH_DIR_MAP[arch]
    for prefix, replacement in _ARCH_DIR_PREFIXES:
        if arch.startswith(prefix):
            return replacement + arch[len(prefix):]
    if arch == "clang_64":
        return "macos"
    return arch


def compute_cache_digest(modules: str, archives: str) -> str:
    """Generate a SHA-256 digest for cache key differentiation."""
    content = f"{modules}\n{archives}"
    return hashlib.sha256(content.encode()).hexdigest()


def resolve_qt_root(outdir: Path, version: str, arch_dir: str) -> Path:
    """Resolve and validate the Qt root directory after installation."""
    qt_root = outdir / version / arch_dir
    if not qt_root.is_dir():
        available = "none"
        version_dir = outdir / version
        if version_dir.is_dir():
            available = ", ".join(sorted(p.name for p in version_dir.iterdir() if p.is_dir())) or "none"
        print(f"::error::Qt root not found at {qt_root}")
        print(f"Expected arch_dir '{arch_dir}' from arch, available: {available}")
        sys.exit(1)
    return qt_root


def install_qt(
    host: str,
    target: str,
    version: str,
    arch: str,
    outdir: Path,
    modules: str = "",
    archives: str = "",
) -> Path:
    """Install Qt using aqtinstall and return the resolved root directory."""
    aqt = shutil.which("aqt")
    if not aqt:
        from common import pip_install
        pip_install(["aqtinstall"])
        aqt = shutil.which("aqt")
        if not aqt:
            print("::error::aqtinstall not found after pip install")
            sys.exit(1)

    args = [aqt, "install-qt", host, target, version, arch, "--outputdir", str(outdir)]

    if modules:
        args.extend(["--modules", *modules.split()])
    if archives:
        args.extend(["--archives", *archives.split()])

    print(f"Running: {' '.join(args)}")
    subprocess.run(args, check=True)

    arch_dir = resolve_arch_dir(arch)
    return resolve_qt_root(outdir, version, arch_dir)


# Android ABI → aqtinstall arch mapping (priority order for resolution)
_ANDROID_ABI_ORDER = [
    ("arm64-v8a", "arm64"),
    ("armeabi-v7a", "armv7"),
    ("x86_64", "x86_64"),
    ("x86", "x86"),
]


def resolve_android_qt_root(abis: str, roots: dict[str, str]) -> str:
    """Pick the primary Android Qt root from installed ABIs.

    Args:
        abis: Semicolon-separated ABI list (e.g. "arm64-v8a;x86_64")
        roots: Mapping of short ABI key → Qt root path (e.g. {"arm64": "/path/to/qt"})

    Returns:
        The Qt root path for the highest-priority installed ABI.

    Raises:
        SystemExit: If no matching ABI is found.
    """
    abi_set = {a.strip() for a in abis.split(";") if a.strip()}
    for abi, key in _ANDROID_ABI_ORDER:
        if abi in abi_set and roots.get(key):
            return roots[key]
    print(f"::error::Failed to resolve an installed Android Qt root for ABIs: {abis}")
    sys.exit(1)


def _add_arch_args(p: argparse.ArgumentParser) -> None:
    p.add_argument("--arch", required=True)
    p.add_argument("--modules", default="")
    p.add_argument("--archives", default="")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Install Qt SDK using aqtinstall.")
    sub = parser.add_subparsers(dest="command", required=True)

    install_p = sub.add_parser("install", help="Install Qt")
    install_p.add_argument("--version", required=True)
    install_p.add_argument("--host", default="linux")
    install_p.add_argument("--target", default="desktop")
    install_p.add_argument("--outdir", type=Path, default=Path(".qt"))
    _add_arch_args(install_p)

    cache_p = sub.add_parser("cache-key", help="Output arch_dir and cache digest")
    _add_arch_args(cache_p)

    resolve_p = sub.add_parser("resolve-arch", help="Print resolved arch directory name")
    resolve_p.add_argument("--arch", required=True)

    android_p = sub.add_parser("resolve-android-root", help="Pick primary Android Qt root from installed ABIs")
    android_p.add_argument("--abis", required=True, help="Semicolon-separated ABI list")
    android_p.add_argument("--arm64", default="")
    android_p.add_argument("--armv7", default="")
    android_p.add_argument("--x86-64", default="", dest="x86_64")
    android_p.add_argument("--x86", default="")

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.command == "resolve-arch":
        print(resolve_arch_dir(args.arch))
        return 0

    if args.command == "resolve-android-root":
        roots = {"arm64": args.arm64, "armv7": args.armv7, "x86_64": args.x86_64, "x86": args.x86}
        qt_root = resolve_android_qt_root(args.abis, roots)
        write_github_output({"qt_root_dir": qt_root})
        print(f"qt_root_dir={qt_root}")
        return 0

    if args.command == "cache-key":
        arch_dir = resolve_arch_dir(args.arch)
        digest = compute_cache_digest(args.modules, args.archives)
        write_github_output({"arch_dir": arch_dir, "digest": digest})
        print(f"arch_dir={arch_dir}")
        print(f"digest={digest}")
        return 0

    # Default: install
    arch_dir = resolve_arch_dir(args.arch)
    qt_root = install_qt(
        host=args.host,
        target=args.target,
        version=args.version,
        arch=args.arch,
        outdir=args.outdir,
        modules=args.modules,
        archives=args.archives,
    )

    write_github_output({
        "arch_dir": arch_dir,
        "qt_root_dir": str(qt_root),
        "qt_bin_dir": str(qt_root / "bin"),
    })
    print(f"Qt installed at {qt_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
