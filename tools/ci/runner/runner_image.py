#!/usr/bin/env python3
"""Verify and promote smoke-tested QGC AMIs and retire only expired QGC-owned images.

`runner-images.yml` runs `preflight` before it requests AWS credentials, `manifest` to output the
built candidate (`image`), `verify` on the candidate, and `promote` and `cleanup` after the smoke
test. AWS commands take their region from `--region`, else AWS_REGION.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any

PREFIX = "qgc-runs-on-ubuntu24-x64-"
CANDIDATE_PREFIX = "qgc-candidate-ubuntu24-x64-"
SDK_ROOT = Path("/opt/qgc-sdk")
REQUIRED_PACKAGES = ("libgstreamer1.0-dev", "libxcb-cursor-dev", "patchelf")


def ec2(region: str, *args: str) -> dict[str, Any]:
    result = subprocess.run(
        ["aws", "ec2", *args, "--region", region, "--output", "json", "--no-cli-pager"],
        check=True,
        capture_output=True,
        text=True,
        timeout=1200,
    )
    return json.loads(result.stdout) if result.stdout.strip() else {}


def preflight(ref: str, default_branch: str, role_arn: str, subnet_id: str) -> list[str]:
    """Return why this run must not build an image: another branch or unset AWS settings."""
    errors = []
    expected = f"refs/heads/{default_branch}"
    if ref != expected:
        errors.append(f"Runner images must be built from {expected}; got {ref}")
    if not role_arn or not subnet_id:
        errors.append(
            "Configure RUNS_ON_AMI_ROLE_ARN and RUNS_ON_AMI_SUBNET_ID before dispatching this workflow"
        )
    return errors


def write_output(name: str, value: str) -> None:
    if path := os.environ.get("GITHUB_OUTPUT"):
        with open(path, "a", encoding="utf-8") as output:
            output.write(f"{name}={value}\n")


def manifest_image(manifest: Path, region: str) -> str:
    artifact = json.loads(manifest.read_text(encoding="utf-8"))["builds"][-1]["artifact_id"]
    match = re.fullmatch(re.escape(region) + r":(ami-[0-9a-f]+)", artifact)
    if not match:
        raise ValueError(f"Unexpected Packer artifact: {artifact}")
    return match[1]


def owned_image(image: dict[str, Any]) -> bool:
    tags = {tag["Key"]: tag["Value"] for tag in image.get("Tags", [])}
    return (
        tags.get("Project") == "qgroundcontrol"
        and tags.get("ManagedBy") == "packer"
        and image.get("Name", "").startswith((PREFIX, CANDIDATE_PREFIX))
    )


def expired_images(images: list[dict[str, Any]], in_use: set[str], now: datetime) -> list[str]:
    owned = sorted(
        (image for image in images if owned_image(image)),
        key=lambda image: image["CreationDate"],
        reverse=True,
    )
    approved = [
        image
        for image in owned
        if image["Name"].startswith(PREFIX) and image["State"] == "available"
    ]
    protected = in_use | {image["ImageId"] for image in approved[:3]}
    return [
        image["ImageId"]
        for image in owned
        if image["ImageId"] not in protected
        and datetime.fromisoformat(image["CreationDate"].replace("Z", "+00:00"))
        < now - timedelta(days=7 if image["Name"].startswith(CANDIDATE_PREFIX) else 30)
    ]


def promote(region: str, image_id: str, commit: str) -> str:
    images = ec2(region, "describe-images", "--owners", "self", "--image-ids", image_id)["Images"]
    if len(images) != 1 or not owned_image(images[0]):
        raise ValueError("Candidate is not a QGC-owned image")
    candidate = images[0]
    tags = {tag["Key"]: tag["Value"] for tag in candidate.get("Tags", [])}
    if (
        not candidate["Name"].startswith(CANDIDATE_PREFIX)
        or candidate["State"] != "available"
        or tags.get("SourceCommit") != commit
    ):
        raise ValueError("Candidate state or source commit does not match the smoke-tested build")
    approved = ec2(
        region,
        "copy-image",
        "--source-region",
        region,
        "--source-image-id",
        image_id,
        "--name",
        PREFIX + candidate["Name"].removeprefix(CANDIDATE_PREFIX),
        "--copy-image-tags",
        "--client-token",
        image_id,
    )["ImageId"]
    ec2(region, "wait", "image-available", "--image-ids", approved)
    return approved


def cleanup(region: str) -> None:
    images = ec2(
        region,
        "describe-images",
        "--owners",
        "self",
        "--filters",
        "Name=tag:Project,Values=qgroundcontrol",
        "Name=tag:ManagedBy,Values=packer",
    )["Images"]
    reservations = ec2(
        region,
        "describe-instances",
        "--filters",
        "Name=instance-state-name,Values=pending,running,stopping,stopped",
    )["Reservations"]
    in_use = {
        instance["ImageId"] for reservation in reservations for instance in reservation["Instances"]
    }
    for image_id in expired_images(images, in_use, datetime.now(timezone.utc)):
        print(f"Retiring {image_id}", flush=True)
        result = ec2(
            region, "deregister-image", "--image-id", image_id, "--delete-associated-snapshots"
        )
        if result.get("Return") is not True or any(
            snapshot["ReturnCode"] not in {"success", "skipped"}
            for snapshot in result.get("DeleteSnapshotResults", [])
        ):
            raise RuntimeError(f"Incomplete image cleanup: {result}")


def verify_image(sdk_root: Path = SDK_ROOT) -> None:
    """Check the running image's preinstalled Qt SDK, CPM seed, and build tools."""
    qt_roots = sorted(sdk_root.glob("Qt/*/gcc_64"))
    if len(qt_roots) != 1:
        raise RuntimeError(f"Expected exactly one preinstalled Qt SDK, found {len(qt_roots)}")
    qt_cmake = qt_roots[0] / "bin" / "qt-cmake"
    modules = qt_roots[0] / ".qgc-modules"
    manifest = sdk_root / "cpm" / "manifest.json"
    sources = sdk_root / "cpm" / "sources"
    checks = {
        f"{qt_cmake} is not executable": qt_cmake.is_file() and os.access(qt_cmake, os.X_OK),
        f"{modules} is missing": modules.is_file(),
        f"{manifest} is missing or empty": manifest.is_file() and manifest.stat().st_size > 0,
        f"{sources} is missing": sources.is_dir(),
    }
    if failures := [message for message, passed in checks.items() if not passed]:
        raise RuntimeError("; ".join(failures))
    for command in (
        [str(qt_cmake), "--version"],
        ["cmake", "--version"],
        ["ninja", "--version"],
        ["dpkg-query", "-W", *REQUIRED_PACKAGES],
    ):
        try:
            subprocess.run(command, check=True, timeout=120)
        except (OSError, subprocess.SubprocessError) as error:
            raise RuntimeError(f"{' '.join(command)} failed: {error}") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command", choices=["preflight", "manifest", "promote", "cleanup", "verify"]
    )
    parser.add_argument(
        "--region",
        default=os.environ.get("AWS_REGION"),
        help="AWS region (default: $AWS_REGION; required except for preflight and verify)",
    )
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--image")
    parser.add_argument("--commit")
    args = parser.parse_args(argv)
    if args.command == "preflight":
        errors = preflight(
            os.environ.get("GITHUB_REF", ""),
            os.environ.get("DEFAULT_BRANCH", ""),
            os.environ.get("ROLE_ARN", ""),
            os.environ.get("SUBNET_ID", ""),
        )
        for error in errors:
            print(f"::error::{error}", flush=True)
        return 1 if errors else 0
    if args.command == "verify":
        try:
            verify_image()
        except RuntimeError as error:
            print(f"::error::{error}", flush=True)
            return 1
        return 0
    if not args.region:
        parser.error(f"{args.command} requires --region")
    if args.command == "manifest":
        if args.manifest is None:
            parser.error("manifest requires --manifest")
        image = manifest_image(args.manifest, args.region)
        print(image)
        write_output("image", image)
    elif args.command == "promote":
        if not args.image or not args.commit:
            parser.error("promote requires --image and --commit")
        print(promote(args.region, args.image, args.commit))
    else:
        cleanup(args.region)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
