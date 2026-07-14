from __future__ import annotations

import os
import subprocess
from typing import TYPE_CHECKING

from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

_SCRIPT = REPO_ROOT / "deploy" / "ios" / "prepare-bundle.sh"


def _write_executable(path: Path, contents: str) -> None:
    path.write_text(contents)
    path.chmod(0o755)


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
        ["bash", str(_SCRIPT), str(bundle), "17.0", "iphoneos"],
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
        ["bash", str(_SCRIPT), str(tmp_path), "17.0", "macosx"],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 2
    assert result.stderr == "Unsupported iOS platform: macosx\n"
