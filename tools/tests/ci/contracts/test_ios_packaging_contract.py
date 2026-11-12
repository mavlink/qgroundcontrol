"""Contracts for iOS bundle preparation and release packaging."""

from __future__ import annotations

import json
import os
import plistlib
import re
import struct
import subprocess
import sys
from typing import TYPE_CHECKING

import pytest

from ..._helpers import REPO_ROOT, load_workflow

if TYPE_CHECKING:
    from pathlib import Path

IOS_DIR = REPO_ROOT / "deploy/ios"
PREPARE_BUNDLE = IOS_DIR / "prepare_bundle.py"


def _write_executable(path: Path, contents: str) -> None:
    path.write_text(contents)
    path.chmod(0o755)


def _png_metadata(path: Path) -> tuple[int, int, int]:
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    assert data[12:16] == b"IHDR"
    width, height, _bit_depth, color_type, _compression, _filter, _interlace = struct.unpack(
        ">IIBBBBB", data[16:29]
    )
    return width, height, color_type


@pytest.mark.parametrize("binary", [False, True])
def test_prepares_ninja_ios_bundle(tmp_path: Path, binary: bool) -> None:
    bundle = tmp_path / "QGroundControl.app"
    bundle.mkdir()
    (bundle / "Info.plist").write_bytes(
        plistlib.dumps(
            {"CFBundleIdentifier": "org.test.qgc", "CFBundleIcons": {"Existing": True}},
            fmt=plistlib.FMT_BINARY if binary else plistlib.FMT_XML,
        )
    )
    (bundle / "QGCLaunchScreen.storyboard").write_text("uncompiled\n")

    calls = tmp_path / "calls.log"
    xcrun = tmp_path / "xcrun"
    _write_executable(
        xcrun,
        """#!/usr/bin/env bash
set -euo pipefail
printf '%s\\n' "$*" >> "$CALLS_LOG"
tool=$1
shift
case "$tool" in
    actool)
        while (( $# )); do
            case "$1" in
                --output-partial-info-plist)
                    partial_plist=$2
                    shift 2
                    ;;
                --compile)
                    bundle_path=$2
                    shift 2
                    ;;
                *) shift ;;
            esac
        done
        touch "$bundle_path/Assets.car"
        printf "<?xml version='1.0'?><plist><dict><key>CFBundleIcons</key><dict><key>Generated</key><true/></dict></dict></plist>\\n" > "$partial_plist"
        ;;
    ibtool)
        while (( $# )); do
            if [[ $1 == --compile ]]; then
                mkdir -p "$2"
                break
            fi
            shift
        done
        ;;
esac
""",
    )

    env = os.environ | {
        "CALLS_LOG": str(calls),
        "XCRUN": str(xcrun),
    }
    subprocess.run(
        [sys.executable, str(PREPARE_BUNDLE), str(bundle), "17.0", "iphoneos"],
        check=True,
        env=env,
    )

    assert (bundle / "Assets.car").is_file()
    assert (bundle / "QGCLaunchScreen.storyboardc").is_dir()
    assert not (bundle / "QGCLaunchScreen.storyboard").exists()
    log = calls.read_text()
    assert "actool --app-icon AppIcon" in log
    assert "--platform iphoneos" in log
    assert "ibtool --errors --warnings --notices" in log
    metadata = plistlib.loads((bundle / "Info.plist").read_bytes())
    assert metadata["CFBundleIdentifier"] == "org.test.qgc"
    assert metadata["CFBundleIcons"] == {"Existing": True, "Generated": True}
    assert (bundle / "Info.plist").read_bytes().startswith(b"bplist") == binary
    assert not list(bundle.glob(".qgc-ios-assets-*"))


def test_rejects_unsupported_ios_platform(tmp_path: Path) -> None:
    result = subprocess.run(
        [sys.executable, str(PREPARE_BUNDLE), str(tmp_path), "17.0", "macosx"],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 2
    assert "invalid choice" in result.stderr
    assert "macosx" in result.stderr


def test_app_store_release_requires_every_secret_and_variable_its_steps_read() -> None:
    """A tag signs and uploads only when nothing its signing and upload steps read is empty."""
    job = load_workflow("ios.yml")["jobs"]["build"]
    read = set(re.findall(r"(?:secrets|vars)\.APPSTORE_\w+", str(job["steps"])))
    assert read
    gate = job["env"]["APPSTORE_RELEASE"]
    assert set(re.findall(r"((?:secrets|vars)\.APPSTORE_\w+) != ''", gate)) == read


def test_ios_asset_catalog_references_valid_icons() -> None:
    app_icon_dir = IOS_DIR / "Images.xcassets/AppIcon.appiconset"
    catalog = json.loads((app_icon_dir / "Contents.json").read_text())

    for image in catalog["images"]:
        filename = image.get("filename")
        if not filename:
            continue

        width, height, color_type = _png_metadata(app_icon_dir / filename)
        expected_size = round(
            float(image["size"].split("x", 1)[0]) * int(image["scale"].removesuffix("x"))
        )
        assert (width, height) == (expected_size, expected_size)
        if image["idiom"] == "ios-marketing":
            assert color_type not in (4, 6)


def test_selects_matching_ios_xcframework_slices(tmp_path: Path) -> None:
    xcframework = tmp_path / "Video.xcframework"
    for slice_name in ("ios-arm64", "ios-arm64_x86_64-simulator", "ios-arm64-maccatalyst"):
        (xcframework / slice_name / "Video.framework").mkdir(parents=True)

    result_path = tmp_path / "selected.txt"
    script = tmp_path / "select-xcframework.cmake"
    script.write_text(
        f'include("{REPO_ROOT}/cmake/modules/AppleXCFramework.cmake")\n'
        f'qgc_find_ios_xcframework_slice(XCFRAMEWORK "{xcframework}" PLATFORM iphoneos '
        "ARCHITECTURES arm64 OUT_VAR device_slice)\n"
        f'qgc_find_ios_xcframework_slice(XCFRAMEWORK "{xcframework}" PLATFORM iphonesimulator '
        "ARCHITECTURES arm64 x86_64 OUT_VAR simulator_slice)\n"
        f'file(WRITE "{result_path}" "${{device_slice}}\n${{simulator_slice}}\n")\n'
    )

    subprocess.run(["cmake", "-P", str(script)], check=True, capture_output=True, text=True)

    assert result_path.read_text().splitlines() == [
        str(xcframework / "ios-arm64"),
        str(xcframework / "ios-arm64_x86_64-simulator"),
    ]
