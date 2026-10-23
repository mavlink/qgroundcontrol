"""Tests for docker_helper.py."""

from __future__ import annotations

import argparse
import os
import subprocess
from typing import TYPE_CHECKING

import pytest
from ci.compile import docker_helper

if TYPE_CHECKING:
    from pathlib import Path


@pytest.fixture
def docker_runs(monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    calls: list[list[str]] = []

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 3)

    monkeypatch.setattr(docker_helper.subprocess, "run", run)
    return calls


def test_exec_runs_a_workspace_script_as_the_caller_with_the_build_tree(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, docker_runs: list[list[str]]
) -> None:
    monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))
    argv = ["exec", "--image", "builder:linux", "--mount-build", "--"]
    argv += ["tools/ci/security/generate_cpm_sbom.py", "--build-dir", "/project/build"]
    assert docker_helper.main(argv) == 3
    assert docker_runs == [
        [
            "docker",
            "run",
            "--rm",
            "--user",
            f"{os.getuid()}:{os.getgid()}",
            "--env",
            "HOME=/tmp",
            "--entrypoint",
            "/opt/qgc-venv/bin/python",
            "-v",
            f"{tmp_path}:/project/source:ro",
            "-v",
            f"{tmp_path / 'build'}:/project/build:ro",
            "builder:linux",
            "/project/source/tools/ci/security/generate_cpm_sbom.py",
            "--build-dir",
            "/project/build",
        ]
    ]


@pytest.mark.parametrize(
    ("mount_build", "package"),
    [(False, "/project/source/build/qgc 5.deb"), (True, "/project/build/qgc 5.deb")],
)
def test_exec_as_root_passes_host_paths_as_container_paths(
    tmp_path: Path, docker_runs: list[list[str]], mount_build: bool, package: str
) -> None:
    workspace = tmp_path / "source tree"
    argv = ["exec", "--image", "img", "--root", "--workspace", str(workspace)]
    argv += ["--mount-build"] * mount_build
    argv += ["--", "tools/ci/packaging/validate_native_package.py", "--install"]
    argv += [str(workspace / "build" / "qgc 5.deb"), str(tmp_path / "outside.deb")]
    docker_helper.main(argv)
    (command,) = docker_runs
    assert "--user" not in command
    assert command[command.index("img") + 1 :] == [
        "/project/source/tools/ci/packaging/validate_native_package.py",
        "--install",
        package,
        str(tmp_path / "outside.deb"),
    ]


def test_exec_requires_a_workspace(docker_runs: list[list[str]]) -> None:
    with pytest.raises(SystemExit):
        docker_helper.main(["exec", "--image", "img", "--", "tools/x.py"])
    assert docker_runs == []


def test_cache_key_runs_inside_the_login_environment(monkeypatch):
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
    monkeypatch.setattr(
        docker_helper.subprocess,
        "run",
        lambda command, **kwargs: subprocess.CompletedProcess(command, 0, "invalid\noutput\n"),
    )
    with pytest.raises(ValueError, match="fingerprint"):
        docker_helper.cmd_cache_key(argparse.Namespace(image="builder"))


def test_performance_summary_reports_failed_phase(tmp_path, monkeypatch):
    import json

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
    from types import SimpleNamespace

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


@pytest.mark.parametrize(("enable_fuse", "fuse"), [("true", ["--fuse"]), ("false", []), ("", [])])
def test_run_reads_fuse_from_the_environment(monkeypatch, enable_fuse, fuse):
    calls = []
    monkeypatch.setenv("ENABLE_FUSE", enable_fuse)
    monkeypatch.setattr(docker_helper, "run_with_retry", lambda command, **_: calls.append(command))
    assert docker_helper.main(["run", "--image", "sha", "--build-type", "Release"]) == 0
    assert calls[0][1:] == ["deploy/docker/run_docker.py", "run", *fuse, "sha", "Release"]


def test_summary_records_ninja_timings_before_reporting(tmp_path, monkeypatch, gh_output):
    import json

    (tmp_path / ".ninja_log").write_text(
        "# ninja log v5\n0\t4000\t0\tsrc/a.cc.o\tabc\n0\t1000\t0\tqgc\tdef\n"
    )
    summaries = []
    monkeypatch.setattr(docker_helper, "write_step_summary", summaries.append)
    assert docker_helper.main(["summary", "--build-dir", str(tmp_path)]) == 0
    # A failed container may leave no report; the timings are still kept for upload.
    assert {path.name for path in (tmp_path / "build-profile").iterdir()} == {
        "ninja.log",
        "report.json",
        "report.md",
    }
    assert summaries == []

    (tmp_path / "docker-build-report.json").write_text(
        json.dumps({"success": True, "build_success": True, "seconds": {}, "cpm_bytes": 0})
    )
    assert docker_helper.main(["summary", "--build-dir", str(tmp_path)]) == 0
    assert "| Build steps run | 2 |" in summaries[0]


def test_summary_reports_even_when_timings_cannot_be_recorded(
    tmp_path, monkeypatch, gh_output, capsys
):
    import json

    (tmp_path / ".ninja_log").write_text("not a ninja log\n")
    (tmp_path / "docker-build-report.json").write_text(
        json.dumps({"success": True, "build_success": True, "seconds": {}, "cpm_bytes": 0})
    )

    def fail(*_args, **_kwargs):
        raise ValueError("bad log")

    monkeypatch.setattr(docker_helper, "write_snapshot", fail)
    assert docker_helper.main(["summary", "--build-dir", str(tmp_path)]) == 0
    assert "::warning::Cannot write Ninja build timings: bad log" in capsys.readouterr().out
    # The cache save step reads this output.
    assert "build_success=true" in gh_output.read_text()
