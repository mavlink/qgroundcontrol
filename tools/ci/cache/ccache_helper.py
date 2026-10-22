#!/usr/bin/env python3
"""
Ccache helper for CI: install, report, and gate cache saves.

Subcommands:
    install    Install the pinned compiler tools for this runner (ccache; mold and the
               sccache pilot on Linux)
    summary    Write a build cache hit/miss summary to GitHub Step Summary
    save-gate  Decide whether the compiler and moc caches are worth saving

Examples:
    ccache_helper.py install
    ccache_helper.py install --compiler-cache sccache
    ccache_helper.py summary
    ccache_helper.py save-gate --min-misses 20
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    append_github_env,
    append_github_path,
    write_github_output,
    write_step_summary,
)
from common.io import extract_tar_data, extract_zip_safe
from common.markdown import md_table
from common.net import download_file
from common.platform import host_arch, is_linux, is_macos, is_windows
from common.proc import run_captured
from common.tool_version import probe_version, version_prefix_matches

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------

# Digests are bound to their version: bump both together. ccache and sccache publish their release
# asset digests; mold publishes none, so its digests were computed from the release tarballs.
CCACHE_VERSION = "4.13.6"
CCACHE_SHA256 = {
    "linux-x86_64-glibc": "508b2a1217dc6e04a23e967c7b95a0fb45d8a7e16fde9e180919698f2e2be060",
    "linux-aarch64-glibc": "9cdd30c768a5b06d27bea8ec28b138735c30910c1e845b66f522687ae950dadc",
    "windows-x86_64": "3d7cebb05850ad704e197b3f1d3f0f924ab6c9fdfc561578e146184fe9d89380",
    "windows-aarch64": "bec01846b06d6d87bf35eda50544d7c8bf9b9a4859f218417a7081aa45d7fd47",
    "darwin": "0274210ec9c9936ed5711d59b0de3167a51216a588ddde35f6bc828f366fe6d9",
}
MOLD_VERSION = "2.41.0"
MOLD_SHA256 = {
    "x86_64": "a3696680d99e692970590a178bc3a33d78d60d1c6dc9db7a11b557b02b751f5d",
    "aarch64": "946de2774b06a71346bd59b55fddba610b65b8d93c3a4a1559cc84e103472710",
}
SCCACHE_VERSION = "0.18.0"
SCCACHE_SHA256 = {
    "x86_64": "45f1447fbe231e3037bde351ef70677dd212216c8d62ae7ca409fecc4d6acc89",
    "aarch64": "2b3284d5da3b46a47dc4229e75bb7b88ac4aa99c8d754fb7d2f84997e5a4354a",
}

_CCACHE_RELEASE_URL = f"https://github.com/ccache/ccache/releases/download/v{CCACHE_VERSION}"
_MOLD_RELEASE_URL = f"https://github.com/rui314/mold/releases/download/v{MOLD_VERSION}"
_SCCACHE_RELEASE_URL = f"https://github.com/mozilla/sccache/releases/download/v{SCCACHE_VERSION}"
_LINUX_BIN = Path("/usr/local/bin")


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
    archive_name = url.rsplit("/", 1)[-1]
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        archive = temp_path / archive_name
        download_file(url, archive, expected_sha256=sha256)
        if archive_name.endswith(".zip"):
            extract_zip_safe(archive, temp_path)
        else:
            extract_tar_data(archive, temp_path)
        source = temp_path / member
        if not source.is_file():
            raise FileNotFoundError(f"{member} not found in {archive_name}")
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
    if is_linux():
        stem = f"ccache-{CCACHE_VERSION}-linux-{arch}-glibc"
        if not _on_path("ccache", CCACHE_VERSION):
            _install_pinned(
                f"{_CCACHE_RELEASE_URL}/{stem}.tar.xz",
                CCACHE_SHA256[f"linux-{arch}-glibc"],
                f"{stem}/ccache",
                _LINUX_BIN / "ccache",
                sudo=True,
            )
        stem = f"mold-{MOLD_VERSION}-{arch}-linux"
        if not _on_path("mold", MOLD_VERSION):
            _install_pinned(
                f"{_MOLD_RELEASE_URL}/{stem}.tar.gz",
                MOLD_SHA256[arch],
                f"{stem}/bin/mold",
                _LINUX_BIN / "mold",
                sudo=True,
            )
            _run(["sudo", "ln", "-sf", "mold", str(_LINUX_BIN / "ld.mold")])
        if compiler_cache == "sccache":
            binary = _LINUX_BIN / "sccache"
            if _on_path("sccache", SCCACHE_VERSION):
                binary = Path(shutil.which("sccache") or binary)
            else:
                stem = f"sccache-v{SCCACHE_VERSION}-{arch}-unknown-linux-musl"
                _install_pinned(
                    f"{_SCCACHE_RELEASE_URL}/{stem}.tar.gz",
                    SCCACHE_SHA256[arch],
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
        stem = f"ccache-{CCACHE_VERSION}-darwin"
        # A dedicated prefix keeps /usr/local/bin (system python3) from shadowing tools/.venv on PATH.
        binary = _install_pinned(
            f"{_CCACHE_RELEASE_URL}/{stem}.tar.gz",
            CCACHE_SHA256["darwin"],
            f"{stem}/ccache",
            runner_temp / "ccache" / "bin" / "ccache",
        )
        append_github_env({"QGC_CACHE_PROGRAM": str(binary)})
    elif is_windows():
        stem = f"ccache-{CCACHE_VERSION}-windows-{arch}"
        binary = _install_pinned(
            f"{_CCACHE_RELEASE_URL}/{stem}.zip",
            CCACHE_SHA256[f"windows-{arch}"],
            f"{stem}/ccache.exe",
            runner_temp / stem / "ccache.exe",
        )
    else:
        raise RuntimeError(f"ccache installation is not supported on {sys.platform}")
    append_github_path(str(binary.parent))


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------


def get_ccache_json_stats() -> dict | None:
    """Run ``ccache --print-stats --format=json`` and return parsed dict.

    Returns None when ccache is unavailable or too old (< 4.10).
    """
    ccache_bin = shutil.which("ccache")
    if not ccache_bin:
        return None

    try:
        result = run_captured([ccache_bin, "--print-stats", "--format=json"])
        if result.returncode != 0:
            return None
        return json.loads(result.stdout)
    except (OSError, json.JSONDecodeError):
        return None


def build_summary_markdown(stats: dict) -> str:
    """Build a GitHub-flavoured markdown summary table from ccache JSON stats."""
    direct = int(stats.get("direct_cache_hit", 0))
    preprocessed = int(stats.get("preprocessed_cache_hit", 0))
    misses = int(stats.get("cache_miss", 0))
    hits = direct + preprocessed
    total = hits + misses
    pct = f"{hits / total * 100:.1f}" if total > 0 else "0.0"

    size_kib = int(stats.get("cache_size_kibibyte", 0))
    max_kib = int(stats.get("max_cache_size_kibibyte", 0))
    size_pct = f"{size_kib / max_kib * 100:.1f}" if max_kib else "0.0"
    cleanups = int(stats.get("cleanups_performed", 0))
    errors = sum(
        int(stats.get(k, 0))
        for k in (
            "internal_error",
            "compiler_check_failed",
            "error_hashing_extra_file",
            "missing_cache_file",
            "preprocessor_error",
        )
    )

    rows = [
        ["Cache hits", f"{hits} / {total} ({pct}%)"],
        ["Direct hits", direct],
        ["Preprocessed hits", preprocessed],
        ["Misses", misses],
        ["Cache size", f"{size_kib / 1024:.0f} MiB / {max_kib / 1048576:.1f} GiB ({size_pct}%)"],
        ["Cleanups (LRU evictions)", cleanups],
    ]
    if errors:
        rows.append(["⚠ Errors", errors])
    return f"### CCache Statistics\n\n{md_table(['Metric', 'Value'], rows)}\n"


def get_ccache_verbose_stats() -> str | None:
    """Run ``ccache -s -vv`` and return its output."""
    ccache_bin = shutil.which("ccache")
    if not ccache_bin:
        return None

    try:
        result = run_captured([ccache_bin, "-s", "-vv"])
        if result.returncode != 0:
            return None
        return result.stdout.strip()
    except OSError:
        return None


def get_ccache_compression_stats() -> str | None:
    """Run ``ccache --show-compression`` (compression ratio + space savings)."""
    ccache_bin = shutil.which("ccache")
    if not ccache_bin:
        return None

    try:
        result = run_captured([ccache_bin, "--show-compression"])
        if result.returncode != 0:
            return None
        return result.stdout.strip()
    except OSError:
        return None


def _sccache_output(*args: str) -> str | None:
    """Run ``sccache <args>`` and return its output, or None when sccache is unavailable."""
    sccache_bin = shutil.which("sccache")
    if not sccache_bin:
        return None
    try:
        result = run_captured([sccache_bin, *args])
    except OSError:
        return None
    return result.stdout.strip() if result.returncode == 0 else None


def build_sccache_summary_markdown(report: dict) -> str:
    """Build a markdown summary table from ``sccache --show-stats --stats-format json``."""
    stats = report.get("stats") or {}

    def total(name: str) -> int:
        return sum(int(count) for count in ((stats.get(name) or {}).get("counts") or {}).values())

    hits = total("cache_hits")
    misses = total("cache_misses")
    lookups = hits + misses
    pct = f"{hits / lookups * 100:.1f}" if lookups else "0.0"
    errors = total("cache_errors") + sum(
        int(stats.get(key, 0))
        for key in ("cache_read_errors", "cache_write_errors", "cache_timeouts")
    )
    rows = [
        ["Cache hits", f"{hits} / {lookups} ({pct}%)"],
        ["Misses", misses],
        ["Cache writes", int(stats.get("cache_writes", 0))],
        ["Non-cacheable calls", int(stats.get("requests_not_cacheable", 0))],
        ["Cache location", report.get("cache_location", "unknown")],
    ]
    if errors:
        rows.append(["⚠ Errors", errors])
    return f"### sccache Statistics\n\n{md_table(['Metric', 'Value'], rows)}\n"


def run_sccache_summary(error_log: Path | None) -> int:
    """Write the sccache pilot's statistics, and any server errors, to the job summary."""
    parts: list[str] = []
    try:
        report = json.loads(_sccache_output("--show-stats", "--stats-format", "json") or "null")
    except json.JSONDecodeError:
        report = None
    if isinstance(report, dict):
        parts.append(build_sccache_summary_markdown(report))
    else:
        print("sccache statistics unavailable", file=sys.stderr)

    text = _sccache_output("--show-stats")
    if text:
        parts.append(
            f"<details>\n<summary>sccache --show-stats</summary>\n\n```\n{text}\n```\n\n</details>\n"
        )

    try:
        log = error_log.read_text(encoding="utf-8", errors="replace").strip() if error_log else ""
    except OSError:
        log = ""
    if log:
        tail = "\n".join(log.splitlines()[-50:])
        parts.append(
            f"<details>\n<summary>sccache error log</summary>\n\n```\n{tail}\n```\n\n</details>\n"
        )

    if parts:
        summary = "\n".join(parts)
        print(summary)
        write_step_summary(summary)
    return 0


