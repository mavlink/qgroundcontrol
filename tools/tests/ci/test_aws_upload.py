"""Tests for aws_upload.py."""

from __future__ import annotations

import argparse
import subprocess
from typing import TYPE_CHECKING

import common.aws
import pytest
from ci.publish import aws_upload
from ci.publish.aws_upload import main, parse_artifact, sanitize_ref, validate_artifact
from qgc_tools import release_tag_check

from .._helpers import completed, load_action

if TYPE_CHECKING:
    from collections.abc import Sequence
    from pathlib import Path

pytestmark = pytest.mark.windows


class TestSanitizeRef:
    def test_simple_branch(self) -> None:
        assert sanitize_ref("main") == "main"

    def test_slashes_replaced(self) -> None:
        assert sanitize_ref("feature/foo") == "feature_foo"

    def test_dotdot_removed(self) -> None:
        assert sanitize_ref("../../../etc/passwd") == "___etc_passwd"

    def test_special_chars(self) -> None:
        assert sanitize_ref("v1.2.3-rc1") == "v1.2.3-rc1"

    def test_spaces_replaced(self) -> None:
        assert sanitize_ref("my branch") == "my_branch"


class TestValidateArtifact:
    def test_path_traversal_rejected(self, tmp_path) -> None:
        f = tmp_path / "test.bin"
        f.touch()
        with pytest.raises(SystemExit):
            validate_artifact(str(f), "../test.bin")

    def test_slash_in_name_rejected(self, tmp_path) -> None:
        f = tmp_path / "test.bin"
        f.touch()
        with pytest.raises(SystemExit):
            validate_artifact(str(f), "path/test.bin")

    def test_backslash_rejected(self, tmp_path) -> None:
        f = tmp_path / "test.bin"
        f.touch()
        with pytest.raises(SystemExit):
            validate_artifact(str(f), "path\\test.bin")

    def test_missing_file_exits(self) -> None:
        with pytest.raises(SystemExit):
            validate_artifact("/nonexistent/file", "test.bin")

    def test_valid_artifact(self, tmp_path) -> None:
        f = tmp_path / "QGroundControl.AppImage"
        f.touch()
        validate_artifact(str(f), "QGroundControl.AppImage")


class TestParseArtifact:
    def test_posix_path(self) -> None:
        assert parse_artifact("/tmp/build/QGC.AppImage:QGC.AppImage") == (
            "/tmp/build/QGC.AppImage",
            "QGC.AppImage",
        )

    def test_windows_drive_letter_survives(self) -> None:
        assert parse_artifact("D:\\a\\_temp\\build\\QGC.exe.sha256:QGC.exe.sha256") == (
            "D:\\a\\_temp\\build\\QGC.exe.sha256",
            "QGC.exe.sha256",
        )

    @pytest.mark.parametrize("spec", ["QGC.AppImage", ":QGC.AppImage", "/tmp/QGC.AppImage:"])
    def test_rejects_missing_path_or_name(self, spec: str) -> None:
        with pytest.raises(argparse.ArgumentTypeError):
            parse_artifact(spec)


ARTIFACT = "QGroundControl.AppImage"
CHECKSUM = f"{ARTIFACT}.sha256"
DISTRIBUTION = "E123DIST"


class AwsRecorder:
    """Stand-in for the AWS CLI and git: records every command in call order."""

    def __init__(self, monkeypatch: pytest.MonkeyPatch, stable_branches: str = "") -> None:
        self.aws: list[list[str]] = []
        self.git: list[tuple[str, ...]] = []
        self.failing: set[str] = set()
        self.git_returncode = 0
        self.stable_branches = stable_branches
        monkeypatch.setattr(common.aws, "run_captured", self._aws)
        monkeypatch.setattr(aws_upload, "run_captured", self._aws)
        monkeypatch.setattr(release_tag_check, "run_git", self._git)

    def _aws(self, cmd: Sequence[str], *, check: bool = False, **_: object):
        command = list(cmd)
        self.aws.append(command)
        returncode = 1 if any(part in self.failing for part in command) else 0
        if check and returncode:
            raise subprocess.CalledProcessError(returncode, command, stderr="boom")
        return completed(returncode=returncode, stderr="boom" if returncode else "")

    def _git(self, *args: str, **_: object):
        self.git.append(args)
        return completed(self.stable_branches, self.git_returncode)


def _cp(source: Path, key: str) -> list[str]:
    return ["aws", "s3", "cp", str(source), f"s3://qgroundcontrol/{key}", "--acl", "public-read"]


