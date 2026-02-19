#!/usr/bin/env python3
"""
Install and configure ccache with signature verification.

Usage:
    install_ccache.py [--version VERSION] [--arch ARCH] [--config PATH] [--prefix PATH]

Outputs (for GitHub Actions):
    version, arch, max_size
"""

from __future__ import annotations

import argparse
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
from urllib.request import urlopen, urlretrieve


class CcacheConfig(NamedTuple):
    """Configuration values extracted for ccache setup."""

    version: str
    arch: str
    max_size: str


class CcacheInstaller:
    """Handles ccache installation with signature verification."""

    DEFAULT_VERSION = "4.12.2"
    DEFAULT_MAX_SIZE = "2G"
    MINISIGN_VERSION = "0.11"
    MINISIGN_KEY = "RWQX7yXbBedVfI4PNx6FLdFXu9GHUFsr28s4BVGxm4BeybtnX3P06saF"

    CCACHE_RELEASE_URL = "https://github.com/ccache/ccache/releases/download"
    MINISIGN_RELEASE_URL = "https://github.com/jedisct1/minisign/releases/download"

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
        archive_name = f"ccache-{self.version}-linux-{self.arch}.tar.xz"
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
        extract_dir = temp_dir / f"ccache-{self.version}-linux-{self.arch}"

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


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Install and configure ccache with signature verification",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--version",
        default=CcacheInstaller.DEFAULT_VERSION,
        help=f"ccache version (default: {CcacheInstaller.DEFAULT_VERSION})",
    )
    parser.add_argument(
        "--arch",
        choices=["x86_64", "aarch64"],
        help="Architecture (default: auto-detect)",
    )
    parser.add_argument(
        "--config",
        type=Path,
        metavar="PATH",
        help="Path to ccache.conf for max_size",
    )
    parser.add_argument(
        "--prefix",
        type=Path,
        default=None,
        help="Installation prefix (default: $CCACHE_PREFIX or /usr/local)",
    )
    parser.add_argument(
        "--install",
        action="store_true",
        help="Install ccache binary (Linux only)",
    )
    parser.add_argument(
        "--output-only",
        action="store_true",
        help="Only output config, don't install",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    if not CcacheInstaller.validate_version(args.version):
        print(f"Error: Invalid ccache version format: {args.version}", file=sys.stderr)
        return 1

    installer = CcacheInstaller(
        version=args.version,
        arch=args.arch,
        config_path=args.config,
        prefix=args.prefix,
    )

    config = installer.get_config()

    print("ccache configuration:")
    print(f"  Version: {config.version}")
    print(f"  Arch: {config.arch}")
    print(f"  Max Size: {config.max_size}")

    output_github_actions(config)

    if args.install and not args.output_only:
        if not installer.run_install():
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
