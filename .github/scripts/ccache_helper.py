#!/usr/bin/env python3
"""
Ccache helper for CI: install, configure, and report.

Subcommands:
    config   Output ccache configuration for GitHub Actions
    install  Download and install a pinned ccache binary (Linux only)
    summary  Write a build cache hit/miss summary to GitHub Step Summary

Examples:
    ccache_helper.py config  --version 4.13.1 --arch x86_64 --conf ccache.conf
    ccache_helper.py install --version 4.13.1 --arch x86_64
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
import subprocess
import sys
import tarfile
import tempfile
import time
from pathlib import Path
from typing import NamedTuple
from urllib.error import HTTPError, URLError
from urllib.request import urlretrieve


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

    DEFAULT_VERSION = "4.13.1"
    DEFAULT_MAX_SIZE = "2G"
    MINISIGN_VERSION = "0.11"
    MINISIGN_KEY = "RWQX7yXbBedVfI4PNx6FLdFXu9GHUFsr28s4BVGxm4BeybtnX3P06saF"

    CCACHE_RELEASE_URL = "https://github.com/ccache/ccache/releases/download"
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
        """Download file with retry logic."""
        for attempt in range(1, self.max_retries + 1):
            try:
                print(f"Downloading {url} (attempt {attempt}/{self.max_retries})")
                urlretrieve(url, dest)
                return True
            except (HTTPError, URLError, OSError) as e:
                print(f"Download failed: {e}", file=sys.stderr)
                if attempt < self.max_retries:
                    print(f"Retrying in {self.retry_delay} seconds...")
                    time.sleep(self.retry_delay)

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
            result = subprocess.run(
                [str(minisign_bin), "-Vm", str(archive), "-P", self.MINISIGN_KEY],
                capture_output=True,
                text=True,
                check=False,
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
            result = subprocess.run(
                ["sudo", "cp", str(ccache_bin), str(dest_bin)],
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                print(f"Error copying ccache: {result.stderr}", file=sys.stderr)
                return False

            result = subprocess.run(
                ["sudo", "chmod", "+x", str(dest_bin)],
                capture_output=True,
                text=True,
                check=False,
            )
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
        ccache_path = shutil.which("ccache")
        if not ccache_path:
            return False

        try:
            result = subprocess.run(
                ["ccache", "--version"],
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                return False

            version_match = re.search(r"[0-9]+\.[0-9]+(\.[0-9]+)?", result.stdout)
            if version_match and version_match.group() == self.version:
                return True
        except OSError:
            pass

        return False

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
            result = subprocess.run(
                ["ccache", "--version"],
                capture_output=True,
                text=True,
                check=False,
            )
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
        result = subprocess.run(
            [ccache_bin, "--print-stats", "--format=json"],
            capture_output=True,
            text=True,
            check=False,
        )
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

    lines = [
        "### CCache Statistics",
        "",
        "| Metric | Value |",
        "|--------|-------|",
        f"| Cache hits | {hits} / {total} ({pct}%) |",
        f"| Direct hits | {direct} |",
        f"| Preprocessed hits | {preprocessed} |",
        f"| Misses | {misses} |",
    ]
    return "\n".join(lines) + "\n"


def write_step_summary(markdown: str) -> bool:
    """Append *markdown* to ``$GITHUB_STEP_SUMMARY``.

    Returns True on success, False when the env var is unset or write fails.
    """
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_path:
        print(markdown)
        return True

    try:
        with open(summary_path, "a") as f:
            f.write(markdown)
        return True
    except OSError as e:
        print(f"Warning: Failed to write step summary: {e}", file=sys.stderr)
        return False


def get_ccache_verbose_stats() -> str | None:
    """Run ``ccache -s -vv`` and return its output."""
    ccache_bin = shutil.which("ccache")
    if not ccache_bin:
        return None

    try:
        result = subprocess.run(
            [ccache_bin, "-s", "-vv"],
            capture_output=True,
            text=True,
            check=False,
        )
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

    verbose = get_ccache_verbose_stats()
    if verbose:
        parts.append(
            "<details>\n<summary>Verbose stats</summary>\n\n"
            f"```\n{verbose}\n```\n\n</details>\n"
        )

    if parts:
        write_step_summary("\n".join(parts))
    return 0


# ---------------------------------------------------------------------------
# GitHub Actions output helper
# ---------------------------------------------------------------------------

def output_github_actions(config: CcacheConfig) -> None:
    """Write outputs for GitHub Actions."""
    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        return

    try:
        with open(github_output, "a") as f:
            f.write(f"version={config.version}\n")
            f.write(f"arch={config.arch}\n")
            f.write(f"max_size={config.max_size}\n")
    except OSError as e:
        print(f"Warning: Failed to write GitHub output: {e}", file=sys.stderr)


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

    print("Error: a subcommand is required (config, install, summary)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
