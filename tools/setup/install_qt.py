#!/usr/bin/env python3
"""
Install Qt SDK using aqtinstall with architecture resolution.

Wraps aqtinstall with QGC-specific arch-directory mapping. The qt-install
GitHub Action calls `plan` (cache key, cache path and a reusable preinstalled
SDK) before restoring its cache and `ensure` (reuse or install, then export the
environment) after it.

Usage:
    python tools/setup/install_qt.py install --version 6.8.3 --host linux --arch linux_gcc_64
    python tools/setup/install_qt.py install --from-config --host mac --arch clang_64
    python tools/setup/install_qt.py plan --version 6.8.3 --arch linux_gcc_64 --outdir .qt/Qt
    python tools/setup/install_qt.py resolve-arch --arch win64_msvc2022_64
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.build_config import find_build_config, load_build_config
from common.gh_actions import (
    append_github_env,
    append_github_path,
    gh_error,
    github_cache_path,
    parse_bool,
    write_github_output,
)
from common.proc import run_with_retry
from qgc_tools.python_env import tool_command

_ARCH_DIR_PREFIXES = [
    ("linux_", ""),
    ("win64_", ""),
]


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
    run_with_retry(
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
    autodesktop: bool = False,
) -> Path:
    """Install Qt using the locked aqtinstall and return the resolved root directory.

    The explicit command bypasses other aqt executables on PATH.
    """
    command = tool_command("aqt", "qt")
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


def plan_install(
    outdir: Path,
    version: str,
    arch: str,
    modules: str = "",
    archives: str = "",
    preinstalled_prefix: str = "",
) -> dict[str, str]:
    """Return the cache digest, workspace-relative cache path and any reusable preinstalled root."""
    arch_dir = resolve_arch_dir(arch)
    preinstalled = (
        resolve_preinstalled_qt(Path(preinstalled_prefix), version, arch_dir, modules, archives)
        if preinstalled_prefix
        else None
    )
    return {
        "digest": compute_cache_digest(modules, archives),
        "cache_path": github_cache_path(outdir / version / arch_dir),
        "preinstalled_root": str(preinstalled or ""),
    }


def ensure_qt(
    host: str,
    target: str,
    version: str,
    arch: str,
    outdir: Path,
    modules: str = "",
    archives: str = "",
    preinstalled_root: str = "",
    cache_hit: bool = False,
) -> Path:
    """Reuse a preinstalled or cache-restored Qt, installing it otherwise."""
    if preinstalled_root:
        print(f"Using preinstalled Qt at {preinstalled_root}")
        return Path(preinstalled_root)
    if cache_hit:
        qt_root = resolve_qt_root(outdir, version, resolve_arch_dir(arch))
        print(f"Using cached Qt at {qt_root}")
        return qt_root
    qt_root = install_qt(host, target, version, arch, outdir, modules, archives)
    print(f"Qt installed at {qt_root}")
    return qt_root


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
    _add_arch_args(install_p)

    plan_p = sub.add_parser(
        "plan", help="Output the cache digest/path and any compatible preinstalled Qt root"
    )
    plan_p.add_argument("--preinstalled-prefix", default="")

    ensure_p = sub.add_parser(
        "ensure", help="Reuse a preinstalled or cached Qt or install it, then export its paths"
    )
    ensure_p.add_argument("--host", required=True)
    ensure_p.add_argument("--target", default="desktop")
    ensure_p.add_argument("--preinstalled-root", default="")
    ensure_p.add_argument("--cache-hit", type=parse_bool, default=False)
    ensure_p.add_argument(
        "--export-env",
        type=parse_bool,
        default=True,
        help="Export QT_ROOT_DIR and prepend the Qt bin directory to PATH",
    )

    for action_p in (plan_p, ensure_p):
        action_p.add_argument("--version", required=True)
        action_p.add_argument("--outdir", type=Path, required=True)
        _add_arch_args(action_p)

    resolve_p = sub.add_parser("resolve-arch", help="Print resolved arch directory name")
    resolve_p.add_argument("--arch", required=True)

    host_arch_p = sub.add_parser(
        "resolve-windows-host-arch", help="Resolve host Qt arch for Windows ARM64 cross builds"
    )
    host_arch_p.add_argument("--arch", required=True)

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

    if args.command == "plan":
        outputs = plan_install(
            args.outdir,
            args.version,
            args.arch,
            args.modules,
            args.archives,
            args.preinstalled_prefix,
        )
        write_github_output(outputs)
        for key, value in outputs.items():
            print(f"{key}={value}")
        return 0

    if args.command == "ensure":
        qt_root = ensure_qt(
            host=args.host,
            target=args.target,
            version=args.version,
            arch=args.arch,
            outdir=args.outdir,
            modules=args.modules,
            archives=args.archives,
            preinstalled_root=args.preinstalled_root,
            cache_hit=args.cache_hit,
        )
        write_github_output({"qt_root_dir": str(qt_root)})
        if args.export_env:
            append_github_env({"QT_ROOT_DIR": str(qt_root)})
            append_github_path(str(qt_root / "bin"))
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
    qt_root = install_qt(
        host=args.host,
        target=args.target,
        version=args.version,
        arch=args.arch,
        outdir=args.outdir,
        modules=args.modules,
        archives=args.archives,
        autodesktop=args.autodesktop,
    )
    print(f"Qt installed at {qt_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