def run_summary() -> int:
    """Collect ccache stats and write a job summary table."""
    if os.environ.get("QGC_COMPILER_CACHE_TOOL") == "sccache":
        error_log = os.environ.get("SCCACHE_ERROR_LOG")
        return run_sccache_summary(Path(error_log) if error_log else None)

    parts: list[str] = []

    stats = get_ccache_json_stats()
    if stats is not None:
        parts.append(build_summary_markdown(stats))
    else:
        print("ccache JSON stats unavailable (ccache missing or < 4.10)", file=sys.stderr)

    compression = get_ccache_compression_stats()
    if compression:
        parts.append(f"### Compression\n\n```\n{compression}\n```\n")

    verbose = get_ccache_verbose_stats()
    if verbose:
        parts.append(
            f"<details>\n<summary>Verbose stats</summary>\n\n```\n{verbose}\n```\n\n</details>\n"
        )

    if parts:
        summary = "\n".join(parts)
        print(summary)
        write_step_summary(summary)
    return 0


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
            text = json.loads(docker_report.read_text(encoding="utf-8")).get("ccache")
        except (OSError, ValueError, AttributeError):
            return None
        match = _SHOW_STATS_MISSES.search(text) if isinstance(text, str) else None
        return int(match.group(1)) if match else None
    stats = get_ccache_json_stats()
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

    # -- summary -------------------------------------------------------
    sub.add_parser(
        "summary",
        help="Write cache hit/miss summary to GitHub Step Summary (sccache when selected)",
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

    if args.command == "summary":
        return run_summary()

    if args.command == "save-gate":
        run_save_gate(
            args.event_name,
            parse_min_misses(args.min_misses),
            args.docker_report,
            args.moccache_dir,
        )
        return 0

    print(
        "Error: a subcommand is required (install, summary, save-gate)",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
