#!/usr/bin/env python3
"""
Create GStreamer archive and optionally upload to S3.

Usage:
    gstreamer_archive.py --platform linux --arch x86_64 --version 1.24.0 [--simulator] [--upload-s3]

Environment variables for S3 upload:
    AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, AWS_DEFAULT_REGION
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class ArchiveResult:
    name: str
    path: Path
    extension: str


def run_command(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    """Run command with improved error output."""
    result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    if result.returncode != 0:
        print(f"Command failed: {' '.join(cmd)}", file=sys.stderr)
        if result.stdout:
            print(f"stdout:\n{result.stdout}", file=sys.stderr)
        if result.stderr:
            print(f"stderr:\n{result.stderr}", file=sys.stderr)
        raise subprocess.CalledProcessError(result.returncode, cmd, result.stdout, result.stderr)
    return result


class GStreamerArchiver:
    VALID_PLATFORMS = {"linux", "macos", "windows", "android", "ios"}

    def __init__(
        self,
        platform: str,
        arch: str,
        version: str,
        prefix: Path | None = None,
        simulator: bool = False,
        output_dir: Path | None = None,
    ) -> None:
        if platform not in self.VALID_PLATFORMS:
            raise ValueError(f"Unknown platform: {platform}")

        self.platform = platform
        self.arch = arch
        self.version = version
        self.simulator = simulator
        self.output_dir = output_dir or Path(os.environ.get("RUNNER_TEMP", "."))
        self.prefix = prefix or self._default_prefix()
        self._archive_result: ArchiveResult | None = None

    def _default_prefix(self) -> Path:
        """Get default GStreamer prefix from env or platform-specific location."""
        env_prefix = os.environ.get("GSTREAMER_PREFIX")
        if env_prefix:
            return Path(env_prefix)

        if self.platform == "windows":
            normalized = "x64" if self.arch == "x86_64" else self.arch
            return Path(os.environ.get("GSTREAMER_WIN_PREFIX", f"C:/gstreamer-{normalized}"))
        return Path(os.environ.get("RUNNER_TEMP", "/tmp")) / "gstreamer"

    def _normalize_arch(self) -> str:
        if self.platform == "windows" and self.arch == "x86_64":
            return "x64"
        return self.arch

    def get_archive_name(self) -> str:
        arch = self._normalize_arch()

        platform_names = {
            "linux": f"gstreamer-1.0-linux-{arch}-{self.version}",
            "macos": f"gstreamer-1.0-macos-{arch}-{self.version}",
            "windows": f"gstreamer-1.0-msvc-{arch}-{self.version}",
            "android": f"gstreamer-1.0-android-{arch}-{self.version}",
        }

        if self.platform == "ios":
            suffix = f"{arch}-simulator" if self.simulator else arch
            return f"gstreamer-1.0-ios-{suffix}-{self.version}"

        return platform_names[self.platform]

    def _get_extension(self) -> str:
        return "zip" if self.platform == "windows" else "tar.xz"

    def create_archive(self) -> Path:
        name = self.get_archive_name()
        ext = self._get_extension()
        archive_path = self.output_dir / f"{name}.{ext}"

        print(f"Creating archive: {name}")
        print(f"  Platform: {self.platform}")
        print(f"  Arch: {self._normalize_arch()}")
        print(f"  Version: {self.version}")
        print(f"  Prefix: {self.prefix}")

        self.output_dir.mkdir(parents=True, exist_ok=True)

        if self.platform == "windows":
            self._create_zip_archive(archive_path)
        else:
            self._create_tar_archive(archive_path)

        print(f"Created archive: {archive_path}")
        size = archive_path.stat().st_size
        print(f"  Size: {size / (1024 * 1024):.1f} MB")

        self._archive_result = ArchiveResult(name=name, path=archive_path, extension=ext)
        return archive_path

    def _create_zip_archive(self, archive_path: Path) -> None:
        cmd = [
            "powershell",
            "-Command",
            f"Compress-Archive -Path '{self.prefix}' -DestinationPath '{archive_path}' -CompressionLevel Optimal",
        ]
        run_command(cmd)

    def _create_tar_archive(self, archive_path: Path) -> None:
        parent_dir = self.prefix.parent
        folder_name = self.prefix.name
        cmd = ["tar", "-C", str(parent_dir), "-cJf", str(archive_path), folder_name]
        run_command(cmd)

    def upload_to_s3(
        self,
        bucket: str,
        key_id: str | None = None,
        secret: str | None = None,
        region: str | None = None,
    ) -> bool:
        key_id = key_id or os.environ.get("AWS_ACCESS_KEY_ID")
        secret = secret or os.environ.get("AWS_SECRET_ACCESS_KEY")
        region = region or os.environ.get("AWS_DEFAULT_REGION")

        if not key_id:
            print("Warning: AWS credentials not set, skipping S3 upload", file=sys.stderr)
            return False

        if not self._archive_result:
            raise RuntimeError("Must call create_archive() before upload_to_s3()")

        self._ensure_aws_cli()

        s3_path = f"s3://{bucket}/dependencies/gstreamer/{self.platform}/{self.version}"
        full_name = f"{self._archive_result.name}.{self._archive_result.extension}"

        env = os.environ.copy()
        if key_id:
            env["AWS_ACCESS_KEY_ID"] = key_id
        if secret:
            env["AWS_SECRET_ACCESS_KEY"] = secret
        if region:
            env["AWS_DEFAULT_REGION"] = region

        cmd = [
            "aws",
            "s3",
            "cp",
            str(self._archive_result.path),
            f"{s3_path}/{full_name}",
            "--acl",
            "public-read",
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, env=env)
        if result.returncode != 0:
            print(f"S3 upload failed: {result.stderr}", file=sys.stderr)
            raise subprocess.CalledProcessError(result.returncode, cmd)
        print(f"Uploaded to: {s3_path}/{full_name}")
        return True

    def _ensure_aws_cli(self) -> None:
        """Ensure AWS CLI is available, installing if necessary."""
        if shutil.which("aws"):
            return

        runner_os = os.environ.get("RUNNER_OS", "")
        print(f"AWS CLI not found, attempting installation for {runner_os or 'local'}...")

        if runner_os == "Windows":
            self._install_aws_cli_windows()
        elif runner_os == "Linux":
            self._install_aws_cli_linux()
        elif runner_os == "macOS":
            self._install_aws_cli_macos()
        else:
            raise RuntimeError(
                "AWS CLI not found. Install manually or set RUNNER_OS environment variable."
            )

    def _install_aws_cli_windows(self) -> None:
        """Install AWS CLI on Windows via MSI."""
        installer_path = self.output_dir / "AWSCLIV2.msi"
        run_command(["curl", "-o", str(installer_path), "https://awscli.amazonaws.com/AWSCLIV2.msi"])
        run_command(["msiexec.exe", "/i", str(installer_path), "/quiet"])
        aws_path = "/c/Program Files/Amazon/AWSCLIV2"
        os.environ["PATH"] = f"{aws_path}:{os.environ.get('PATH', '')}"

    def _install_aws_cli_linux(self) -> None:
        """Install AWS CLI on Linux via zip installer."""
        installer_zip = self.output_dir / "awscliv2.zip"
        installer_dir = self.output_dir / "aws"
        run_command(["curl", "-o", str(installer_zip), "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip"])
        run_command(["unzip", "-q", str(installer_zip), "-d", str(self.output_dir)])
        run_command(["sudo", str(installer_dir / "install")])

    def _install_aws_cli_macos(self) -> None:
        """Install AWS CLI on macOS via pkg installer."""
        installer_pkg = self.output_dir / "AWSCLIV2.pkg"
        run_command(["curl", "-o", str(installer_pkg), "https://awscli.amazonaws.com/AWSCLIV2.pkg"])
        run_command(["sudo", "installer", "-pkg", str(installer_pkg), "-target", "/"])

    def write_github_output(self) -> None:
        github_output = os.environ.get("GITHUB_OUTPUT")
        if not github_output or not self._archive_result:
            return

        with open(github_output, "a") as f:
            f.write(f"name={self._archive_result.name}\n")
            f.write(f"path={self._archive_result.path}\n")
            f.write(f"ext={self._archive_result.extension}\n")

    def write_github_summary(self, uploaded: bool = False) -> None:
        summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
        if not summary_path or not self._archive_result:
            return

        arch_display = self._normalize_arch()
        if self.platform == "ios" and self.simulator:
            arch_display = f"{arch_display}-simulator"

        platform_title = self.platform.capitalize()
        full_name = f"{self._archive_result.name}.{self._archive_result.extension}"

        lines = [
            f"## GStreamer {platform_title} Build Complete",
            "",
            "| Property | Value |",
            "|----------|-------|",
            f"| Version | {self.version} |",
            f"| Architecture | {arch_display} |",
            f"| Archive | {full_name} |",
        ]

        if uploaded:
            lines.append(
                f"| S3 Path | dependencies/gstreamer/{self.platform}/{self.version}/ |"
            )

        with open(summary_path, "a") as f:
            f.write("\n".join(lines) + "\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create GStreamer archive and optionally upload to S3",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Environment variables for S3 upload:
  AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, AWS_DEFAULT_REGION
""",
    )
    parser.add_argument(
        "--platform",
        required=True,
        choices=["linux", "macos", "windows", "android", "ios"],
        help="Target platform",
    )
    parser.add_argument(
        "--arch",
        required=True,
        help="Architecture (x86_64, aarch64, arm64, etc.)",
    )
    parser.add_argument(
        "--version",
        required=True,
        help="GStreamer version",
    )
    parser.add_argument(
        "--prefix",
        type=Path,
        default=None,
        help="Installation prefix (default: auto-detect)",
    )
    parser.add_argument(
        "--simulator",
        action="store_true",
        help="iOS simulator build (default: false)",
    )
    parser.add_argument(
        "--upload-s3",
        action="store_true",
        help="Upload to S3 (requires AWS credentials)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Output directory for archive (default: $RUNNER_TEMP or .)",
    )
    parser.add_argument(
        "--bucket",
        default="qgroundcontrol",
        help="S3 bucket name (default: qgroundcontrol)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        archiver = GStreamerArchiver(
            platform=args.platform,
            arch=args.arch,
            version=args.version,
            prefix=args.prefix,
            simulator=args.simulator,
            output_dir=args.output_dir,
        )

        archiver.create_archive()
        archiver.write_github_output()

        uploaded = False
        if args.upload_s3:
            uploaded = archiver.upload_to_s3(bucket=args.bucket)

        archiver.write_github_summary(uploaded=uploaded)
        return 0

    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {e}", file=sys.stderr)
        return 1
    except RuntimeError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
