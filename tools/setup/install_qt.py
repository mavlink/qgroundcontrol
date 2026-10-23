#!/usr/bin/env python3
"""
Install the Qt SDK with QGC's installer (tools/setup/qt_installer) or the locked aqtinstall.

`--installer` (or QGC_QT_INSTALLER) picks the tool; QGC's installer is the default and aqt the
fallback. The qt-install GitHub Action calls `plan` (Qt version, installer, cache key, cache path
and a reusable preinstalled SDK) before restoring its cache and `ensure` (reuse or install, then
export the environment) after it. `compare` checks that both installers produce the same tree.

Usage:
    python tools/setup/install_qt.py install --version 6.8.3 --host linux --arch linux_gcc_64
    python tools/setup/install_qt.py install --from-config --host mac --arch clang_64
    python tools/setup/install_qt.py plan --host linux --arch linux_gcc_64 --outdir .qt/Qt
    python tools/setup/install_qt.py compare --version 6.12.0 --arch linux_gcc_64 \
        --reference aqt/Qt --candidate qgc/Qt
"""

from __future__ import annotations

import argparse
import hashlib
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.gh_actions import (
    append_github_env,
    append_github_path,
    gh_error,
    github_cache_path,
    parse_bool,
    write_github_output,
    write_step_summary,
)
from common.proc import run_with_retry
from qgc_tools.python_env import tool_command
from setup.qt_installer.compare import compare_trees, render_markdown
from setup.qt_installer.install import ensure_host_desktop, install
from setup.qt_installer.repo import (
    QtRequest,
    resolve_arch_dir,
    resolve_windows_host_arch,
)

INSTALLERS = ("qgc", "aqt")


DEFAULT_INSTALLER = "qgc"


def resolve_installer(requested: str) -> str:
    """Return `--installer`, else $QGC_QT_INSTALLER, else QGC's installer (aqt stays a fallback)."""
    installer = requested or os.environ.get("QGC_QT_INSTALLER", "") or DEFAULT_INSTALLER
    if installer not in INSTALLERS:
        raise ValueError(f"Unknown Qt installer {installer!r}; use one of: {', '.join(INSTALLERS)}")
    return installer


def compute_cache_digest(modules: str, archives: str, installer: str = "aqt") -> str:
    """Generate a SHA-256 digest for cache key differentiation.

    aqt keeps the original inputs, so its existing caches stay valid; other installers add their
    name, so neither tool ever restores the other's tree.
    """
    content = f"{modules}\n{archives}"
    if installer != "aqt":
        content += f"\ninstaller={installer}"
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
    installer: str = "",
) -> Path:
    """Install Qt and return the resolved root directory.

    Both installers get the absolute output directory, which their patched files embed.
    """
    outdir = outdir.absolute()
    if resolve_installer(installer) == "qgc":
        request = QtRequest(
            host, target, version, arch, tuple(modules.split()), tuple(archives.split()) or None
        )
        if autodesktop:
            ensure_host_desktop(request, outdir)
        install(request, outdir)
    else:
        _install_with_aqt(host, target, version, arch, outdir, modules, archives, autodesktop)
    return resolve_qt_root(outdir, version, resolve_arch_dir(arch))


def _install_with_aqt(
    host: str,
    target: str,
    version: str,
    arch: str,
    outdir: Path,
    modules: str,
    archives: str,
    autodesktop: bool,
) -> None:
    """Run the locked aqtinstall; the explicit command bypasses other aqt executables on PATH."""
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


def configured_qt() -> dict[str, str]:
    """Return the ``qt`` section of build-config.json."""
    return load_build_config()["qt"]


