#!/usr/bin/env python3
"""
Ccache helper for CI: install, report, and gate cache saves.

Subcommands:
    install    Install the pinned compiler tools for this runner (ccache; mold and the
               sccache pilot on Linux)
    save-gate  Decide whether the compiler and moc caches are worth saving

The build's cache hit rates are part of the job summary's Build section
(cmake_helper.py summary).

Examples:
    ccache_helper.py install
    ccache_helper.py install --compiler-cache sccache
    ccache_helper.py save-gate --min-misses 20
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path
from typing import NamedTuple

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.gh_actions import (
    append_github_env,
    append_github_path,
    write_github_output,
)
from common.io import read_json
from common.net import fetch_release_member
from common.platform import host_arch, is_linux, is_macos, is_windows
from common.proc import run_captured
from common.tool_version import probe_version, version_prefix_matches
from qgc_tools.compiler_cache import ccache_stats

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------

_LINUX_BIN = Path("/usr/local/bin")


class PinnedRelease(NamedTuple):
    version: str
    sha256: dict[str, str]
    url: str


def pinned_release(tool: str) -> PinnedRelease:
    """Return build-config.json's ``tools.<tool>`` version, per-platform digests and asset URL."""
    pin = load_build_config()["tools"][tool]
    url = f"https://github.com/{pin['repository']}/releases/download/v{pin['version']}"
    return PinnedRelease(pin["version"], pin["sha256"], url)


def _run(command: list[str]) -> str:
    result = run_captured(command)
    if result.returncode != 0:
        raise RuntimeError(f"{' '.join(command)} failed: {result.stderr.strip()}")
    return result.stdout.strip()


def _on_path(tool: str, version: str) -> bool:
    """Report whether ``tool`` on PATH already has the pinned ``version``."""
    installed = probe_version(tool)
    if installed is None or not version_prefix_matches(installed, version):
        return False
    print(f"{tool} {version} already installed")
    return True


def _install_pinned(url: str, sha256: str, member: str, dest: Path, *, sudo: bool = False) -> Path:
    """Install the ``member`` binary of a SHA-256-verified release archive as ``dest``."""
    with tempfile.TemporaryDirectory() as temp_dir:
        source = fetch_release_member(url, sha256, member, Path(temp_dir))
        if sudo:
            _run(["sudo", "install", "-D", "-m", "0755", str(source), str(dest)])
        else:
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, dest)
            dest.chmod(0o755)
    print(_run([str(dest), "--version"]))
    return dest


def install_compiler_tools(runner_temp: Path | None, compiler_cache: str = "ccache") -> None:
    """Install this runner's pinned compiler tools and expose them to later steps.

    Linux installs ccache and mold under /usr/local unless the pinned versions are
    already on PATH, and for the sccache pilot also sccache, which it selects as
    the compiler launcher. macOS and Windows install ccache under ``runner_temp``
    and add it to PATH.
    """
    if compiler_cache == "sccache" and not is_linux():
        raise RuntimeError(f"the sccache pilot has no pinned binary for {sys.platform}")
    arch = host_arch()
    ccache = pinned_release("ccache")
    if is_linux():
        stem = f"ccache-{ccache.version}-linux-{arch}-glibc"
        if not _on_path("ccache", ccache.version):
            _install_pinned(
                f"{ccache.url}/{stem}.tar.xz",
                ccache.sha256[f"linux-{arch}-glibc"],
                f"{stem}/ccache",
                _LINUX_BIN / "ccache",
                sudo=True,
            )
        mold = pinned_release("mold")
        stem = f"mold-{mold.version}-{arch}-linux"
        if not _on_path("mold", mold.version):
            _install_pinned(
                f"{mold.url}/{stem}.tar.gz",
                mold.sha256[arch],
                f"{stem}/bin/mold",
                _LINUX_BIN / "mold",
                sudo=True,
            )
            _run(["sudo", "ln", "-sf", "mold", str(_LINUX_BIN / "ld.mold")])
        if compiler_cache == "sccache":
            sccache = pinned_release("sccache")
            binary = _LINUX_BIN / "sccache"
            if _on_path("sccache", sccache.version):
                binary = Path(shutil.which("sccache") or binary)
            else:
                stem = f"sccache-v{sccache.version}-{arch}-unknown-linux-musl"
                _install_pinned(
                    f"{sccache.url}/{stem}.tar.gz",
                    sccache.sha256[arch],
                    f"{stem}/sccache",
                    binary,
                    sudo=True,
                )
            # CMake prefers ccache on PATH, so name the launcher explicitly.
            append_github_env({"QGC_CACHE_PROGRAM": str(binary)})
        return
    if runner_temp is None:
        raise RuntimeError("RUNNER_TEMP is required to install ccache on macOS and Windows")
    if is_macos():
        stem = f"ccache-{ccache.version}-darwin"
        # A dedicated prefix keeps /usr/local/bin (system python3) from shadowing tools/.venv on PATH.
        binary = _install_pinned(
            f"{ccache.url}/{stem}.tar.gz",
            ccache.sha256["darwin"],
            f"{stem}/ccache",
            runner_temp / "ccache" / "bin" / "ccache",
        )
        append_github_env({"QGC_CACHE_PROGRAM": str(binary)})
    elif is_windows():
        stem = f"ccache-{ccache.version}-windows-{arch}"
        binary = _install_pinned(
            f"{ccache.url}/{stem}.zip",
            ccache.sha256[f"windows-{arch}"],
            f"{stem}/ccache.exe",
            runner_temp / stem / "ccache.exe",
        )
    else:
        raise RuntimeError(f"ccache installation is not supported on {sys.platform}")
    append_github_path(str(binary.parent))


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Save gate
# ---------------------------------------------------------------------------

