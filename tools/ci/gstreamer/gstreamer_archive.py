#!/usr/bin/env python3
"""
Create GStreamer archive and optionally upload to S3.

Usage:
    gstreamer_archive.py --platform linux --arch x86_64 --prefix DIR [--version 1.24.0] [--simulator] [--upload-s3]

--upload-s3 needs the AWS CLI on PATH and credentials in the environment (AWS_ACCESS_KEY_ID,
AWS_SECRET_ACCESS_KEY, AWS_DEFAULT_REGION); it fails without them.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import ClassVar

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.aws import upload_public_file
from common.build_config import get_gstreamer_version
from common.gh_actions import gh_error, write_github_output, write_step_summary
from common.markdown import code_span, summary_section
from common.proc import MissingToolError, require_tools, run_captured

S3_BUCKET = "qgroundcontrol"


@dataclass
class ArchiveResult:
    name: str
    path: Path
    extension: str


def require_s3_upload_tools() -> None:
    """Fail before archiving when an S3 upload cannot succeed."""
    require_tools("aws", hint="--upload-s3 requires the AWS CLI")
    if not os.environ.get("AWS_ACCESS_KEY_ID"):
        raise RuntimeError("--upload-s3 requires AWS credentials; AWS_ACCESS_KEY_ID is unset")


def run_command(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    """Run *cmd* via common.proc.run_captured, printing stdout/stderr on failure.

    The verbose error logging exists because tar failures here are
    debugged from CI logs — a plain CalledProcessError traceback isn't enough.
    """
    result = run_captured(cmd, **kwargs)
    if result.returncode != 0:
        print(f"Command failed: {' '.join(cmd)}", file=sys.stderr)
        if result.stdout:
            print(f"stdout:\n{result.stdout}", file=sys.stderr)
        if result.stderr:
            print(f"stderr:\n{result.stderr}", file=sys.stderr)
        raise subprocess.CalledProcessError(result.returncode, cmd, result.stdout, result.stderr)
    return result


class GStreamerArchiver:
    VALID_PLATFORMS: ClassVar[set[str]] = {"linux", "macos", "windows", "android", "ios"}

    def __init__(
        self,
        platform: str,
        arch: str,
        version: str,
        prefix: Path,
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
        self.prefix = prefix
        self._archive_result: ArchiveResult | None = None

    def get_archive_name(self) -> str:
        arch = self.arch
        platform_names = {
            "linux": f"gstreamer-1.0-linux-{arch}-{self.version}",
            "macos": f"gstreamer-1.0-macos-{arch}-{self.version}",
            "windows": f"gstreamer-1.0-msvc-{arch}-{self.version}",
            "android": f"gstreamer-1.0-android-{arch}-{self.version}",
        }

        if self.platform == "ios":
            suffix = f"{arch}-simulator" if self.simulator else arch
            return f"gstreamer-1.0-ios-{suffix}-{self.version}"

        if self.platform not in platform_names:
            raise ValueError(f"No archive name mapping for platform: {self.platform}")
        return platform_names[self.platform]

    def _get_extension(self) -> str:
        return "zip" if self.platform == "windows" else "tar.xz"

    def create_archive(self) -> Path:
        name = self.get_archive_name()
        ext = self._get_extension()
        archive_path = self.output_dir / f"{name}.{ext}"

        print(f"Creating archive: {name}")
        print(f"  Platform: {self.platform}")
        print(f"  Arch: {self.arch}")
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
        shutil.make_archive(
            str(archive_path).removesuffix(".zip"),
            "zip",
            root_dir=str(self.prefix.parent),
            base_dir=self.prefix.name,
        )

    def _create_tar_archive(self, archive_path: Path) -> None:
        parent_dir = self.prefix.parent
        folder_name = self.prefix.name
        cmd = ["tar", "-C", str(parent_dir), "-cJf", str(archive_path), folder_name]
        run_command(cmd)

    def upload_to_s3(self) -> None:
        if not self._archive_result:
            raise RuntimeError("Must call create_archive() before upload_to_s3()")
        full_name = f"{self._archive_result.name}.{self._archive_result.extension}"
        key = f"dependencies/gstreamer/{self.platform}/{self.version}/{full_name}"
        upload_public_file(self._archive_result.path, S3_BUCKET, key)
        print(f"Uploaded to: s3://{S3_BUCKET}/{key}")

    def write_github_output(self) -> None:
        if not self._archive_result:
            return
        write_github_output(
            {
                "name": self._archive_result.name,
                "path": str(self._archive_result.path),
            }
        )

    def write_github_summary(self, uploaded: bool = False) -> None:
        if not self._archive_result:
            return

        arch_display = self.arch
        if self.platform == "ios" and self.simulator:
            arch_display = f"{arch_display}-simulator"

        full_name = f"{self._archive_result.name}.{self._archive_result.extension}"
        s3_path = f"dependencies/gstreamer/{self.platform}/{self.version}/" if uploaded else ""
        write_step_summary(
            summary_section(
                f"GStreamer {self.platform.capitalize()} SDK",
                [
                    ("Version", self.version),
                    ("Architecture", arch_display),
                    ("Archive", code_span(full_name)),
                    ("S3 path", code_span(s3_path) if s3_path else "not uploaded"),
                ],
            )
        )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create GStreamer archive and optionally upload to S3",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
--upload-s3 requires the AWS CLI and these environment variables:
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
        help="Architecture as named in the archive (x86_64, aarch64, arm64; x64 for Windows x86_64)",
    )
    parser.add_argument(
        "--version",
        default="",
        help="GStreamer version (default: the platform's in build-config.json)",
    )
    parser.add_argument(
        "--prefix",
        type=Path,
        required=True,
        help="GStreamer install prefix to archive",
    )
    parser.add_argument(
        "--simulator",
        action="store_true",
        help="iOS simulator build (default: false)",
    )
    parser.add_argument(
        "--upload-s3",
        action="store_true",
        help="Upload to S3 (requires the AWS CLI and credentials)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Output directory for archive (default: $RUNNER_TEMP or .)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    try:
        if args.upload_s3:
            require_s3_upload_tools()
        archiver = GStreamerArchiver(
            platform=args.platform,
            arch=args.arch,
            version=args.version or get_gstreamer_version(args.platform),
            prefix=args.prefix,
            simulator=args.simulator,
            output_dir=args.output_dir,
        )

        archiver.create_archive()
        archiver.write_github_output()
        if args.upload_s3:
            archiver.upload_to_s3()
        archiver.write_github_summary(uploaded=args.upload_s3)
        return 0
    except (ValueError, RuntimeError, MissingToolError, subprocess.CalledProcessError) as e:
        gh_error(str(e))
        return 1


if __name__ == "__main__":
    sys.exit(main())
