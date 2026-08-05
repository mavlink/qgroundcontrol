#!/usr/bin/env python3
"""
Ccache helper for CI: install, configure, and report.

Subcommands:
    config   Output ccache configuration for GitHub Actions
    install  Download and install a pinned ccache binary (Linux only)
    summary  Write a build cache hit/miss summary to GitHub Step Summary

Examples:
    ccache_helper.py config  --version 4.13.6 --arch x86_64 --conf ccache.conf
    ccache_helper.py install --version 4.13.6 --arch x86_64
    ccache_helper.py summary
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import sys
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, NamedTuple

if TYPE_CHECKING:
    from collections.abc import Callable

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import (
    append_github_env,
    append_github_path,
    write_github_output,
    write_step_summary,
)
from common.io import require_tar_data_filter
from common.markdown import md_table
from common.net import download_with_retry
from common.proc import run_captured
from common.tool_version import probe_version

_CCACHE_RELEASE_URL = "https://github.com/ccache/ccache/releases/download"


def _extract_zip(archive: Path, dest: Path) -> None:
    with zipfile.ZipFile(archive) as zf:
        zf.extractall(dest)


def _extract_tar_gz(archive: Path, dest: Path) -> None:
    require_tar_data_filter()
    with tarfile.open(archive, "r:gz") as tar:
        tar.extractall(dest, filter="data")


def _install_release_binary(
    url: str,
    sha256: str,
    archive_name: str,
    extract: Callable[[Path, Path], None],
    locate_source: Callable[[Path], Path],
    dest: Path,
) -> Path:
    """Download a SHA256-verified release archive, extract it, and copy the binary to ``dest``."""
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        archive_path = temp_path / archive_name
        download_with_retry(url, archive_path)
        actual = hashlib.sha256(archive_path.read_bytes()).hexdigest()
        if actual != sha256:
            raise RuntimeError(f"SHA256 mismatch: {actual} != {sha256}")
        extract(archive_path, temp_path)
        source = locate_source(temp_path)
        if not source.exists():
            raise FileNotFoundError(f"ccache binary not found in archive: {source}")
        shutil.copy2(source, dest)
    return dest


@dataclass(frozen=True)
class CcacheRelease:
    """A pinned ccache release: version coupled to its per-platform digests."""

    version: str
    windows_sha256: dict[str, str]  # keyed by arch (x86_64, aarch64)
    macos_sha256: str  # universal (x86_64 + arm64) tarball, no arch suffix


# Digests are bound to .version; test_ccache_version_drift.py enforces sync with build-config.json.
PINNED_RELEASE = CcacheRelease(
    version="4.13.6",
    windows_sha256={
        "x86_64": "3d7cebb05850ad704e197b3f1d3f0f924ab6c9fdfc561578e146184fe9d89380",
        "aarch64": "bec01846b06d6d87bf35eda50544d7c8bf9b9a4859f218417a7081aa45d7fd47",
    },
    macos_sha256="0274210ec9c9936ed5711d59b0de3167a51216a588ddde35f6bc828f366fe6d9",
)

PINNED_BINARY_VERSION = PINNED_RELEASE.version
WINDOWS_BINARY_SHA256 = PINNED_RELEASE.windows_sha256
MACOS_BINARY_SHA256 = PINNED_RELEASE.macos_sha256

# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------


class CcacheConfig(NamedTuple):
    """Configuration values extracted for ccache setup."""

    version: str
    arch: str
    max_size: str


# ---------------------------------------------------------------------------
# Installer
# ---------------------------------------------------------------------------


class CcacheInstaller:
    """Handles ccache installation with signature verification."""

    DEFAULT_VERSION = PINNED_BINARY_VERSION
    DEFAULT_MAX_SIZE = "2G"
    MINISIGN_VERSION = "0.11"
    MINISIGN_KEY = "RWQX7yXbBedVfI4PNx6FLdFXu9GHUFsr28s4BVGxm4BeybtnX3P06saF"

    CCACHE_RELEASE_URL = _CCACHE_RELEASE_URL
    MINISIGN_RELEASE_URL = "https://github.com/jedisct1/minisign/releases/download"
    MINISIGN_ARCHIVE_SHA256 = "f0a0954413df8531befed169e447a66da6868d79052ed7e892e50a4291af7ae0"

    def __init__(
        self,
        version: str = DEFAULT_VERSION,
        arch: str | None = None,
        config_path: Path | None = None,
        prefix: Path | None = None,
        max_retries: int = 3,
        retry_delay: float = 5.0,
    ) -> None:
        self.version = version
        self.arch = arch or self.detect_arch()
        self.config_path = config_path
        self.prefix = prefix or self._default_prefix()
        self.max_retries = max_retries
        self.retry_delay = retry_delay
        self.max_size = self.DEFAULT_MAX_SIZE

        if config_path:
            self.max_size = self.read_max_size(config_path)

    @staticmethod
    def _default_prefix() -> Path:
        """Get default installation prefix from env or standard location."""
        env_prefix = os.environ.get("CCACHE_PREFIX")
        if env_prefix:
            return Path(env_prefix)
        return Path("/usr/local")

    @staticmethod
    def validate_version(version: str) -> bool:
        """Validate version format matches X.Y or X.Y.Z pattern."""
        pattern = r"^[0-9]+\.[0-9]+(\.[0-9]+)?$"
        return bool(re.match(pattern, version))

    @staticmethod
    def detect_arch() -> str:
        """Auto-detect CPU architecture, normalizing to ccache naming."""
        machine = platform.machine().lower()
        arch_map = {
            "x86_64": "x86_64",
            "amd64": "x86_64",
            "aarch64": "aarch64",
            "arm64": "aarch64",
        }
        return arch_map.get(machine, "x86_64")

    def read_max_size(self, config_path: Path) -> str:
        """Extract max_size value from ccache.conf file."""
        if not config_path.exists():
            print(
                f"Warning: ccache config not found at {config_path}, using default max_size",
                file=sys.stderr,
            )
            return self.DEFAULT_MAX_SIZE

        pattern = re.compile(r"^max_size\s*=\s*(.+)$")
        try:
            with config_path.open() as f:
                for line in f:
                    match = pattern.match(line.strip())
                    if match:
                        return match.group(1).strip()
        except OSError as e:
            print(f"Warning: Failed to read config: {e}", file=sys.stderr)

        return self.DEFAULT_MAX_SIZE

    def download_with_retry(self, url: str, dest: Path) -> bool:
        """Download file with retry logic. Returns False on exhaustion."""
        try:
            download_with_retry(url, dest, attempts=self.max_retries, delay=self.retry_delay)
            return True
        except RuntimeError as e:
            print(f"Download failed: {e}", file=sys.stderr)
            return False

    def download_with_verify(self, temp_dir: Path) -> Path | None:
        """Download ccache archive and verify its signature."""
        archive_name = f"ccache-{self.version}-linux-{self.arch}-glibc.tar.xz"
        archive_path = temp_dir / archive_name
        sig_path = temp_dir / f"{archive_name}.minisig"

        ccache_url = f"{self.CCACHE_RELEASE_URL}/v{self.version}/{archive_name}"
        sig_url = f"{ccache_url}.minisig"

        if not self.download_with_retry(ccache_url, archive_path):
            print("Error: Failed to download ccache archive", file=sys.stderr)
            return None

        if not self.download_with_retry(sig_url, sig_path):
            print("Error: Failed to download signature file", file=sys.stderr)
            return None

        minisign_bin = self._setup_minisign(temp_dir)
        if not minisign_bin:
            print("Error: Failed to setup minisign", file=sys.stderr)
            return None

        if not self.verify_signature(archive_path, sig_path, minisign_bin):
            print("Error: Signature verification failed", file=sys.stderr)
            return None

        return archive_path

    def _setup_minisign(self, temp_dir: Path) -> Path | None:
        """Download and extract minisign binary."""
        minisign_archive = f"minisign-{self.MINISIGN_VERSION}-linux.tar.gz"
        minisign_url = f"{self.MINISIGN_RELEASE_URL}/{self.MINISIGN_VERSION}/{minisign_archive}"
        minisign_path = temp_dir / minisign_archive

        if not self.download_with_retry(minisign_url, minisign_path):
            return None

        actual_hash = hashlib.sha256(minisign_path.read_bytes()).hexdigest()
        if actual_hash != self.MINISIGN_ARCHIVE_SHA256:
            print(
                f"Error: minisign archive hash mismatch: {actual_hash} != {self.MINISIGN_ARCHIVE_SHA256}",
                file=sys.stderr,
            )
            return None

        try:
            require_tar_data_filter()
            with tarfile.open(minisign_path, "r:gz") as tar:
                tar.extractall(temp_dir, filter="data")
        except tarfile.TarError as e:
            print(f"Error extracting minisign: {e}", file=sys.stderr)
            return None

        minisign_bin = temp_dir / "minisign-linux" / self.arch / "minisign"
        if not minisign_bin.exists():
            print(f"Error: minisign binary not found at {minisign_bin}", file=sys.stderr)
            return None

        minisign_bin.chmod(0o755)
        return minisign_bin

    def verify_signature(self, archive: Path, sig_file: Path, minisign_bin: Path) -> bool:
        """Verify archive signature using minisign."""
        try:
            result = run_captured(
                [
                    str(minisign_bin),
                    "-Vm",
                    str(archive),
                    "-x",
                    str(sig_file),
                    "-P",
                    self.MINISIGN_KEY,
                ],
            )
            if result.returncode != 0:
                print(f"Signature verification failed: {result.stderr}", file=sys.stderr)
                return False
            print("Signature verified successfully")
            return True
        except OSError as e:
            print(f"Error running minisign: {e}", file=sys.stderr)
            return False

    def install(self, archive: Path) -> bool:
        """Extract and install ccache binary."""
        temp_dir = archive.parent
        extract_dir = temp_dir / f"ccache-{self.version}-linux-{self.arch}-glibc"

        try:
            require_tar_data_filter()
            with tarfile.open(archive, "r:xz") as tar:
                tar.extractall(temp_dir, filter="data")
        except tarfile.TarError as e:
            print(f"Error extracting ccache: {e}", file=sys.stderr)
            return False

        ccache_bin = extract_dir / "ccache"
        if not ccache_bin.exists():
            print(f"Error: ccache binary not found at {ccache_bin}", file=sys.stderr)
            return False

        dest_bin = self.prefix / "bin" / "ccache"

        try:
            result = run_captured(["sudo", "cp", str(ccache_bin), str(dest_bin)])
            if result.returncode != 0:
                print(f"Error copying ccache: {result.stderr}", file=sys.stderr)
                return False

            result = run_captured(["sudo", "chmod", "+x", str(dest_bin)])
            if result.returncode != 0:
                print(f"Error setting permissions: {result.stderr}", file=sys.stderr)
                return False

        except OSError as e:
            print(f"Error installing ccache: {e}", file=sys.stderr)
            return False

        print(f"ccache {self.version} installed successfully to {dest_bin}")
        return True

    def is_installed(self) -> bool:
        """Check if requested ccache version is already installed."""
        installed = probe_version("ccache")
        if installed is None:
            return False
        expected = tuple(int(n) for n in self.version.split("."))
        # probe_version drops a missing patch component; compare on the shorter prefix.
        compare_len = min(len(installed), len(expected))
        return installed[:compare_len] == expected[:compare_len]

    def get_config(self) -> CcacheConfig:
        """Return current configuration as named tuple."""
        return CcacheConfig(
            version=self.version,
            arch=self.arch,
            max_size=self.max_size,
        )

    def run_install(self) -> bool:
        """Execute full installation workflow."""
        if platform.system() != "Linux":
            print("Warning: Binary installation only supported on Linux", file=sys.stderr)
            return False

        if self.is_installed():
            print(f"ccache {self.version} already installed")
            return True

        print(f"Installing ccache {self.version}...")

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            archive = self.download_with_verify(temp_path)
            if not archive:
                return False

            if not self.install(archive):
                return False

        self._print_version()
        return True

    def _print_version(self) -> None:
        """Print installed ccache version."""
        try:
            result = run_captured(["ccache", "--version"])
            if result.returncode == 0:
                print(result.stdout.strip())
        except OSError:
            pass


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


def run_summary() -> int:
    """Collect ccache stats and write a job summary table."""
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
        write_step_summary("\n".join(parts))
    return 0


# ---------------------------------------------------------------------------
# GitHub Actions output helper
# ---------------------------------------------------------------------------


def output_github_actions(config: CcacheConfig) -> None:
    """Write outputs for GitHub Actions."""
    write_github_output(
        {
            "version": config.version,
            "arch": config.arch,
            "max_size": config.max_size,
        }
    )


def determine_cache_scope(event_name: str, ref_name: str, pr_number: str = "") -> str:
    """Return the normalized cache scope used by CI."""
    scope = "shared"
    if event_name == "pull_request":
        scope = f"pr-{pr_number or 'unknown'}"
    elif event_name == "workflow_dispatch":
        scope = f"manual-{ref_name}"
    elif event_name == "push":
        if ref_name != "master":
            scope = f"branch-{ref_name}"
    else:
        scope = f"{event_name}-{ref_name}"
    return scope.replace("/", "-")


def resolve_windows_binary_config(host: str, target: str) -> dict[str, str]:
    """Return Windows ccache binary arch and checksum for the requested target."""
    arch = "aarch64" if host == "windows_arm64" else "x86_64"
    if target != "android" and host != "windows_arm64":
        arch = "x86_64"
    return {"arch": arch, "sha256": WINDOWS_BINARY_SHA256[arch]}


def _assert_pinned_version(version: str, platform_label: str) -> None:
    """Fail fast if the requested version doesn't match the pinned SHA256 dicts.

    The SHA256 constants above are bound to PINNED_BINARY_VERSION; downloading
    any other version would corrupt verification. Raising here turns a runtime
    checksum mismatch into a clear configuration error.
    """
    if version != PINNED_BINARY_VERSION:
        raise RuntimeError(
            f"ccache {platform_label} installer pinned to {PINNED_BINARY_VERSION} but "
            f"got {version}. Bump PINNED_BINARY_VERSION (and the matching SHA256 "
            f"constants) in ccache_helper.py, or update .github/build-config.json "
            f"to match."
        )


def install_windows_binary(version: str, arch: str, sha256: str, runner_temp: Path) -> Path:
    """Download and install the ccache Windows binary under ``runner_temp``."""
    _assert_pinned_version(version, "Windows")
    install_dir = runner_temp / f"ccache-{version}-windows-{arch}"
    install_dir.mkdir(parents=True, exist_ok=True)
    _install_release_binary(
        f"{_CCACHE_RELEASE_URL}/v{version}/ccache-{version}-windows-{arch}.zip",
        sha256,
        "ccache.zip",
        _extract_zip,
        lambda root: root / f"ccache-{version}-windows-{arch}" / "ccache.exe",
        install_dir / "ccache.exe",
    )
    return install_dir


def install_macos_binary(version: str, sha256: str, prefix: Path) -> Path:
    """Download and install the ccache macOS universal binary under ``prefix``/bin.

    SHA256-verified GitHub release tarball (not the Linux minisign path), so the
    macOS runner needs no minisign binary, and all three platforms install the
    version declared in ``.github/build-config.json`` (vs unpinned brew).
    """
    _assert_pinned_version(version, "macOS")
    bin_dir = prefix / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    dest = _install_release_binary(
        f"{_CCACHE_RELEASE_URL}/v{version}/ccache-{version}-darwin.tar.gz",
        sha256,
        "ccache.tar.gz",
        _extract_tar_gz,
        lambda root: root / f"ccache-{version}-darwin" / "ccache",
        bin_dir / "ccache",
    )
    dest.chmod(0o755)
    return dest


def add_windows_binary_to_path(version: str, arch: str, runner_temp: Path) -> Path:
    """Add the installed Windows ccache binary directory to PATH."""
    install_dir = runner_temp / f"ccache-{version}-windows-{arch}"
    binary = install_dir / "ccache.exe"
    if not binary.exists():
        raise FileNotFoundError(f"ccache.exe not found at {binary}")
    append_github_path(str(install_dir))
    result = run_captured([str(binary), "--version"])
    if result.stdout:
        print(result.stdout.strip())
    return install_dir


def configure_ccache_environment(workspace: Path) -> Path:
    """Configure standard ccache environment variables for GitHub Actions."""
    workspace_path = Path(str(workspace).replace("\\", "/"))
    ccache_dir = workspace_path / ".ccache"
    ccache_dir.mkdir(parents=True, exist_ok=True)
    append_github_env(
        {
            "CCACHE_DIR": ccache_dir.as_posix(),
            "CCACHE_BASEDIR": workspace_path.as_posix(),
            "CCACHE_CONFIGPATH": (workspace_path / "tools" / "configs" / "ccache.conf").as_posix(),
            "CCACHE_MAXSIZE": "2G",
            "CCACHE_NOFILECLONE": "1",
        }
    )
    return ccache_dir


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

TARGET_ARCH_MAP: dict[str, str] = {
    "linux_gcc_arm64": "aarch64",
}


def resolve_arch(args: argparse.Namespace) -> str | None:
    """Return architecture from --arch, --target, or None for auto-detect."""
    if getattr(args, "arch", None):
        return args.arch
    target = getattr(args, "target", None)
    if target:
        return TARGET_ARCH_MAP.get(target)
    return None


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Ccache helper for CI: install, configure, and report",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    # -- config --------------------------------------------------------
    cfg = sub.add_parser("config", help="Output ccache configuration for GitHub Actions")
    cfg.add_argument(
        "--version",
        default=CcacheInstaller.DEFAULT_VERSION,
        help=f"ccache version (default: {CcacheInstaller.DEFAULT_VERSION})",
    )
    cfg_arch = cfg.add_mutually_exclusive_group()
    cfg_arch.add_argument(
        "--arch",
        choices=["x86_64", "aarch64"],
        help="Architecture (default: auto-detect)",
    )
    cfg_arch.add_argument(
        "--target",
        metavar="CI_TARGET",
        help="CI build target — resolved to arch (e.g. linux_gcc_arm64 → aarch64)",
    )
    cfg.add_argument(
        "--conf",
        type=Path,
        metavar="PATH",
        help="Path to ccache.conf for max_size",
    )

    # -- install -------------------------------------------------------
    inst = sub.add_parser("install", help="Install a pinned ccache binary (Linux)")
    inst.add_argument(
        "--version",
        default=CcacheInstaller.DEFAULT_VERSION,
        help=f"ccache version (default: {CcacheInstaller.DEFAULT_VERSION})",
    )
    inst_arch = inst.add_mutually_exclusive_group()
    inst_arch.add_argument(
        "--arch",
        choices=["x86_64", "aarch64"],
        help="Architecture (default: auto-detect)",
    )
    inst_arch.add_argument(
        "--target",
        metavar="CI_TARGET",
        help="CI build target — resolved to arch (e.g. linux_gcc_arm64 → aarch64)",
    )
    inst.add_argument(
        "--prefix",
        type=Path,
        default=None,
        help="Installation prefix (default: $CCACHE_PREFIX or /usr/local)",
    )

    # -- summary -------------------------------------------------------
    sub.add_parser("summary", help="Write cache hit/miss summary to GitHub Step Summary")

    # -- scope ---------------------------------------------------------
    scope = sub.add_parser("scope", help="Compute the workflow cache scope")
    scope.add_argument("--event-name", required=True, help="GitHub event name")
    scope.add_argument("--ref-name", required=True, help="Git ref name")
    scope.add_argument("--pr-number", default="", help="Pull request number")

    # -- windows-config ------------------------------------------------
    windows_cfg = sub.add_parser("windows-config", help="Resolve Windows ccache binary metadata")
    windows_cfg.add_argument("--host", required=True, help="Workflow host input")
    windows_cfg.add_argument("--target", required=True, help="Workflow target input")

    install_win = sub.add_parser(
        "install-windows", help="Download and install Windows ccache binary"
    )
    install_win.add_argument("--version", required=True)
    install_win.add_argument("--arch", required=True, choices=["x86_64", "aarch64"])
    install_win.add_argument("--sha256", required=True)
    install_win.add_argument("--runner-temp", type=Path, required=True)

    add_win = sub.add_parser("add-windows-path", help="Add Windows ccache install dir to PATH")
    add_win.add_argument("--version", required=True)
    add_win.add_argument("--arch", required=True, choices=["x86_64", "aarch64"])
    add_win.add_argument("--runner-temp", type=Path, required=True)

    sub.add_parser("macos-config", help="Emit pinned macOS ccache SHA256 for the cache step")

    install_mac = sub.add_parser("install-macos", help="Download and install macOS ccache binary")
    install_mac.add_argument("--version", required=True)
    install_mac.add_argument("--sha256", required=True)
    install_mac.add_argument("--prefix", type=Path, default=Path("/usr/local"))

    env_cfg = sub.add_parser("configure-env", help="Configure ccache GitHub environment variables")
    env_cfg.add_argument("--workspace", type=Path, required=True)

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    if args.command == "config":
        if not CcacheInstaller.validate_version(args.version):
            print(f"Error: Invalid ccache version format: {args.version}", file=sys.stderr)
            return 1

        installer = CcacheInstaller(
            version=args.version,
            arch=resolve_arch(args),
            config_path=args.conf,
        )
        config = installer.get_config()

        print("ccache configuration:")
        print(f"  Version: {config.version}")
        print(f"  Arch: {config.arch}")
        print(f"  Max Size: {config.max_size}")

        output_github_actions(config)
        return 0

    if args.command == "install":
        if not CcacheInstaller.validate_version(args.version):
            print(f"Error: Invalid ccache version format: {args.version}", file=sys.stderr)
            return 1

        installer = CcacheInstaller(
            version=args.version,
            arch=resolve_arch(args),
            prefix=args.prefix,
        )

        config = installer.get_config()
        print("ccache configuration:")
        print(f"  Version: {config.version}")
        print(f"  Arch: {config.arch}")
        print(f"  Max Size: {config.max_size}")

        if not installer.run_install():
            return 1
        return 0

    if args.command == "summary":
        return run_summary()

    if args.command == "scope":
        scope = determine_cache_scope(args.event_name, args.ref_name, args.pr_number)
        print(scope)
        write_github_output({"scope": scope})
        return 0

    if args.command == "windows-config":
        values = resolve_windows_binary_config(args.host, args.target)
        print(f"arch={values['arch']}")
        print(f"sha256={values['sha256']}")
        write_github_output(values)
        return 0

    if args.command == "install-windows":
        install_dir = install_windows_binary(args.version, args.arch, args.sha256, args.runner_temp)
        print(install_dir)
        write_github_output({"install_dir": str(install_dir)})
        return 0

    if args.command == "add-windows-path":
        install_dir = add_windows_binary_to_path(args.version, args.arch, args.runner_temp)
        print(install_dir)
        return 0

    if args.command == "macos-config":
        print(f"sha256={MACOS_BINARY_SHA256}")
        write_github_output({"sha256": MACOS_BINARY_SHA256})
        return 0

    if args.command == "install-macos":
        dest = install_macos_binary(args.version, args.sha256, args.prefix)
        print(dest)
        return 0

    if args.command == "configure-env":
        ccache_dir = configure_ccache_environment(args.workspace)
        print(ccache_dir)
        return 0

    print(
        "Error: a subcommand is required (config, install, summary, scope, "
        "windows-config, install-windows, add-windows-path, macos-config, "
        "install-macos, configure-env)",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
