"""Exercise retention boundaries and promotion provenance without touching AWS."""

from __future__ import annotations

import subprocess
from datetime import datetime, timedelta, timezone

import pytest
from ci.runner import runner_image as runner

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


QT_ROOT = "Qt/6.12.0/gcc_64"


def _sdk(tmp_path):
    qt_root = tmp_path / QT_ROOT
    qt_cmake = qt_root / "bin" / "qt-cmake"
    qt_cmake.parent.mkdir(parents=True)
    qt_cmake.write_text("#!/bin/sh\n")
    qt_cmake.chmod(0o755)
    (qt_root / ".qgc-modules").write_text("qtmultimedia\n")
    (tmp_path / "cpm" / "sources").mkdir(parents=True)
    (tmp_path / "cpm" / "manifest.json").write_text("{}")
    return qt_cmake


def test_verify_checks_sdk_layout_and_tool_versions(monkeypatch, tmp_path):
    qt_cmake = _sdk(tmp_path)
    calls = []
    monkeypatch.setattr(
        runner.subprocess,
        "run",
        lambda cmd, **kwargs: calls.append(cmd) or subprocess.CompletedProcess(cmd, 0),
    )

    runner.verify_image(tmp_path)

    assert calls == [
        [str(qt_cmake), "--version"],
        ["cmake", "--version"],
        ["ninja", "--version"],
        ["dpkg-query", "-W", "libgstreamer1.0-dev", "libxcb-cursor-dev", "patchelf"],
    ]


@pytest.mark.parametrize(
    ("break_sdk", "message"),
    [
        (lambda root: (root / "Qt/6.13.0/gcc_64").mkdir(parents=True), "found 2"),
        (lambda root: (root / QT_ROOT / "bin/qt-cmake").chmod(0o644), "qt-cmake"),
        (lambda root: (root / QT_ROOT / ".qgc-modules").unlink(), ".qgc-modules"),
        (lambda root: (root / "cpm/manifest.json").write_text(""), "manifest.json"),
        (lambda root: (root / "cpm/sources").rmdir(), "sources"),
    ],
    ids=["two-qt-sdks", "qt-cmake-not-executable", "no-modules", "empty-manifest", "no-sources"],
)
def test_verify_rejects_incomplete_sdk_before_running_tools(
    monkeypatch, tmp_path, break_sdk, message
):
    _sdk(tmp_path)
    break_sdk(tmp_path)
    monkeypatch.setattr(runner.subprocess, "run", lambda *_a, **_k: pytest.fail("tool ran"))

    with pytest.raises(RuntimeError, match=message):
        runner.verify_image(tmp_path)


def test_verify_reports_failed_tool_probe(monkeypatch, tmp_path):
    _sdk(tmp_path)

    def run(cmd, **kwargs):
        if cmd[0] == "dpkg-query":
            raise subprocess.CalledProcessError(1, cmd)
        return subprocess.CompletedProcess(cmd, 0)

    monkeypatch.setattr(runner.subprocess, "run", run)
    with pytest.raises(RuntimeError, match=r"dpkg-query -W libgstreamer1\.0-dev"):
        runner.verify_image(tmp_path)


def test_verify_cli_needs_no_region_but_aws_commands_do(monkeypatch, capsys):
    monkeypatch.delenv("AWS_REGION", raising=False)
    monkeypatch.setattr(runner, "verify_image", lambda: (_ for _ in ()).throw(RuntimeError("bad")))
    assert runner.main(["verify"]) == 1
    assert "::error::bad" in capsys.readouterr().out

    with pytest.raises(SystemExit) as exc:
        runner.main(["cleanup"])
    assert exc.value.code == 2


def test_aws_commands_default_to_the_workflow_region(monkeypatch):
    monkeypatch.setenv("AWS_REGION", "eu-central-1")
    regions = []
    monkeypatch.setattr(runner, "cleanup", regions.append)
    assert runner.main(["cleanup"]) == 0
    assert runner.main(["cleanup", "--region", "us-west-2"]) == 0
    assert regions == ["eu-central-1", "us-west-2"]


def test_manifest_outputs_the_candidate_image(monkeypatch, tmp_path, gh_output, capsys):
    import json

    manifest = tmp_path / "manifest.json"
    manifest.write_text(json.dumps({"builds": [{"artifact_id": "us-west-2:ami-0abc"}]}))
    monkeypatch.setenv("AWS_REGION", "us-west-2")

    assert runner.main(["manifest", "--manifest", str(manifest)]) == 0

    assert capsys.readouterr().out == "ami-0abc\n"
    assert gh_output.read_text() == "image=ami-0abc\n"


PREFLIGHT_OK = {
    "GITHUB_REF": "refs/heads/master",
    "DEFAULT_BRANCH": "master",
    "ROLE_ARN": "arn:aws:iam::1:role/ami",
    "SUBNET_ID": "subnet-1",
}
BRANCH_ERROR = "::error::Runner images must be built from refs/heads/master; got "
SETTINGS_ERROR = (
    "::error::Configure RUNS_ON_AMI_ROLE_ARN and RUNS_ON_AMI_SUBNET_ID before dispatching "
    "this workflow"
)


@pytest.mark.parametrize(
    ("changes", "errors"),
    [
        ({}, []),
        ({"GITHUB_REF": "refs/heads/feature"}, [BRANCH_ERROR + "refs/heads/feature"]),
        ({"GITHUB_REF": "refs/tags/v5.0.0"}, [BRANCH_ERROR + "refs/tags/v5.0.0"]),
        ({"ROLE_ARN": ""}, [SETTINGS_ERROR]),
        ({"SUBNET_ID": ""}, [SETTINGS_ERROR]),
        (
            {"GITHUB_REF": "refs/heads/Stable_V5.0", "ROLE_ARN": "", "SUBNET_ID": ""},
            [BRANCH_ERROR + "refs/heads/Stable_V5.0", SETTINGS_ERROR],
        ),
    ],
    ids=["ok", "branch", "tag", "no-role", "no-subnet", "everything"],
)
def test_preflight_requires_the_default_branch_and_aws_settings(
    monkeypatch, capsys, changes, errors
):
    for name, value in {**PREFLIGHT_OK, **changes}.items():
        monkeypatch.setenv(name, value)
    monkeypatch.setattr(runner, "ec2", lambda *_args: pytest.fail("preflight called AWS"))

    assert runner.main(["preflight"]) == (1 if errors else 0)
    assert capsys.readouterr().out.splitlines() == errors