# `ccache --show-stats` text: "Misses: N / total" (4.5+) or "cache miss N" (older).
_SHOW_STATS_MISSES = re.compile(r"^\s*(?:Misses:|cache miss)\s+(\d+)", re.MULTILINE)


def should_save_cache(event_name: str, misses: int | None, min_misses: int) -> bool:
    """Pull requests skip re-uploading a cache that the build barely changed.

    Every save is a new multi-GB archive, so a handful of new entries does not
    justify it; the next build recompiles them, and once their number reaches
    ``min_misses`` that build saves again. Unknown statistics always save.
    """
    if event_name != "pull_request" or min_misses <= 0 or misses is None:
        return True
    return misses >= min_misses


def ccache_misses(docker_report: Path | None = None) -> int | None:
    """Return this job's ccache misses, or None when unknown.

    Host builds reset the statistics before compiling. Docker builds reset them
    inside the container and record ``ccache --show-stats`` in their report.
    """
    if docker_report is not None:
        try:
            text = read_json(docker_report).get("ccache")
        except (OSError, ValueError, AttributeError):
            return None
        match = _SHOW_STATS_MISSES.search(text) if isinstance(text, str) else None
        return int(match.group(1)) if match else None
    stats = ccache_stats()
    misses = stats.get("cache_miss") if isinstance(stats, dict) else None
    return misses if isinstance(misses, int) else None


def moccache_misses(cache_dir: Path) -> int | None:
    """Return this job's moccache misses from the stats log reset before the build.

    None when the log recorded no moc runs, e.g. when moc ran in a container.
    """
    try:
        text = (cache_dir / "stats.log").read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    kinds = [line.split("\t", 1)[0] for line in text.splitlines()]
    misses = sum(kind == "miss" or kind.startswith("miss-") for kind in kinds)
    if not misses and "hit" not in kinds:
        return None
    return misses


def parse_min_misses(value: str) -> int:
    """Parse the threshold; an invalid value disables the gate rather than failing the job."""
    try:
        return int(value.strip() or 0)
    except ValueError:
        print(f"Invalid cache save threshold {value!r}; saving unconditionally", file=sys.stderr)
        return 0


def run_save_gate(
    event_name: str, min_misses: int, docker_report: Path | None, moccache_dir: Path
) -> None:
    """Write ``ccache`` and ``moccache`` save decisions to GITHUB_OUTPUT."""
    decisions: dict[str, str] = {}
    for name, label, misses in (
        ("ccache", "compiler", ccache_misses(docker_report)),
        ("moccache", "moc", moccache_misses(moccache_dir)),
    ):
        save = should_save_cache(event_name, misses, min_misses)
        decisions[name] = str(save).lower()
        if save:
            print(f"Saving the {label} cache (misses: {'unknown' if misses is None else misses})")
        else:
            print(f"Skipping the {label} cache save: {misses} misses, PR threshold {min_misses}")
    write_github_output(decisions)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Ccache helper for CI: install, report, and gate cache saves",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    # -- install -------------------------------------------------------
    inst = sub.add_parser(
        "install", help="Install the pinned compiler tools for this runner (ccache; mold on Linux)"
    )
    inst.add_argument(
        "--runner-temp",
        type=Path,
        default=os.environ.get("RUNNER_TEMP") or None,
        help="macOS/Windows install root (default: $RUNNER_TEMP)",
    )
    inst.add_argument(
        "--compiler-cache",
        choices=("ccache", "sccache"),
        default=os.environ.get("QGC_COMPILER_CACHE_TOOL") or "ccache",
        help="Compiler launcher; sccache is a Linux-only pilot (default: $QGC_COMPILER_CACHE_TOOL)",
    )

    # -- save-gate -----------------------------------------------------
    gate = sub.add_parser("save-gate", help="Decide whether the compiler and moc caches are saved")
    gate.add_argument("--event-name", default=os.environ.get("GITHUB_EVENT_NAME", ""))
    gate.add_argument(
        "--min-misses",
        default=os.environ.get("QGC_PR_CACHE_SAVE_MIN_MISSES", "0"),
        help="Pull requests save a cache only with at least this many misses (0: always)",
    )
    gate.add_argument(
        "--docker-report", type=Path, help="Docker build report with the container's ccache stats"
    )
    gate.add_argument(
        "--moccache-dir",
        type=Path,
        default=Path(os.environ.get("GITHUB_WORKSPACE", ".")) / ".cache" / "moccache",
    )

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    if args.command == "install":
        install_compiler_tools(args.runner_temp, args.compiler_cache)
        return 0

    if args.command == "save-gate":
        run_save_gate(
            args.event_name,
            parse_min_misses(args.min_misses),
            args.docker_report,
            args.moccache_dir,
        )
        return 0

    print(
        "Error: a subcommand is required (install, save-gate)",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
