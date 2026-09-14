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
import re
import sys
from pathlib import Path

_tools_dir = str(Path(__file__).resolve().parent.parent)
if _tools_dir not in sys.path:
    sys.path.insert(0, _tools_dir)

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.build_config import find_build_config, load_build_config
from common.gh_actions import gh_error, github_cache_path, write_github_output
from common.proc import run_checked_with_retry
from qgc_tools.python_env import tool_command

_ARCH_DIR_PREFIXES = [
    ("linux_", ""),
    ("win64_", ""),
]

# Allowlist gates --aqt-source before uv sees it (extra-index-url flag injection, hostile git host).
_AQT_SOURCE_ALLOWLIST = re.compile(
    r"^(?:aqtinstall(?:==[0-9][0-9A-Za-z.\-]*)?"
    r"|git\+https://github\.com/miurahr/aqtinstall(?:\.git)?@[0-9a-f]{7,40})$"
)


def validate_aqt_source(spec: str) -> str:
    """Return `spec` unchanged if it matches the allowlist; sys.exit(1) otherwise."""
    if not spec or _AQT_SOURCE_ALLOWLIST.match(spec):
        return spec
    gh_error(
        f"--aqt-source '{spec}' is not allowed. "
        "Must be 'aqtinstall' (optionally ==<version>) or "
        "'git+https://github.com/miurahr/aqtinstall@<sha>'."
    )
    sys.exit(1)


def resolve_arch_dir(arch: str) -> str:
    """Map a Qt arch identifier to the on-disk directory name aqtinstall creates."""
    arch = arch.removesuffix("_cross_compiled")
    for prefix, replacement in _ARCH_DIR_PREFIXES:
        if arch.startswith(prefix):
            return replacement + arch[len(prefix) :]
    if arch == "clang_64":
        return "macos"
    return arch


def resolve_windows_host_arch(arch: str) -> str:
    """Return the native x64 Qt arch paired with a Windows ARM64 cross arch."""
    suffix = "_arm64_cross_compiled"
    if not arch.startswith("win64_msvc") or not arch.endswith(suffix):
        raise ValueError(f"Not a Windows ARM64 cross-compiled Qt architecture: {arch}")
    return f"{arch.removesuffix(suffix)}_64"


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
            available = (
                ", ".join(sorted(p.name for p in version_dir.iterdir() if p.is_dir())) or "none"
            )
        gh_error(f"Qt root not found at {qt_root}")
        print(f"Expected arch_dir '{arch_dir}' from arch, available: {available}")
        sys.exit(1)
    return qt_root


def resolve_preinstalled_qt(
    prefix: Path,
    version: str,
    arch_dir: str,
    modules: str = "",
    archives: str = "",
) -> Path | None:
    """Return a compatible preinstalled Qt root, or ``None`` when it cannot be reused."""
    if archives:
        return None

    qt_root = prefix / "Qt" / version / arch_dir
    modules_file = qt_root / ".qgc-modules"
    if not (qt_root / "bin").is_dir() or not modules_file.is_file():
        return None

    installed_modules = set(modules_file.read_text(encoding="utf-8").split())
    if not set(modules.split()).issubset(installed_modules):
        return None

    return qt_root


_AQT_MAX_ATTEMPTS = 3
_AQT_RETRY_DELAY_SECONDS = 15


def _run_aqt_with_retries(args: list[str]) -> None:
    """Run aqt, retrying transient CDN download/extraction failures (exit 254, "bad path")."""
    run_checked_with_retry(
        args,
        max_attempts=_AQT_MAX_ATTEMPTS,
        retry_backoff_seconds=_AQT_RETRY_DELAY_SECONDS,
        timeout=1800,
    )


def install_qt(
    host: str,
    target: str,
    version: str,
    arch: str,
    outdir: Path,
    modules: str = "",
    archives: str = "",
    aqt_source: str = "",
    autodesktop: bool = False,
) -> Path:
    """Install Qt using aqtinstall and return the resolved root directory.

    `aqt_source` overrides the PyPI `aqtinstall` package with an isolated uv package
    spec (e.g. `git+https://github.com/miurahr/aqtinstall.git@<sha>`). The explicit
    command bypasses other aqt executables on PATH.
    """
    if aqt_source:
        validate_aqt_source(aqt_source)
    command = tool_command("aqt", "qt", source=aqt_source)
    args = [*command, "install-qt", host, target, version, arch, "--outputdir", str(outdir)]

    if modules:
        args.extend(["--modules", *modules.split()])
    if archives:
        args.extend(["--archives", *archives.split()])
    if autodesktop:
        args.append("--autodesktop")

    print(f"Running: {' '.join(args)}")
    _run_aqt_with_retries(args)

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
    gh_error(f"Failed to resolve an installed Android Qt root for ABIs: {abis}")
    sys.exit(1)


