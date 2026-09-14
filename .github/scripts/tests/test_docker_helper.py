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
        assert (
            resolve_push_target("push", self.UPSTREAM, "refs/tags/v5.0.0")
            == "dronecode/qgroundcontrol"
        )

    def test_upstream_master_pushes_ghcr_not_dockerhub(self) -> None:
        assert (
            resolve_push_target("push", self.UPSTREAM, "refs/heads/master")
            == "ghcr.io/mavlink/qgroundcontrol"
        )

    def test_upstream_stable_pushes_ghcr_not_dockerhub(self) -> None:
        assert (
            resolve_push_target("push", self.UPSTREAM, "refs/heads/Stable_V4.4")
            == "ghcr.io/mavlink/qgroundcontrol"
        )

    def test_upstream_feature_branch_no_push(self) -> None:
        assert resolve_push_target("push", self.UPSTREAM, "refs/heads/feature-x") == ""

    @pytest.mark.parametrize(
        "ref", ["refs/tags/v5.0.0", "refs/heads/master", "refs/heads/Stable_V4.4"]
    )
    def test_fork_never_pushes(self, ref: str) -> None:
        assert resolve_push_target("push", "someuser/qgroundcontrol", ref) == ""

    def test_pull_request_never_pushes(self) -> None:
        assert resolve_push_target("pull_request", self.UPSTREAM, "refs/tags/v5.0.0") == ""

    def test_workflow_dispatch_never_pushes(self) -> None:
        assert resolve_push_target("workflow_dispatch", self.UPSTREAM, "refs/heads/master") == ""


def test_cache_key_runs_inside_the_login_environment(monkeypatch):
    import subprocess

    import docker_helper

    calls = []
    outputs = {}

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 0, "a" * 64 + "\n")

    monkeypatch.setattr(docker_helper.subprocess, "run", run)
    monkeypatch.setattr(docker_helper, "write_github_output", outputs.update)
    docker_helper.cmd_cache_key(argparse.Namespace(image="builder:linux"))
    assert calls[0][:7] == [
        "docker",
        "run",
        "--rm",
        "--entrypoint",
        "/bin/bash",
        "builder:linux",
        "-lc",
    ]
    assert calls[0][-1].endswith("/entrypoint.py --cache-key")
    assert outputs == {"fingerprint": "a" * 64}


def test_cache_key_rejects_invalid_container_output(monkeypatch):
    import subprocess

    import docker_helper

    monkeypatch.setattr(
        docker_helper.subprocess,
        "run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 0, "invalid\noutput\n"),
    )
    with pytest.raises(ValueError, match="fingerprint"):
        docker_helper.cmd_cache_key(argparse.Namespace(image="builder"))


def test_performance_summary_reports_failed_phase(tmp_path, monkeypatch):
    import json

    import docker_helper

    (tmp_path / "docker-build-report.json").write_text(
        json.dumps(
            {
                "success": False,
                "seconds": {"configure": 2, "build": 4},
                "cpm_bytes": 1048576,
                "ccache": "Hits: 12",
                "moccache": "Hits: 8",
            }
        )
    )
    summaries = []
    monkeypatch.setattr(docker_helper, "write_step_summary", summaries.append)
    docker_helper.cmd_summary(argparse.Namespace(build_dir=tmp_path))
    assert "| build | 4.00 |" in summaries[0]
    assert "CPM sources: 1.0 MiB" in summaries[0]
    assert "Hits: 12" in summaries[0]
