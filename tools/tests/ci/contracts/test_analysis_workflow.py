"""analysis.yml and its planner agree on tool families, dispatch inputs, arguments and the result cache."""

from __future__ import annotations

import json
import re
from pathlib import Path

import analyze
import pytest
from analyze import FileCollector
from ci.planning.analysis_matrix import DISPATCH_FAMILIES, RESULT_CACHE_DIR, build_matrix

from ..._helpers import REPO_ROOT, commit_files, get_step, git, load_workflow
from .._workflows import uses_action

WORKFLOW = load_workflow("analysis.yml")
JOB = WORKFLOW["jobs"]["analyze"]
PLANNER = REPO_ROOT / "tools/ci/planning/analysis_matrix.py"
LLVM_VERSION = json.loads((REPO_ROOT / ".github/build-config.json").read_text())["analysis"][
    "llvm_version"
]


def test_steps_select_on_the_families_the_planner_emits():
    conditions = "\n".join(str(step.get("if", "")) for step in JOB["steps"])
    used = set(re.findall(r"matrix\.family [!=]= '([a-z]+)'", conditions))
    assert used == set(DISPATCH_FAMILIES.values())


def test_dispatch_offers_exactly_the_tools_the_planner_accepts():
    options = WORKFLOW["on"]["workflow_dispatch"]["inputs"]["tool"]["options"]
    assert set(options) == set(DISPATCH_FAMILIES)


def test_plan_step_sets_exactly_the_environment_the_planner_reads():
    plan = get_step(WORKFLOW["jobs"]["plan"]["steps"], "plan")
    read = set(re.findall(r'os\.environ\.get\("(\w+)"', PLANNER.read_text(encoding="utf-8")))
    assert set(plan["env"]) == read


def test_result_cache_steps_use_the_directory_the_planner_passes_to_analyze_py():
    (restore,) = (step for step in JOB["steps"] if uses_action(step, "actions/cache/restore"))
    (save,) = (step for step in JOB["steps"] if uses_action(step, "actions/cache/save"))
    assert restore["with"]["path"] == save["with"]["path"] == RESULT_CACHE_DIR
    # A key outside the restore prefix would be saved and never restored.
    assert save["with"]["key"].startswith(restore["with"]["restore-keys"])


ANALYSIS_CONFIGURE = [
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DQGC_BUILD_TESTING=OFF",
    "-DQGC_ENABLE_WERROR=OFF",
    f"-DCMAKE_C_COMPILER=clang-{LLVM_VERSION}",
    f"-DCMAKE_CXX_COMPILER=clang++-{LLVM_VERSION}",
    "-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON",
]
AUTOGEN_TARGET = ["-DCMAKE_AUTOGEN_ORIGIN_DEPENDS=OFF", "-DCMAKE_GLOBAL_AUTOGEN_TARGET=ON"]


@pytest.mark.parametrize(
    ("leg", "expected"),
    [
        (
            {"event": "pull_request"},
            [*ANALYSIS_CONFIGURE, *AUTOGEN_TARGET, "-DQGC_UNITY_BUILD=OFF"],
        ),
        ({"event": "push"}, [*ANALYSIS_CONFIGURE, *AUTOGEN_TARGET, "-DQGC_UNITY_BUILD=OFF"]),
        ({"tool": "clazy"}, [*ANALYSIS_CONFIGURE, *AUTOGEN_TARGET, "-DQGC_UNITY_BUILD=OFF"]),
        ({"tool": "clang-tidy"}, [*ANALYSIS_CONFIGURE, *AUTOGEN_TARGET, "-DQGC_UNITY_BUILD=OFF"]),
        ({"tool": "iwyu"}, [*ANALYSIS_CONFIGURE, *AUTOGEN_TARGET, "-DQGC_UNITY_BUILD=OFF"]),
        ({"tool": "qmllint"}, [*ANALYSIS_CONFIGURE, "-DQGC_UNITY_BUILD=OFF"]),
        ({"tool": "tsan"}, ["-DQGC_ENABLE_COVERAGE=OFF", "-DQGC_ENABLE_TSAN=ON"]),
    ],
)
def test_every_family_gets_its_configure_arguments(leg, expected):
    event = leg.pop("event", "workflow_dispatch")
    for planned in build_matrix(event, REPO_ROOT, "", llvm_version=LLVM_VERSION, **leg):
        args = planned["configure_args"]
        assert isinstance(args, list)
        assert args == expected
        # cmake_helper.py splits extra-args on whitespace, so each argument must be one word.
        assert all(arg.split() == [arg] for arg in args)


def _parse(leg: dict):
    return analyze.parse_args(["--build-dir", "build", *leg["analyze_args"]])


@pytest.mark.parametrize(
    ("leg", "expected"),
    [
        (
            {"event": "pull_request"},
            {"tools": ["clang-tidy", "clazy"], "all": True, "path": [], "shard_count": 4},
        ),
        ({"event": "push"}, {"tools": ["clang-tidy", "clazy"], "all": True, "shard_count": 1}),
        (
            {"tool": "clang-tidy", "path": "src/Vehicle/", "analyze_all": False},
            {"tools": ["clang-tidy"], "all": False, "path": ["src/Vehicle/"]},
        ),
        (
            {"tool": "clang-tidy", "profile_checks": True, "analyze_all": False},
            {"tools": ["clang-tidy"], "all": False, "path": [], "profile_checks": True},
        ),
        ({"tool": "clazy"}, {"tools": ["clazy"], "all": True, "profile_checks": False}),
        (
            {"tool": "qmllint", "path": "src/FlyView/", "analyze_all": False},
            {"tools": ["qmllint"], "path": ["src/FlyView/"], "qml_build": True, "cache_dir": None},
        ),
    ],
)
def test_planned_arguments_are_accepted_by_analyze_py(leg, expected):
    event = leg.pop("event", "workflow_dispatch")
    legs = build_matrix(event, REPO_ROOT, "", llvm_version=LLVM_VERSION, **leg)
    for planned in legs:
        args = _parse(planned)
        assert args.advisory
        assert args.build_dir == "build"
        assert {key: getattr(args, key) for key in expected} == expected
        if planned["family"] == "compiler":
            assert args.cache_dir == Path(RESULT_CACHE_DIR)
            assert args.shard == planned["shard"]


def test_changed_scope_selects_only_the_pull_request_sources(git_repo, monkeypatch):
    files = {"src/changed.cc": "int value = 1;\n", "src/unchanged.cc": "int other = 2;\n"}
    base = commit_files(git_repo, files, "base")
    (git_repo / "src/changed.cc").write_text("int value = 3;\n")
    git(git_repo, "commit", "-qam", "change")

    (leg,) = build_matrix("pull_request", git_repo, base, llvm_version=LLVM_VERSION)
    assert leg["scan"] == "changed"
    args = _parse(leg)
    assert not args.all
    assert args.path == []
    monkeypatch.setenv("PR_BASE_SHA", base)
    selected = FileCollector(git_repo).get_cpp_files(analyze_all=False)
    assert {path.name for path in selected} == {"changed.cc"}
