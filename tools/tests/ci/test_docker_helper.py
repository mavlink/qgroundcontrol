"""Tests for docker_helper.py."""

from __future__ import annotations

import argparse

import pytest
from ci.compile.docker_helper import resolve_push_target


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

    from ci.compile import docker_helper

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

    from ci.compile import docker_helper

    monkeypatch.setattr(
        docker_helper.subprocess,
        "run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 0, "invalid\noutput\n"),
    )
    with pytest.raises(ValueError, match="fingerprint"):
        docker_helper.cmd_cache_key(argparse.Namespace(image="builder"))


def test_performance_summary_reports_failed_phase(tmp_path, monkeypatch):
    import json

    from ci.compile import docker_helper

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


@pytest.mark.parametrize(("failures", "exit_code"), [(1, 0), (2, 1)])
def test_run_retries_the_build_once(monkeypatch, capsys, failures, exit_code):
    import subprocess
    from types import SimpleNamespace

    from ci.compile import docker_helper

    calls = []
    sleeps = []

    def run(command, **kwargs):
        calls.append(command)
        if len(calls) <= failures:
            raise subprocess.CalledProcessError(1, command)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr("common.proc.subprocess.run", run)
    monkeypatch.setattr("common.proc.time", SimpleNamespace(sleep=sleeps.append))
    args = argparse.Namespace(fuse=True, image="builder:linux", build_type="Release")
    assert docker_helper.cmd_run(args) == exit_code
    if exit_code:
        assert "::error::Docker build failed after 2 attempts." in capsys.readouterr().out
    assert len(calls) == 2
    assert calls[0][1:] == [
        "deploy/docker/run_docker.py",
        "run",
        "--fuse",
        "builder:linux",
        "Release",
    ]
    assert sleeps == [30]
