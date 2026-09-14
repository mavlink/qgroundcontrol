#!/usr/bin/env python3
"""Compile iOS assets and merge their metadata into a Ninja-built app bundle."""

from __future__ import annotations

import argparse
import os
import plistlib
import subprocess
import sys
import tempfile
from pathlib import Path


def merge_metadata(existing: dict, generated: dict) -> None:
    for key, value in generated.items():
        if isinstance(value, dict) and isinstance(existing.get(key), dict):
            merge_metadata(existing[key], value)
        else:
            existing[key] = value


def prepare_bundle(bundle: Path, minimum_version: str, platform: str) -> None:
    if not bundle.is_dir() or not (bundle / "Info.plist").is_file():
        raise ValueError(f"Invalid iOS app bundle: {bundle}")
    source = Path(__file__).resolve().parent
    xcrun = os.environ.get("XCRUN", "xcrun")
    targets = [
        "--target-device",
        "iphone",
        "--target-device",
        "ipad",
        "--minimum-deployment-target",
        minimum_version,
    ]
    with tempfile.TemporaryDirectory(prefix=".qgc-ios-assets-", dir=bundle) as directory:
        partial = Path(directory) / "assets.plist"
        subprocess.run(
            [
                xcrun,
                "actool",
                "--app-icon",
                "AppIcon",
                "--output-partial-info-plist",
                str(partial),
                "--platform",
                platform,
                *targets,
                "--compile",
                str(bundle),
                str(source / "Images.xcassets"),
            ],
            check=True,
        )
        subprocess.run(
            [
                xcrun,
                "ibtool",
                "--errors",
                "--warnings",
                "--notices",
                *targets,
                "--compile",
                str(bundle / "QGCLaunchScreen.storyboardc"),
                str(source / "QGCLaunchScreen.storyboard"),
            ],
            check=True,
        )
        if (
            not (bundle / "Assets.car").is_file()
            or not (bundle / "QGCLaunchScreen.storyboardc").is_dir()
        ):
            raise ValueError("Asset compilation did not produce the required bundle resources")
        info = bundle / "Info.plist"
        original = info.read_bytes()
        metadata = plistlib.loads(original)
        merge_metadata(metadata, plistlib.loads(partial.read_bytes()))
        merged = Path(directory) / "Info.plist"
        merged.write_bytes(
            plistlib.dumps(
                metadata,
                fmt=plistlib.FMT_BINARY if original.startswith(b"bplist") else plistlib.FMT_XML,
            )
        )
        merged.chmod(info.stat().st_mode & 0o777)
        merged.replace(info)
    (bundle / "QGCLaunchScreen.storyboard").unlink(missing_ok=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("minimum_version")
    parser.add_argument("platform", choices=("iphoneos", "iphonesimulator"))
    args = parser.parse_args(argv)
    try:
        prepare_bundle(args.bundle.resolve(), args.minimum_version, args.platform)
        return 0
    except (
        OSError,
        ValueError,
        subprocess.CalledProcessError,
        plistlib.InvalidFileException,
    ) as error:
        print(f"iOS bundle preparation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
