"""Exercise compiler-analysis selection using the workflow shell and real Git diffs."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
import yaml
from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from analyze import FileCollector

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = yaml.safe_load((ROOT / ".github/workflows/analysis.yml").read_text())
SCRIPT = next(
    step["run"]
    for step in WORKFLOW["jobs"]["analyze"]["steps"]
    if step["name"] == "Run compiler analysis"
)


def test_each_analysis_job_uses_its_matrix_tool():
    job = WORKFLOW["jobs"]["analyze"]
    assert job["strategy"]["fail-fast"] is False
    assert 'fromJSON(\'["clazy", "clang-tidy"]\')' in job["strategy"]["matrix"]["tool"]
    for step in job["steps"]:
        if "ANALYSIS_TOOL" in step.get("env", {}):
            assert step["env"]["ANALYSIS_TOOL"] == "${{ matrix.tool }}"


pytestmark = pytest.mark.skipif(
    os.name == "nt" or not shutil.which("bash") or not shutil.which("git"),
    reason="The analysis workflow runs with Bash and Git on Linux",
)


@pytest.fixture
def checkout(tmp_path):
    subprocess.run(["git", "init", "-q", str(tmp_path)], check=True)
    source = tmp_path / "src"
    source.mkdir()
    (source / "changed.cc").write_text("int value = 1;\n")
    (source / "unchanged.cc").write_text("int other = 2;\n")
    subprocess.run(["git", "add", "."], cwd=tmp_path, check=True)
    subprocess.run(
        [
            "git",
            "-c",
            "user.name=Test",
            "-c",
            "user.email=test@example.invalid",
            "commit",
            "-qm",
            "base",
        ],
        cwd=tmp_path,
        check=True,
    )
    base = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=tmp_path, text=True).strip()
    binary = tmp_path / "bin"
    binary.mkdir()
    python = binary / "python3"
    python.write_text(
        f"#!{sys.executable}\n"
        "import json, os, sys\n"
        "with open(os.environ['ARGS_LOG'], 'a') as log:\n"
        "    log.write(json.dumps(sys.argv[1:]) + '\\n')\n"
    )
    python.chmod(0o755)
    (tmp_path / "build").mkdir()
    env = dict(
        os.environ,
        PATH=f"{binary}{os.pathsep}{os.environ['PATH']}",
        ARGS_LOG=str(tmp_path / "args.jsonl"),
        RUNNER_TEMP=str(tmp_path),
        GITHUB_WORKSPACE=str(tmp_path),
        GITHUB_EVENT_NAME="pull_request",
        ANALYSIS_TOOL="clazy",
        ANALYZE_ALL="false",
        INPUT_PATH="",
        PR_BASE_SHA=base,
        ANALYSIS_JOBS="16",
        PROFILE_CHECKS="false",
    )
    return tmp_path, env


@pytest.mark.parametrize(
    "changed_path",
    [
        ".clang-tidy",
        "tools/analyzers/clang_tidy.py",
        "tools/analyzers/clazy.py",
        "tools/analyzers/qmllint.py",
        "tools/common/markdown.py",
        "tools/analyzers/compiler.py",
        "tools/analyze.py",
        "tools/common/analyzer.py",
        "tools/pyproject.toml",
        "tools/uv.lock",
        ".github/workflows/analysis.yml",
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/modules/Example.cmake",
        "src/Example/CMakeLists.txt",
        "src/changed.cc",
    ],
)
@pytest.mark.parametrize("tool", ["clazy", "clang-tidy"])
def test_pr_analysis_stays_scoped_to_changed_code_for_configuration_changes(
    checkout, changed_path, monkeypatch, tool
):
    root, env = checkout
    env["ANALYSIS_TOOL"] = tool
    changed = root / changed_path
    changed.parent.mkdir(parents=True, exist_ok=True)
    changed.write_text("changed\n")
    subprocess.run(["git", "add", changed_path], cwd=root, check=True)
    subprocess.run(
        [
            "git",
            "-c",
            "user.name=Test",
            "-c",
            "user.email=test@example.invalid",
            "commit",
            "-qm",
            "change",
        ],
        cwd=root,
        check=True,
    )
    subprocess.run(["bash", "-e", "-o", "pipefail", "-c", SCRIPT], cwd=root, env=env, check=True)
    invocations = [json.loads(line) for line in (root / "args.jsonl").read_text().splitlines()]
    assert len(invocations) == 1
    assert invocations[0][2] == env["ANALYSIS_TOOL"]
    args = invocations[0]
    assert "--all" not in args
    assert args[args.index("--diff-base") + 1] == env["PR_BASE_SHA"]
    assert args[args.index("--review-output") + 1] == str(root / "analysis-review/report.json")
    monkeypatch.setenv("PR_BASE_SHA", env["PR_BASE_SHA"])
    selected = FileCollector(root).get_cpp_files()
    expected = {"changed.cc"} if changed_path == "src/changed.cc" else set()
    assert {path.name for path in selected} == expected


@pytest.mark.parametrize("tool", ["clazy", "clang-tidy"])
@pytest.mark.parametrize("profile_checks", ["true", "false"])
@pytest.mark.parametrize("jobs", ["4", "16"])
def test_workflow_forwards_workers_and_only_explicit_tidy_profiling(
    checkout, tool, profile_checks, jobs
):
    root, env = checkout
    env.update(
        ANALYSIS_TOOL=tool,
        PROFILE_CHECKS=profile_checks,
        ANALYSIS_JOBS=jobs,
    )
    subprocess.run(["bash", "-e", "-o", "pipefail", "-c", SCRIPT], cwd=root, env=env, check=True)
    args = json.loads((root / "args.jsonl").read_text())
    assert args[args.index("--jobs") + 1] == jobs
    assert "--shard" not in args
    assert "--shard-count" not in args
    assert ("--profile-checks" in args) == (tool == "clang-tidy" and profile_checks == "true")


def test_automatic_matrix_runs_one_job_per_tool():
    job = WORKFLOW["jobs"]["analyze"]
    matrix = job["strategy"]["matrix"]
    assert set(matrix) == {"tool"}
    assert "matrix.shard" not in job["name"]
    assert "matrix.tool == 'clang-tidy'" in job["runs-on"]
    for label in ("cpu=16", "ram=64", "spot=false", "extras=s3-cache", "runner=linux-x64-tester"):
        assert label in job["runs-on"]
    setup = next(step for step in job["steps"] if step["name"] == "Build Setup (clazy/iwyu)")
    assert setup["with"]["save-cache"] == "true"
    upload = next(step for step in job["steps"] if step["name"] == "Upload Output")
    assert upload["with"]["name"] == "${{ matrix.tool }}-output"


@pytest.mark.parametrize("jobs", ["4", "16"])
def test_workflow_detects_and_forwards_available_cpus(checkout, jobs):
    root, env = checkout
    nproc = root / "bin/nproc"
    nproc.write_text(f"#!/bin/sh\nprintf '%s\\n' '{jobs}'\n")
    nproc.chmod(0o755)
    output = root / "outputs"
    env["GITHUB_OUTPUT"] = str(output)
    steps = WORKFLOW["jobs"]["analyze"]["steps"]
    parallel = next(step for step in steps if step.get("id") == "parallel")
    subprocess.run(
        ["bash", "-e", "-o", "pipefail", "-c", parallel["run"]],
        cwd=root,
        env=env,
        check=True,
    )
    assert output.read_text().strip() == f"jobs={jobs}"
    for step in steps:
        if step.get("uses") == "./.github/actions/cmake-build":
            assert step["with"]["parallel-jobs"] == "${{ steps.parallel.outputs.jobs }}"
        if step["name"] in ("Run compiler analysis", "Run IWYU", "Build matching Clazy"):
            assert step["env"]["ANALYSIS_JOBS"] == "${{ steps.parallel.outputs.jobs }}"


@pytest.mark.parametrize("path", ["", "src/"])
def test_manual_analysis_preserves_full_scan_and_path_modes(checkout, path):
    root, env = checkout
    env.update(PR_BASE_SHA="", INPUT_PATH=path, ANALYZE_ALL="false" if path else "true")
    subprocess.run(["bash", "-e", "-o", "pipefail", "-c", SCRIPT], cwd=root, env=env, check=True)
    args = json.loads((root / "args.jsonl").read_text())
    assert "--diff-base" not in args
    assert "--review-output" not in args
    assert ("--all" in args) is (not path)
    if path:
        assert path in args


@pytest.mark.parametrize("exit_code", [1, 2])
def test_analyzer_failures_are_not_hidden_by_tee(checkout, exit_code):
    root, env = checkout
    assert WORKFLOW["jobs"]["analyze"]["defaults"]["run"]["shell"] == "bash"
    (root / "bin/python3").write_text(
        f"#!{sys.executable}\nimport sys\nprint('analysis failed')\nsys.exit({exit_code})\n"
    )
    result = subprocess.run(
        ["bash", "--noprofile", "--norc", "-e", "-o", "pipefail", "-c", SCRIPT],
        cwd=root,
        env=env,
        check=False,
        capture_output=True,
        text=True,
    )
    assert result.returncode == exit_code
    assert "analysis failed" in (root / "build/analysis-output.txt").read_text()
