"""Contracts for iOS bundle preparation and release packaging."""

from __future__ import annotations

import json
import os
import plistlib
import struct
import subprocess
from typing import TYPE_CHECKING

import yaml
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

IOS_DIR = REPO_ROOT / "deploy/ios"
PREPARE_BUNDLE = IOS_DIR / "prepare-bundle.sh"
WORKFLOW = REPO_ROOT / ".github/workflows/ios.yml"
CI_SCRIPTS_WORKFLOW = REPO_ROOT / ".github/workflows/ci-scripts.yml"
QT_IOS_ACTION = REPO_ROOT / ".github/actions/qt-ios/action.yml"


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


def test_prepares_ninja_ios_bundle(tmp_path: Path) -> None:
    bundle = tmp_path / "QGroundControl.app"
    bundle.mkdir()
    (bundle / "Info.plist").write_text("<?xml version='1.0'?><plist><dict/></plist>\n")
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
        printf "<?xml version='1.0'?><plist><dict/></plist>\\n" > "$partial_plist"
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

    plist_buddy = tmp_path / "PlistBuddy"
    _write_executable(
        plist_buddy,
        """#!/usr/bin/env bash
set -euo pipefail
printf 'PlistBuddy %s\\n' "$*" >> "$CALLS_LOG"
""",
    )

    env = os.environ | {
        "CALLS_LOG": str(calls),
        "PLIST_BUDDY": str(plist_buddy),
        "XCRUN": str(xcrun),
    }
    subprocess.run(
        ["bash", str(PREPARE_BUNDLE), str(bundle), "17.0", "iphoneos"],
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
    assert "PlistBuddy -c Merge " in log


def test_rejects_unsupported_ios_platform(tmp_path: Path) -> None:
    result = subprocess.run(
        ["bash", str(PREPARE_BUNDLE), str(tmp_path), "17.0", "macosx"],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 2
    assert result.stderr == "Unsupported iOS platform: macosx\n"


def test_ios_workflow_builds_device_and_simulator_targets() -> None:
    workflow = yaml.safe_load(WORKFLOW.read_text())
    job = workflow["jobs"]["build"]

    assert job["runs-on"] == "${{ matrix.runner }}"
    assert job["strategy"]["matrix"]["include"] == [
        {
            "target": "device",
            "runner": "macos-15",
            "build_type": "Release",
            "preset": "iOS",
        },
        {
            "target": "simulator",
            "runner": "macos-15-intel",
            "build_type": "Debug",
            "preset": "iOS-debug",
        },
    ]

    steps = {step["name"]: step for step in job["steps"]}
    configure = steps["Configure"]["with"]
    assert "'iOS-appstore'" in configure["preset"]
    assert "github.ref_type == 'tag'" in configure["preset"]
    assert "startsWith(github.ref_name, 'v')" in configure["preset"]
    assert "-DCMAKE_OSX_SYSROOT=iphonesimulator" in configure["extra-args"]
    assert "-DCMAKE_OSX_ARCHITECTURES=x86_64" in configure["extra-args"]
    simulator_verification = steps["Verify Simulator Bundle"]["run"]
    assert 'test -f "$APP_PATH/Assets.car"' in simulator_verification
    assert 'test -d "$APP_PATH/Frameworks/gstreamer_mobile.framework"' in simulator_verification
    assert 'lipo "$APP_PATH/$PACKAGE" -verify_arch x86_64' in simulator_verification

    package = steps["Package IPA"]["run"]
    assert 'lipo "$APP_PATH/$PACKAGE" -verify_arch arm64' in package
    assert 'test -d "$APP_PATH/Frameworks/gstreamer_mobile.framework"' in package
    assert "bundle_id=$(/usr/libexec/PlistBuddy" in package
    assert 'unzip -tq "${PACKAGE}.ipa"' in package

    for step_name in (
        "Import App Store signing certificate",
        "Download App Store provisioning profiles",
        "Select App Store provisioning profile",
        "Package IPA",
        "Attest and Upload",
        "Upload to TestFlight",
    ):
        assert "matrix.target == 'device'" in steps[step_name]["if"]

    for step_name in (
        "Import App Store signing certificate",
        "Download App Store provisioning profiles",
        "Select App Store provisioning profile",
        "Upload to TestFlight",
    ):
        condition = steps[step_name]["if"]
        assert "github.ref_type == 'tag'" in condition
        assert "startsWith(github.ref_name, 'v')" in condition

    assert '[[ "$GITHUB_REF_TYPE" == "tag" && "$GITHUB_REF_NAME" == v* ]]' in package
    attest = steps["Attest and Upload"]["with"]
    assert attest["package-name"] == "${{ env.PACKAGE }}-ios"
    assert attest["subject-name"] == "${{ env.PACKAGE }}-ios"


def test_ios_qt_action_exports_cross_compile_roots() -> None:
    action = yaml.safe_load(QT_IOS_ACTION.read_text())
    steps = {step["name"]: step for step in action["runs"]["steps"]}

    assert action["outputs"]["host_qt_root_dir"]["value"] == (
        "${{ steps.qt-host.outputs.qt_root_dir }}"
    )
    assert action["outputs"]["target_qt_root_dir"]["value"] == (
        "${{ steps.qt-target.outputs.qt_root_dir }}"
    )
    assert steps["Install Qt (Host macOS)"]["id"] == "qt-host"
    assert steps["Install Qt for iOS"]["id"] == "qt-target"


def test_ios_contract_dependencies_are_sparse_checked_out() -> None:
    workflow = yaml.safe_load(CI_SCRIPTS_WORKFLOW.read_text())
    steps = workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(step for step in steps if step["name"] == "Checkout")
    sparse_checkout = set(checkout["with"]["sparse-checkout"].splitlines())

    assert {
        ".github/actions/qt-ios",
        "cmake/modules/AppleXCFramework.cmake",
        "cmake/platform/Apple.cmake",
        "deploy/ios",
    } <= sparse_checkout


def test_ios_packaging_uses_consolidated_bundle_sources() -> None:
    for obsolete in (
        "AppStoreIcon_1024x1024.png",
        "iOS-Info.plist",
        "iOSForAppStore-Info-Source.plist",
        "QGCLaunchScreen.xib",
        "qgroundcontrol.xcconfig",
        "qgroundcontrol_appstore.xcconfig",
    ):
        assert not (IOS_DIR / obsolete).exists()

    plist_text = (IOS_DIR / "Info.plist.app.in").read_text()
    plist = plistlib.loads(plist_text.encode())
    apple = (REPO_ROOT / "cmake/platform/Apple.cmake").read_text()
    presets = json.loads((REPO_ROOT / "cmake/presets/iOS.json").read_text())

    assert plist["CFBundleIdentifier"] == "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
    assert plist["UILaunchStoryboardName"] == "QGCLaunchScreen"
    assert "NSBluetoothAlwaysUsageDescription" in plist
    assert "NSLocalNetworkUsageDescription" in plist
    assert "QGCLaunchScreen.storyboard" in apple
    assert "qgc_find_ios_xcframework_slice" in apple
    assert "QGC_IOS_APP_STORE_BUILD requires the Xcode generator" in apple
    assert "XCODE_EMBED_FRAMEWORKS GStreamerMobileXcfw" in apple
    assert "XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES" in apple
    assert "IOSPipe2Compat.c" in apple
    assert "TARGET_BUNDLE_DIR_NAME:GStreamerMobileXcfw" in apple
    assert "${GStreamer_Mobile_MODULE_NAME}.framework" not in apple
    assert "GStreamer_VERSION VERSION_" not in apple
    assert any(
        preset["name"] == "iOS-appstore" and preset["generator"] == "Xcode"
        for preset in presets["configurePresets"]
    )
    assert 'XCODE_ATTRIBUTE_MARKETING_VERSION "${CMAKE_PROJECT_VERSION}"' in apple


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
        "ARCHITECTURES arm64 OUT_VAR device_framework)\n"
        f'qgc_find_ios_xcframework_slice(XCFRAMEWORK "{xcframework}" PLATFORM iphonesimulator '
        "ARCHITECTURES arm64 x86_64 OUT_VAR simulator_framework)\n"
        f'file(WRITE "{result_path}" "${{device_framework}}\n${{simulator_framework}}\n")\n'
    )

    subprocess.run(["cmake", "-P", str(script)], check=True, capture_output=True, text=True)

    assert result_path.read_text().splitlines() == [
        str(xcframework / "ios-arm64/Video.framework"),
        str(xcframework / "ios-arm64_x86_64-simulator/Video.framework"),
    ]
