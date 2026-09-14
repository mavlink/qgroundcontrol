#!/usr/bin/env python3
"""Select iOS signing profiles, verify bundles, and package validated IPA files."""

from __future__ import annotations

import argparse
import json
import os
import plistlib
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output


def select_profile() -> None:
    team = os.environ.get("TEAM_ID", "").strip()
    name = os.environ.get("PROFILE_NAME", "").strip()
    bundle = os.environ.get("BUNDLE_ID", "").strip()
    if not team or not name or not bundle:
        raise ValueError("TEAM_ID, PROFILE_NAME, and BUNDLE_ID are required")
    profiles = json.loads(os.environ.get("PROFILES", "[]"))
    if not isinstance(profiles, list) or any(not isinstance(item, dict) for item in profiles):
        raise ValueError("PROFILES must be a JSON array of profile objects")
    matches = [item for item in profiles if item.get("name") == name]
    if len(matches) != 1:
        raise ValueError(
            f"Expected one active IOS_APP_STORE profile named {name!r}; found {len(matches)}"
        )
    uuid = matches[0].get("udid")
    if not isinstance(uuid, str) or not uuid.strip():
        raise ValueError("Selected profile has no UUID")
    if any("\n" in value or "\r" in value or "\0" in value for value in (team, bundle, uuid)):
        raise ValueError("Signing values must be single-line strings")
    write_github_output(
        {
            "cmake_args": shlex.join(
                [
                    "-DQGC_IOS_APP_STORE_BUILD=ON",
                    f"-DQGC_IOS_DEVELOPMENT_TEAM={team}",
                    f"-DQGC_IOS_PROVISIONING_PROFILE={uuid}",
                    f"-DQGC_PACKAGE_NAME={bundle}",
                ]
            )
        }
    )


def verify_bundle(app: Path, arch: str, *, signed: bool = False) -> None:
    for relative in ("Info.plist", "Assets.car", app.stem):
        if not (app / relative).is_file():
            raise ValueError(f"Missing bundle file: {app / relative}")
    for relative in ("QGCLaunchScreen.storyboardc", "Frameworks/gstreamer_mobile.framework"):
        if not (app / relative).is_dir():
            raise ValueError(f"Missing bundle directory: {app / relative}")
    if (app / "QGCLaunchScreen.storyboard").exists():
        raise ValueError("Bundle contains an uncompiled launch storyboard")
    metadata = plistlib.loads((app / "Info.plist").read_bytes())
    subprocess.run(["lipo", str(app / app.stem), "-verify_arch", arch], check=True)
    if signed:
        expected = os.environ.get("APPSTORE_BUNDLE_ID", "")
        if not expected or metadata.get("CFBundleIdentifier") != expected:
            raise ValueError("Signed bundle ID does not match APPSTORE_BUNDLE_ID")
        if not (app / "embedded.mobileprovision").is_file():
            raise ValueError("Signed bundle has no embedded provisioning profile")
        subprocess.run(
            ["codesign", "--verify", "--deep", "--strict", "--verbose=2", str(app)], check=True
        )


def package(app: Path) -> Path:
    app = app.resolve()
    signed = os.environ.get("GITHUB_REF_TYPE") == "tag" and os.environ.get(
        "GITHUB_REF_NAME", ""
    ).startswith("v")
    verify_bundle(app, "arm64", signed=signed)
    output = app.with_suffix(".ipa")
    # ditto preserves Apple bundle metadata and signatures; publish only after ZIP validation.
    with tempfile.TemporaryDirectory(prefix=".qgc-ipa-", dir=app.parent) as temporary:
        root = Path(temporary)
        payload = root / "Payload"
        payload.mkdir()
        subprocess.run(["ditto", str(app), str(payload / app.name)], check=True)
        archive = root / output.name
        subprocess.run(
            ["ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", str(payload), str(archive)],
            check=True,
        )
        subprocess.run(["unzip", "-tq", str(archive)], check=True)
        archive.replace(output)
    return output


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("select-profile")
    verify = commands.add_parser("verify")
    verify.add_argument("--app", type=Path, required=True)
    verify.add_argument("--arch", choices=("arm64", "x86_64"), required=True)
    archive = commands.add_parser("package")
    archive.add_argument(
        "--app", type=Path, default=Path(f"{os.environ.get('PACKAGE', 'QGroundControl')}.app")
    )
    args = parser.parse_args(argv)
    try:
        if args.command == "select-profile":
            select_profile()
        elif args.command == "verify":
            verify_bundle(args.app, args.arch)
        else:
            print(package(args.app))
        return 0
    except (
        OSError,
        ValueError,
        plistlib.InvalidFileException,
        subprocess.CalledProcessError,
    ) as error:
        print(f"iOS packaging failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