def _add_arch_args(p: argparse.ArgumentParser) -> None:
    p.add_argument("--arch", required=True)
    p.add_argument("--modules", default="")
    p.add_argument("--archives", default="")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Install Qt SDK using aqtinstall.")
    sub = parser.add_subparsers(dest="command", required=True)

    install_p = sub.add_parser("install", help="Install Qt")
    version = install_p.add_mutually_exclusive_group(required=True)
    version.add_argument("--version")
    version.add_argument(
        "--from-config", action="store_true", help="Use the configured Qt version and modules"
    )
    install_p.add_argument("--autodesktop", action="store_true")
    install_p.add_argument("--host", default="linux")
    install_p.add_argument("--target", default="desktop")
    install_p.add_argument("--outdir", type=Path, default=Path(".qt"))
    install_p.add_argument(
        "--aqt-source",
        default="",
        help="Override the isolated uv source for aqtinstall (e.g. git+https://...@<sha>).",
    )
    _add_arch_args(install_p)

    cache_p = sub.add_parser("cache-key", help="Output arch_dir and cache digest")
    cache_p.add_argument("--cache-dir", type=Path, help="Installation directory for cache paths")
    _add_arch_args(cache_p)

    resolve_p = sub.add_parser("resolve-arch", help="Print resolved arch directory name")
    resolve_p.add_argument("--arch", required=True)

    host_arch_p = sub.add_parser(
        "resolve-windows-host-arch", help="Resolve host Qt arch for Windows ARM64 cross builds"
    )
    host_arch_p.add_argument("--arch", required=True)

    paths_p = sub.add_parser(
        "resolve-paths", help="Output qt_root_dir/qt_bin_dir for an installed Qt"
    )
    paths_p.add_argument("--outdir", type=Path, required=True)
    paths_p.add_argument("--version", required=True)
    paths_p.add_argument("--arch-dir", required=True)

    preinstalled_p = sub.add_parser(
        "resolve-preinstalled", help="Resolve a compatible preinstalled Qt SDK"
    )
    preinstalled_p.add_argument("--prefix", default="")
    preinstalled_p.add_argument("--version", required=True)
    preinstalled_p.add_argument("--arch-dir", required=True)
    preinstalled_p.add_argument("--modules", default="")
    preinstalled_p.add_argument("--archives", default="")

    android_p = sub.add_parser(
        "resolve-android-root", help="Pick primary Android Qt root from installed ABIs"
    )
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

    if args.command == "resolve-windows-host-arch":
        try:
            host_arch = resolve_windows_host_arch(args.arch)
        except ValueError as error:
            gh_error(str(error))
            return 1
        write_github_output({"arch": host_arch})
        print(host_arch)
        return 0

    if args.command == "resolve-paths":
        qt_root = args.outdir / args.version / args.arch_dir
        write_github_output(
            {
                "qt_root_dir": str(qt_root),
                "qt_bin_dir": str(qt_root / "bin"),
            }
        )
        print(f"qt_root_dir={qt_root}")
        return 0

    if args.command == "resolve-preinstalled":
        qt_root = (
            resolve_preinstalled_qt(
                Path(args.prefix),
                args.version,
                args.arch_dir,
                args.modules,
                args.archives,
            )
            if args.prefix
            else None
        )
        outputs = {"available": "true" if qt_root else "false"}
        if qt_root:
            outputs.update(
                {
                    "qt_root_dir": str(qt_root),
                    "qt_bin_dir": str(qt_root / "bin"),
                }
            )
        write_github_output(outputs)
        print(f"available={outputs['available']}")
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
        if args.cache_dir is not None:
            write_github_output({"cache_dir": github_cache_path(args.cache_dir)})
        print(f"arch_dir={arch_dir}")
        print(f"digest={digest}")
        return 0

    # Default: install
    if args.from_config:
        config_path = find_build_config(
            start=Path(__file__).parent,
            extra_candidates=[Path(__file__).parent / "build-config.json"],
        )
        qt_config = load_build_config(config_path)["qt"]
        args.version = qt_config["version"]
        args.modules = args.modules or qt_config["modules"]
    arch_dir = resolve_arch_dir(args.arch)
    qt_root = install_qt(
        host=args.host,
        target=args.target,
        version=args.version,
        arch=args.arch,
        outdir=args.outdir,
        modules=args.modules,
        archives=args.archives,
        aqt_source=args.aqt_source,
        autodesktop=args.autodesktop,
    )

    write_github_output(
        {
            "arch_dir": arch_dir,
            "qt_root_dir": str(qt_root),
            "qt_bin_dir": str(qt_root / "bin"),
        }
    )
    print(f"Qt installed at {qt_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
