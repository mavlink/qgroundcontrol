#!/usr/bin/env python3
"""Tests for aws_upload.py."""

from __future__ import annotations

import pytest

from aws_upload import sanitize_ref, validate_artifact, validate_credentials


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
