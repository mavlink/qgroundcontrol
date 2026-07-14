#!/usr/bin/env python3
"""Security and CLI contracts for AWS artifact uploads."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from aws_upload import (
    main,
    resolve_auth_mode,
    sanitize_ref,
    validate_artifact,
    validate_credentials,
)

if TYPE_CHECKING:
    from pathlib import Path


def test_ref_sanitization() -> None:
    expected = {
        "main": "main",
        "feature/foo": "feature_foo",
        "../../../etc/passwd": "___etc_passwd",
        "v1.2.3-rc1": "v1.2.3-rc1",
        "my branch": "my_branch",
    }
    for ref, safe_ref in expected.items():
        assert sanitize_ref(ref) == safe_ref


def test_credentials_accept_complete_auth_and_reject_incomplete_auth() -> None:
    validate_credentials("arn:aws:iam::123:role/test", "", "")
    validate_credentials("", "AKID", "secret")
    for credentials in (("", "", ""), ("", "AKID", "")):
        with pytest.raises(SystemExit):
            validate_credentials(*credentials)


def test_artifact_validation_rejects_unsafe_names_and_missing_files(tmp_path: Path) -> None:
    artifact = tmp_path / "QGroundControl.AppImage"
    artifact.touch()
    validate_artifact(str(artifact), artifact.name)

    for name in ("../test.bin", "path/test.bin", "path\\test.bin"):
        with pytest.raises(SystemExit):
            validate_artifact(str(artifact), name)
    with pytest.raises(SystemExit):
        validate_artifact(str(tmp_path / "missing"), "test.bin")


def test_auth_mode_precedence() -> None:
    cases = [
        (("arn:aws:iam::123:role/x", "AKID"), "oidc"),
        (("arn:aws:iam::123:role/x", ""), "oidc"),
        (("", "AKID"), "static"),
        (("", ""), "none"),
    ]
    for inputs, expected in cases:
        assert resolve_auth_mode(*inputs) == expected


def test_auth_mode_command_writes_stdout_and_github_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    output_file = tmp_path / "gh_output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))

    for argv, expected in (
        (["prog", "auth-mode", "--role-arn", "arn:aws:iam::1:role/x"], "oidc"),
        (["prog", "auth-mode"], "none"),
    ):
        output_file.write_text("")
        monkeypatch.setattr("sys.argv", argv)
        main()
        assert capsys.readouterr().out.strip() == expected
        assert f"mode={expected}" in output_file.read_text()
