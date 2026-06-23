#!/usr/bin/env python3
"""Tests for aws_upload.py."""

from __future__ import annotations

import pytest
from aws_upload import (
    main,
    resolve_auth_mode,
    sanitize_ref,
    validate_artifact,
    validate_credentials,
)


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


class TestValidateCredentials:
    def test_role_arn_sufficient(self) -> None:
        validate_credentials("arn:aws:iam::123:role/test", "", "")

    def test_static_creds_sufficient(self) -> None:
        validate_credentials("", "AKID", "secret")

    def test_missing_both_exits(self) -> None:
        with pytest.raises(SystemExit):
            validate_credentials("", "", "")

    def test_partial_static_exits(self) -> None:
        with pytest.raises(SystemExit):
            validate_credentials("", "AKID", "")


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


class TestResolveAuthMode:
    def test_role_arn_wins_over_key(self) -> None:
        assert resolve_auth_mode("arn:aws:iam::123:role/x", "AKID") == "oidc"

    def test_role_arn_alone(self) -> None:
        assert resolve_auth_mode("arn:aws:iam::123:role/x", "") == "oidc"

    def test_key_id_alone(self) -> None:
        assert resolve_auth_mode("", "AKID") == "static"

    def test_neither_returns_none(self) -> None:
        assert resolve_auth_mode("", "") == "none"


class TestAuthModeCmd:
    def test_writes_output_and_stdout(self, tmp_path, monkeypatch, capsys) -> None:
        output_file = tmp_path / "gh_output"
        output_file.write_text("")
        monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
        monkeypatch.setattr(
            "sys.argv",
            ["prog", "auth-mode", "--role-arn", "arn:aws:iam::1:role/x"],
        )
        main()
        assert capsys.readouterr().out.strip() == "oidc"
        assert "mode=oidc" in output_file.read_text()

    def test_emits_none_when_unset(self, tmp_path, monkeypatch, capsys) -> None:
        output_file = tmp_path / "gh_output"
        output_file.write_text("")
        monkeypatch.setenv("GITHUB_OUTPUT", str(output_file))
        monkeypatch.setattr("sys.argv", ["prog", "auth-mode"])
        main()
        assert capsys.readouterr().out.strip() == "none"
        assert "mode=none" in output_file.read_text()
