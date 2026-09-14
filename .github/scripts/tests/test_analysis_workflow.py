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
        GITHUB_EVENT_NAME="pull_request",
        ANALYSIS_TOOL="clazy",
        ANALYZE_ALL="false",
        INPUT_PATH="",
        PR_BASE_SHA=base,
        ANALYSIS_SHARD="1",
        ANALYSIS_SHARD_COUNT="1",
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
def test_pr_analysis_selects_full_scan_for_configuration_changes(
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
    full_scan = changed_path not in {
        "src/changed.cc",
        "tools/analyzers/qmllint.py",
        "tools/common/markdown.py",
        *(
            {".clang-tidy", "tools/analyzers/clang_tidy.py"}
            if tool == "clazy"
            else {"tools/analyzers/clazy.py"}
        ),
    }
    assert all(("--all" in args) == full_scan for args in invocations)
    monkeypatch.setenv("PR_BASE_SHA", env["PR_BASE_SHA"])
    selected = FileCollector(root).get_cpp_files(analyze_all=full_scan)
    expected = {"changed.cc", "unchanged.cc"} if full_scan else set()
    if changed_path == "src/changed.cc":
        expected = {"changed.cc"}
    assert {path.name for path in selected} == expected


def test_invalid_diff_stops_analysis_instead_of_skipping(checkout):
    root, env = checkout
    env["PR_BASE_SHA"] = "missing-ref"
    result = subprocess.run(
        ["bash", "-e", "-o", "pipefail", "-c", SCRIPT],
        cwd=root,
        env=env,
        check=False,
        capture_output=True,
    )
    assert result.returncode != 0
    assert not (root / "args.jsonl").exists()


@pytest.mark.parametrize("tool", ["clazy", "clang-tidy"])
@pytest.mark.parametrize("profile_checks", ["true", "false"])
def test_workflow_forwards_shards_and_only_explicit_tidy_profiling(checkout, tool, profile_checks):
    root, env = checkout
    env.update(
        ANALYSIS_TOOL=tool,
        PROFILE_CHECKS=profile_checks,
        ANALYSIS_SHARD="3",
        ANALYSIS_SHARD_COUNT="4",
    )
    subprocess.run(["bash", "-e", "-o", "pipefail", "-c", SCRIPT], cwd=root, env=env, check=True)
    args = json.loads((root / "args.jsonl").read_text())
    assert args[args.index("--shard") + 1] == "3"
    assert args[args.index("--shard-count") + 1] == "4"
    assert ("--profile-checks" in args) == (tool == "clang-tidy" and profile_checks == "true")


def test_automatic_matrix_partitions_only_tidy_and_saves_one_cache():
    job = WORKFLOW["jobs"]["analyze"]
    matrix = job["strategy"]["matrix"]
    assert "'[1,2,3,4]'" in matrix["shard"]
    assert "github.event_name == 'workflow_dispatch' && '[1]'" in matrix["shard"]
    legs = [
        {"tool": tool, "shard": shard}
        for tool in ("clazy", "clang-tidy")
        for shard in range(1, 5)
        if {"tool": tool, "shard": shard} not in matrix["exclude"]
    ]
    assert sum(leg["tool"] == "clazy" for leg in legs) == 1
    assert sum(leg["tool"] == "clang-tidy" for leg in legs) == 4
    setup = next(step for step in job["steps"] if step["name"] == "Build Setup (clazy/iwyu)")
    assert setup["with"]["save-cache"] == "${{ matrix.shard == 1 }}"
    upload = next(step for step in job["steps"] if step["name"] == "Upload Output")
    assert "matrix.shard" in upload["with"]["name"]
