"""Tests for docker_helper.py."""

from __future__ import annotations

import argparse

import pytest


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
    assert summaries[0].startswith("### Docker build\n\n| Item | Value |")
    assert "| Result | failed |" in summaries[0]
    assert "| Build | 4.0s |" in summaries[0]
    assert "| CPM sources | 1.0 MiB |" in summaries[0]
    assert "<details><summary>ccache statistics</summary>" in summaries[0]
    assert "Hits: 12" in summaries[0]


def test_performance_summary_reads_container_hit_rates_and_timings(tmp_path, monkeypatch):
    import json

    from ci.compile import docker_helper

    (tmp_path / "build-profile").mkdir()
    (tmp_path / "build-profile" / "report.json").write_text(
        json.dumps(
            {
                "edge_count": 3,
                "categories": {"compile": {"count": 3, "duration_ms": 9000}},
                "slowest_edges": [{"output": "a.o", "duration_ms": 5000, "category": "compile"}],
            }
        )
    )
    (tmp_path / "docker-build-report.json").write_text(
        json.dumps(
            {
                "success": True,
                "build_success": True,
                "seconds": {"configure": 62.4, "build": 730.2, "install": 45},
                "cpm_bytes": 512 * 1048576,
                "ccache": "Cacheable calls: 1934 / 2000 (96.70%)\n  Hits: 1900 / 1934 (98.24%)\n",
                "moccache": "moccache stats (this build, /c):\n  hits     812\n  misses   8\n",
            }
        )
    )
    summaries = []
    monkeypatch.setattr(docker_helper, "write_step_summary", summaries.append)
    docker_helper.cmd_summary(argparse.Namespace(build_dir=tmp_path))
    section = summaries[0]
    assert "| Result | passed |" in section
    assert "| Configure | 1m 02s |" in section
    assert "| Build | 12m 10s |" in section
    assert "| Compiler cache | ccache: 1,900 of 1,934 (98.2%) hits |" in section
    assert "| moc cache | 812 of 820 (99.0%) hits |" in section
    assert "| Build steps run | 3 |" in section
    assert "<details><summary>Slowest build steps (top 10)</summary>" in section


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
