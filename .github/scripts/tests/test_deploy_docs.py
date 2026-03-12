#!/usr/bin/env python3
"""Tests for deploy_docs.py."""

from __future__ import annotations

from deploy_docs import sanitize_branch


class TestSanitizeBranch:
    def test_simple(self) -> None:
        assert sanitize_branch("main") == "main"

    def test_slashes(self) -> None:
        assert sanitize_branch("feature/docs-update") == "feature_docs-update"

    def test_special_chars(self) -> None:
        assert sanitize_branch("v1.2.3-rc1") == "v1.2.3-rc1"

    def test_spaces(self) -> None:
        assert sanitize_branch("my branch") == "my_branch"

    def test_preserves_dots_dashes_underscores(self) -> None:
        assert sanitize_branch("release_v2.0-beta") == "release_v2.0-beta"
