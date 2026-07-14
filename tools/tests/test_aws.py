"""Contracts for allowlisted public S3 operations."""

from __future__ import annotations

import subprocess

import pytest
from common import aws


def test_public_s3_uri_rejects_unapproved_or_unsafe_destinations() -> None:
    assert aws.public_s3_uri("qgroundcontrol", "builds/main/qgc.bin") == (
        "s3://qgroundcontrol/builds/main/qgc.bin"
    )
    for bucket, key in (
        ("other", "builds/main/qgc.bin"),
        ("qgroundcontrol", "../secret"),
        ("qgroundcontrol", "/absolute"),
        ("qgroundcontrol", "path\\windows"),
    ):
        with pytest.raises(ValueError):
            aws.public_s3_uri(bucket, key)


def test_upload_public_file_builds_command_and_surfaces_failure(
    tmp_path, monkeypatch: pytest.MonkeyPatch
) -> None:
    source = tmp_path / "qgc.bin"
    source.write_bytes(b"qgc")
    calls: list[list[str]] = []

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 0, "", "")

    monkeypatch.setattr(aws, "run_captured", run)
    aws.upload_public_file(source, "qgroundcontrol", "latest/qgc.bin")
    assert calls == [
        [
            "aws",
            "s3",
            "cp",
            str(source),
            "s3://qgroundcontrol/latest/qgc.bin",
            "--acl",
            "public-read",
        ]
    ]

    monkeypatch.setattr(
        aws,
        "run_captured",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 1, "", "denied"),
    )
    with pytest.raises(RuntimeError, match="denied"):
        aws.upload_public_file(source, "qgroundcontrol", "latest/qgc.bin")
