#!/usr/bin/env python3
"""Tests for docker_helper.py."""

from __future__ import annotations

import argparse

import pytest
from docker_helper import cmd_validate, resolve_push_target


class TestValidate:
    def test_valid_linux_release(self) -> None:
        args = argparse.Namespace(target="linux", build_type="Release")
        cmd_validate(args)

    def test_valid_android_debug(self) -> None:
        args = argparse.Namespace(target="android", build_type="Debug")
        cmd_validate(args)

    def test_valid_linux_cross(self) -> None:
        args = argparse.Namespace(target="linux-cross", build_type="Release")
        cmd_validate(args)

    def test_invalid_target(self) -> None:
        args = argparse.Namespace(target="bogus", build_type="Release")
        with pytest.raises(SystemExit):
            cmd_validate(args)

    def test_invalid_build_type(self) -> None:
        args = argparse.Namespace(target="linux", build_type="BadType")
        with pytest.raises(SystemExit):
            cmd_validate(args)


class TestResolvePushTarget:
    UPSTREAM = "mavlink/qgroundcontrol"

    def test_upstream_release_tag_pushes_dockerhub(self) -> None:
        assert resolve_push_target("push", self.UPSTREAM, "refs/tags/v5.0.0") == "dronecode/qgroundcontrol"

    def test_upstream_master_pushes_ghcr_not_dockerhub(self) -> None:
        assert resolve_push_target("push", self.UPSTREAM, "refs/heads/master") == "ghcr.io/mavlink/qgroundcontrol"

    def test_upstream_stable_pushes_ghcr_not_dockerhub(self) -> None:
        assert resolve_push_target("push", self.UPSTREAM, "refs/heads/Stable_V4.4") == "ghcr.io/mavlink/qgroundcontrol"

    def test_upstream_feature_branch_no_push(self) -> None:
        assert resolve_push_target("push", self.UPSTREAM, "refs/heads/feature-x") == ""

    @pytest.mark.parametrize("ref", ["refs/tags/v5.0.0", "refs/heads/master", "refs/heads/Stable_V4.4"])
    def test_fork_never_pushes(self, ref: str) -> None:
        assert resolve_push_target("push", "someuser/qgroundcontrol", ref) == ""

    def test_pull_request_never_pushes(self) -> None:
        assert resolve_push_target("pull_request", self.UPSTREAM, "refs/tags/v5.0.0") == ""

    def test_workflow_dispatch_never_pushes(self) -> None:
        assert resolve_push_target("workflow_dispatch", self.UPSTREAM, "refs/heads/master") == ""