def _invalidation(*names: str) -> list[str]:
    paths = [f"/latest/{name}" for name in names]
    return [
        "aws",
        "cloudfront",
        "create-invalidation",
        "--distribution-id",
        DISTRIBUTION,
        "--paths",
        *paths,
    ]


@pytest.fixture
def files(tmp_path: Path) -> tuple[Path, Path]:
    artifact = tmp_path / ARTIFACT
    artifact.write_bytes(b"artifact")
    checksum = tmp_path / CHECKSUM
    checksum.write_text(f"{'0' * 64}  {ARTIFACT}\n", encoding="utf-8")
    return artifact, checksum


def _publish(
    *artifacts: tuple[Path, str],
    ref_name: str,
    ref_type: str,
    distribution_id: str = DISTRIBUTION,
) -> None:
    argv = ["publish"]
    for path, name in artifacts:
        argv += ["--artifact", f"{path}:{name}"]
    argv += ["--ref-name", ref_name, "--ref-type", ref_type, "--distribution-id", distribution_id]
    assert main(argv) == 0


class TestPublish:
    def test_branch_artifact_only_uploads_to_builds(self, monkeypatch, files) -> None:
        artifact, _ = files
        aws = AwsRecorder(monkeypatch)

        _publish((artifact, ARTIFACT), ref_name="master", ref_type="branch")

        assert aws.aws == [_cp(artifact, f"builds/master/{ARTIFACT}")]
        assert aws.git == []

    def test_branch_artifact_and_checksum_upload_to_sanitized_builds_ref(
        self, monkeypatch, files
    ) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch)

        _publish(
            (artifact, ARTIFACT), (checksum, CHECKSUM), ref_name="feature/x", ref_type="branch"
        )

        assert aws.aws == [
            _cp(artifact, f"builds/feature_x/{ARTIFACT}"),
            _cp(checksum, f"builds/feature_x/{CHECKSUM}"),
        ]
        assert aws.git == []

    def test_stable_release_tag_publishes_latest_with_one_invalidation(
        self, monkeypatch, files
    ) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch, stable_branches="  origin/Stable_V5.1\n")

        _publish((artifact, ARTIFACT), (checksum, CHECKSUM), ref_name="v5.1.4", ref_type="tag")

        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.4/{ARTIFACT}"),
            _cp(checksum, f"builds/v5.1.4/{CHECKSUM}"),
            _cp(artifact, f"latest/{ARTIFACT}"),
            _cp(checksum, f"latest/{CHECKSUM}"),
            _invalidation(ARTIFACT, CHECKSUM),
        ]
        assert aws.git == [("branch", "-r", "--contains", "HEAD", "--list", "origin/Stable*")]

    def test_stable_release_tag_artifact_only(self, monkeypatch, files) -> None:
        artifact, _ = files
        aws = AwsRecorder(monkeypatch, stable_branches="origin/Stable_V5.1\n")

        _publish((artifact, ARTIFACT), ref_name="v5.1.4", ref_type="tag")

        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.4/{ARTIFACT}"),
            _cp(artifact, f"latest/{ARTIFACT}"),
            _invalidation(ARTIFACT),
        ]

    def test_release_tag_without_distribution_skips_invalidation(self, monkeypatch, files) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch, stable_branches="origin/Stable_V5.1\n")

        _publish(
            (artifact, ARTIFACT),
            (checksum, CHECKSUM),
            ref_name="v5.1.4",
            ref_type="tag",
            distribution_id="",
        )

        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.4/{ARTIFACT}"),
            _cp(checksum, f"builds/v5.1.4/{CHECKSUM}"),
            _cp(artifact, f"latest/{ARTIFACT}"),
            _cp(checksum, f"latest/{CHECKSUM}"),
        ]

    def test_tag_off_stable_branch_keeps_latest(self, monkeypatch, files, capsys) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch, stable_branches="")

        _publish((artifact, ARTIFACT), (checksum, CHECKSUM), ref_name="v5.1.4", ref_type="tag")

        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.4/{ARTIFACT}"),
            _cp(checksum, f"builds/v5.1.4/{CHECKSUM}"),
        ]
        assert len(aws.git) == 1
        assert "not on a Stable* branch; not publishing to latest/" in capsys.readouterr().out

    def test_non_release_tag_keeps_latest_without_querying_git(
        self, monkeypatch, files, capsys
    ) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch, stable_branches="origin/Stable_V5.1\n")

        _publish((artifact, ARTIFACT), (checksum, CHECKSUM), ref_name="v5.1.5-rc1", ref_type="tag")

        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.5-rc1/{ARTIFACT}"),
            _cp(checksum, f"builds/v5.1.5-rc1/{CHECKSUM}"),
        ]
        assert aws.git == []
        assert "not a vX.Y.Z release tag; not publishing to latest/" in capsys.readouterr().out

    def test_release_tag_check_failure_fails_after_builds_upload(self, monkeypatch, files) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch)
        aws.git_returncode = 128

        with pytest.raises(SystemExit) as excinfo:
            _publish((artifact, ARTIFACT), (checksum, CHECKSUM), ref_name="v5.1.4", ref_type="tag")

        assert excinfo.value.code == 1
        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.4/{ARTIFACT}"),
            _cp(checksum, f"builds/v5.1.4/{CHECKSUM}"),
        ]

    def test_invalid_artifact_fails_before_any_upload(self, monkeypatch, files) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch)

        with pytest.raises(SystemExit):
            _publish((artifact, ARTIFACT), (checksum, "../x"), ref_name="master", ref_type="branch")
        with pytest.raises(SystemExit):
            _publish(
                (artifact, ARTIFACT),
                (checksum.with_name("missing"), CHECKSUM),
                ref_name="master",
                ref_type="branch",
            )

        assert aws.aws == []

    def test_upload_failure_stops_publishing(self, monkeypatch, files) -> None:
        artifact, checksum = files
        aws = AwsRecorder(monkeypatch, stable_branches="origin/Stable_V5.1\n")
        aws.failing.add(f"s3://qgroundcontrol/builds/v5.1.4/{ARTIFACT}")

        with pytest.raises(SystemExit) as excinfo:
            _publish((artifact, ARTIFACT), (checksum, CHECKSUM), ref_name="v5.1.4", ref_type="tag")

        assert excinfo.value.code == 1
        assert aws.aws == [_cp(artifact, f"builds/v5.1.4/{ARTIFACT}")]

    def test_invalidation_failure_fails(self, monkeypatch, files) -> None:
        artifact, _ = files
        aws = AwsRecorder(monkeypatch, stable_branches="origin/Stable_V5.1\n")
        aws.failing.add("create-invalidation")

        with pytest.raises(subprocess.CalledProcessError):
            _publish((artifact, ARTIFACT), ref_name="v5.1.4", ref_type="tag")

    def test_requires_an_artifact(self) -> None:
        with pytest.raises(SystemExit) as excinfo:
            main(["publish", "--ref-name", "master", "--ref-type", "branch"])
        assert excinfo.value.code == 2

    def test_ref_defaults_to_the_runners_ref(self, monkeypatch, files) -> None:
        artifact, _ = files
        aws = AwsRecorder(monkeypatch, stable_branches="origin/Stable_V5.1\n")
        monkeypatch.setenv("GITHUB_REF_NAME", "v5.1.4")
        monkeypatch.setenv("GITHUB_REF_TYPE", "tag")

        assert main(["publish", "--artifact", f"{artifact}:{ARTIFACT}"]) == 0

        assert aws.aws == [
            _cp(artifact, f"builds/v5.1.4/{ARTIFACT}"),
            _cp(artifact, f"latest/{ARTIFACT}"),
        ]

    def test_requires_a_ref_outside_actions(self, files) -> None:
        artifact, _ = files
        with pytest.raises(SystemExit) as excinfo:
            main(["publish", "--artifact", f"{artifact}:{ARTIFACT}", "--ref-type", "branch"])
        assert excinfo.value.code == 2


def test_aws_credentials_selects_oidc_or_static_keys_in_one_step() -> None:
    action = load_action("aws-credentials")
    assert "outputs" not in action
    (step,) = action["runs"]["steps"]
    assert step["uses"] == "aws-actions/configure-aws-credentials@v6"
    assert step["if"] == "inputs.aws-role-arn != '' || inputs.aws-key-id != ''"
    assert step["with"] == {
        "role-to-assume": "${{ inputs.aws-role-arn }}",
        "aws-access-key-id": "${{ inputs.aws-role-arn == '' && inputs.aws-key-id || '' }}",
        "aws-secret-access-key": (
            "${{ inputs.aws-role-arn == '' && inputs.aws-secret-access-key || '' }}"
        ),
        "aws-region": "${{ inputs.aws-region }}",
    }
    assert action["inputs"]["aws-region"]["default"] == "us-west-2"