def plan_install(
    outdir: Path,
    version: str,
    arch: str,
    modules: str = "",
    archives: str = "",
    preinstalled_prefix: str = "",
    installer: str = "aqt",
) -> dict[str, str]:
    """Return the Qt version, cache digest, cache path, preinstalled root and installer."""
    arch_dir = resolve_arch_dir(arch)
    preinstalled = (
        resolve_preinstalled_qt(Path(preinstalled_prefix), version, arch_dir, modules, archives)
        if preinstalled_prefix
        else None
    )
    return {
        "version": version,
        "digest": compute_cache_digest(modules, archives, installer),
        "cache_path": github_cache_path(outdir / version / arch_dir),
        "preinstalled_root": str(preinstalled or ""),
        "installer": installer,
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
    installer: str = "",
) -> Path:
    """Reuse a preinstalled or cache-restored Qt, installing it otherwise."""
    if preinstalled_root:
        print(f"Using preinstalled Qt at {preinstalled_root}")
        return Path(preinstalled_root)
    if cache_hit:
        qt_root = resolve_qt_root(outdir, version, resolve_arch_dir(arch))
        print(f"Using cached Qt at {qt_root}")
        return qt_root
    qt_root = install_qt(
        host, target, version, arch, outdir, modules, archives, installer=installer
    )
    print(f"Qt installed at {qt_root}")
    return qt_root


def compare_installs(reference: Path, candidate: Path, version: str, arch: str) -> int:
    """Report how the QGC installer's tree differs from aqt's; 1 when beyond the known differences."""
    arch_root = f"{version}/{resolve_arch_dir(arch)}"
    result = compare_trees(reference.absolute(), candidate.absolute(), arch_root)
    report = render_markdown(f"Qt {version} {arch}", result)
    print(report)
    write_step_summary(report)
    return 0 if result.ok else 1


def _add_arch_args(p: argparse.ArgumentParser) -> None:
    p.add_argument("--arch", required=True)
    p.add_argument("--modules", default="")
    p.add_argument("--archives", default="")
    p.add_argument(
        "--installer",
        choices=("", *INSTALLERS),
        default="",
        help="Default: $QGC_QT_INSTALLER, else qgc (aqt remains as a fallback)",
    )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Install the Qt SDK.")
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
        "plan",
        help="Output the Qt version, installer, cache digest/path and any reusable preinstalled Qt root",
    )
    plan_p.add_argument("--version", default="", help="Default: build-config.json's qt.version")
    plan_p.add_argument("--host", default="linux")
    plan_p.add_argument("--target", default="desktop")
    plan_p.add_argument("--preinstalled-prefix", default="")

    ensure_p = sub.add_parser(
        "ensure", help="Reuse a preinstalled or cached Qt or install it, then export its paths"
    )
    ensure_p.add_argument("--version", required=True)
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
        action_p.add_argument("--outdir", type=Path, required=True)
        _add_arch_args(action_p)

    host_arch_p = sub.add_parser(
        "resolve-windows-host-arch", help="Resolve host Qt arch for Windows ARM64 cross builds"
    )
    host_arch_p.add_argument("--arch", required=True)

    compare_p = sub.add_parser(
        "compare", help="Compare an aqt-installed Qt tree with QGC's installer's (parity check)"
    )
    compare_p.add_argument("--version", required=True)
    compare_p.add_argument("--arch", required=True)
    compare_p.add_argument("--reference", type=Path, required=True, help="aqt output directory")
    compare_p.add_argument("--candidate", type=Path, required=True, help="qgc output directory")

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.command == "resolve-windows-host-arch":
        try:
            host_arch = resolve_windows_host_arch(args.arch)
        except ValueError as error:
            gh_error(str(error))
            return 1
        write_github_output({"arch": host_arch})
        print(host_arch)
        return 0

    if args.command == "compare":
        return compare_installs(args.reference, args.candidate, args.version, args.arch)

    try:
        installer = resolve_installer(args.installer)
    except ValueError as error:
        gh_error(str(error))
        return 1

    if args.command == "plan":
        outputs = plan_install(
            args.outdir,
            args.version or configured_qt()["version"],
            args.arch,
            args.modules,
            args.archives,
            args.preinstalled_prefix,
            installer,
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
            installer=installer,
        )
        write_github_output({"qt_root_dir": str(qt_root)})
        if args.export_env:
            append_github_env({"QT_ROOT_DIR": str(qt_root)})
            append_github_path(str(qt_root / "bin"))
        return 0

    # Default: install
    if args.from_config:
        qt_config = configured_qt()
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
        installer=installer,
    )
    print(f"Qt installed at {qt_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
