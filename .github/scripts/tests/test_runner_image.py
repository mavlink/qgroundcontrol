"""Exercise retention boundaries and promotion provenance without touching AWS."""

from __future__ import annotations

from datetime import datetime, timedelta, timezone

import pytest
import runner_image as runner

NOW = datetime(2026, 9, 14, tzinfo=timezone.utc)


def image(number, age, candidate=False):
    return {
        "ImageId": f"ami-{number:017x}",
        "Name": (runner.CANDIDATE_PREFIX if candidate else runner.PREFIX) + str(number),
        "CreationDate": (NOW - timedelta(days=age)).isoformat(),
        "State": "available",
        "Tags": [
            {"Key": "Project", "Value": "qgroundcontrol"},
            {"Key": "ManagedBy", "Value": "packer"},
            {"Key": "SourceCommit", "Value": "tested-commit"},
        ],
    }


def test_retention_preserves_recent_in_use_and_three_rollback_images():
    images = [image(n, 40 + n) for n in range(6)]
    images += [image(10, 7, True), image(11, 8, True), image(12, 90, True)]
    foreign = image(13, 100)
    foreign["Tags"] = []
    images.append(foreign)
    expired = runner.expired_images(images, {images[4]["ImageId"], images[8]["ImageId"]}, NOW)
    assert set(expired) == {images[3]["ImageId"], images[5]["ImageId"], images[7]["ImageId"]}


@pytest.mark.parametrize("invalid", ["commit", "owner", "state", "production"])
def test_promotion_rejects_unverified_candidates(monkeypatch, invalid):
    candidate = image(1, 0, True)
    if invalid == "commit":
        candidate["Tags"][-1]["Value"] = "different-commit"
    elif invalid == "owner":
        candidate["Tags"] = []
    elif invalid == "state":
        candidate["State"] = "pending"
    else:
        candidate["Name"] = runner.PREFIX + "already-promoted"
    calls = []

    def ec2(_region, *args):
        calls.append(args)
        return {"Images": [candidate]}

    monkeypatch.setattr(runner, "ec2", ec2)
    with pytest.raises(ValueError):
        runner.promote("us-west-2", candidate["ImageId"], "tested-commit")
    assert len(calls) == 1


def test_promotion_copies_exact_candidate_and_waits(monkeypatch):
    candidate = image(1, 0, True)
    calls = []

    def ec2(_region, *args):
        calls.append(args)
        if args[0] == "describe-images":
            return {"Images": [candidate]}
        if args[0] == "copy-image":
            return {"ImageId": "ami-approved"}
        return {}

    monkeypatch.setattr(runner, "ec2", ec2)
    assert runner.promote("us-west-2", candidate["ImageId"], "tested-commit") == "ami-approved"
    assert calls[1][calls[1].index("--source-image-id") + 1] == candidate["ImageId"]
    assert calls[1][calls[1].index("--name") + 1].startswith(runner.PREFIX)
    assert calls[2] == ("wait", "image-available", "--image-ids", "ami-approved")


def test_unavailable_images_do_not_displace_rollback_images():
    images = [image(n, 40 + n) for n in range(5)]
    images[0]["State"] = "failed"
    images[1]["State"] = "pending"
    assert set(runner.expired_images(images, set(), NOW)) == {
        images[0]["ImageId"],
        images[1]["ImageId"],
    }


def test_manifest_rejects_wrong_region(tmp_path):
    import json

    manifest = tmp_path / "manifest.json"
    manifest.write_text(json.dumps({"builds": [{"artifact_id": "us-west-2:ami-123abc"}]}))
    assert runner.manifest_image(manifest, "us-west-2") == "ami-123abc"
    with pytest.raises(ValueError):
        runner.manifest_image(manifest, "us-east-1")
