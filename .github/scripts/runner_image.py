#!/usr/bin/env python3
"""Promote smoke-tested QGC AMIs and retire only expired QGC-owned images."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any

PREFIX = "qgc-runs-on-ubuntu24-x64-"
CANDIDATE_PREFIX = "qgc-candidate-ubuntu24-x64-"


def ec2(region: str, *args: str) -> dict[str, Any]:
    result = subprocess.run(
        ["aws", "ec2", *args, "--region", region, "--output", "json", "--no-cli-pager"],
        check=True,
        capture_output=True,
        text=True,
        timeout=1200,
    )
    return json.loads(result.stdout) if result.stdout.strip() else {}


def manifest_image(manifest: Path, region: str) -> str:
    artifact = json.loads(manifest.read_text())["builds"][-1]["artifact_id"]
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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["manifest", "promote", "cleanup"])
    parser.add_argument("--region", required=True)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--image")
    parser.add_argument("--commit")
    args = parser.parse_args()
    if args.command == "manifest":
        if args.manifest is None:
            parser.error("manifest requires --manifest")
        print(manifest_image(args.manifest, args.region))
    elif args.command == "promote":
        if not args.image or not args.commit:
            parser.error("promote requires --image and --commit")
        print(promote(args.region, args.image, args.commit))
    else:
        cleanup(args.region)


if __name__ == "__main__":
    main()
