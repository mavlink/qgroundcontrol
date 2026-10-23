"""Static checks of the analysis.yml wiring and of the arguments its planner hands to analyze.py."""

from __future__ import annotations

import json
import re
from pathlib import Path

import analyze
import pytest
import yaml
from analyze import FileCollector
from ci.planning.analysis_matrix import DISPATCH_FAMILIES, RESULT_CACHE_DIR, build_matrix

from ..._helpers import REPO_ROOT, commit_files, git, load_workflow

WORKFLOW_TEXT = (REPO_ROOT / ".github/workflows/analysis.yml").read_text()
WORKFLOW = load_workflow("analysis.yml")
ON = WORKFLOW["on"]
PLAN = WORKFLOW["jobs"]["plan"]
JOB = WORKFLOW["jobs"]["analyze"]
STEPS = {step["name"]: step for step in JOB["steps"]}
TOOLCHAIN = json.loads((REPO_ROOT / ".github/build-config.json").read_text())["analysis"]
LLVM_VERSION = TOOLCHAIN["llvm_version"]


def _dispatch(tool: str, **inputs) -> dict:
    (leg,) = build_matrix(
        "workflow_dispatch", REPO_ROOT, "", llvm_version=LLVM_VERSION, tool=tool, **inputs
    )
    return leg


def test_each_analysis_job_uses_its_planned_tools():
    assert JOB["strategy"]["fail-fast"] is False
    assert JOB["needs"] == "plan"
    assert JOB["if"] == "needs.plan.outputs.include != '[]'"
    assert JOB["strategy"]["matrix"] == {"include": "${{ fromJSON(needs.plan.outputs.include) }}"}
    assert "join(matrix.tools, ' + ')" in JOB["name"]
    assert STEPS["Run analysis"]["env"]["ANALYZE_ARGS"] == "${{ toJSON(matrix.analyze_args) }}"
    assert STEPS["Run IWYU"]["env"]["ANALYSIS_PATH"] == "${{ matrix.path }}"
    assert STEPS["Install dependencies"]["with"]["extra-packages"] == "${{ matrix.extra_packages }}"
    for name in ("Cache matching Clazy", "Build matching Clazy"):
        assert STEPS[name]["if"].startswith("contains(matrix.tools, 'clazy')")


def test_steps_select_on_the_planned_family():
    conditions = [str(step.get("if", "")) for step in JOB["steps"]]
    assert not any("fromJSON" in condition for condition in conditions)
    assert not any("matrix.tool " in condition for condition in conditions)
    used = set(re.findall(r"matrix\.family [!=]= '([a-z]+)'", "\n".join(conditions)))
    assert used == set(DISPATCH_FAMILIES.values())
    assert "if" not in STEPS["Build Setup"]
    assert "if" not in STEPS["Install dependencies"]


def test_workflow_inputs_reach_only_the_planner():
    plan_step = next(step for step in PLAN["steps"] if step.get("id") == "plan")
    assert plan_step["run"] == "python3 tools/ci/planning/analysis_matrix.py"
    assert {
        "DISPATCH_TOOL": "${{ inputs.tool }}",
        "DISPATCH_PATH": "${{ inputs.path }}",
        "DISPATCH_ANALYZE_ALL": "${{ inputs.analyze_all }}",
        "DISPATCH_PROFILE_CHECKS": "${{ inputs.profile_checks }}",
    }.items() <= plan_step["env"].items()
    assert "inputs." not in yaml.safe_dump(JOB)
    for step in JOB["steps"]:
        assert "${{" not in step.get("run", ""), step["name"]


def test_planned_arguments_reach_analyze_py_as_one_word_each():
    script = STEPS["Run analysis"]["run"]
    assert "jq -r '.[]' <<< \"$ANALYZE_ARGS\"" in script
    assert "mapfile -t args" in script
    assert (
        'python3 tools/analyze.py --build-dir "${RUNNER_TEMP}/build" --step-summary "${args[@]}"'
        in script
    )
    assert '"${GITHUB_WORKSPACE}/${ANALYSIS_PATH}"' in STEPS["Run IWYU"]["run"]


def test_analysis_findings_fail_the_job_and_iwyu_stays_advisory():
    # Both scripts pipe into tee; `shell: bash` adds pipefail (test_workflow_shells.py).
    assert STEPS["Run analysis"]["shell"] == STEPS["Run IWYU"]["shell"] == "bash"
    assert "continue-on-error" not in STEPS["Run analysis"]
    assert STEPS["Run IWYU"]["continue-on-error"] is True


def test_toolchain_comes_from_the_planner():
    # Only the Clazy cache and build read the toolchain from the plan job; analysis_tools.py
    # reads build-config.json from the full checkout.
    assert set(PLAN["outputs"]) == {"include", "llvm_version", "clazy_sha256"}
    for key in ("llvm_version", "clazy_sha256"):
        assert PLAN["outputs"][key] == f"${{{{ steps.plan.outputs.{key} }}}}"
    checkout = next(step for step in PLAN["steps"] if step["name"] == "Checkout")
    assert ".github/build-config.json" in checkout["with"]["sparse-checkout"].split()
    assert "sparse-checkout" not in STEPS["Checkout"]["with"]
    assert STEPS["Configure"]["with"] == {
        "preset": "Linux-debug",
        "extra-args": "${{ join(matrix.configure_args, ' ') }}",
    }
    assert "if" not in STEPS["Configure"]
    assert "read_config" not in WORKFLOW_TEXT


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


def test_clang_tidy_and_clazy_skip_autogen_and_llvm_development_packages():
    assert STEPS["Generate Qt analysis prerequisites"]["if"] == "matrix.family == 'iwyu'"
    assert STEPS["Generate protocol headers"]["with"]["target"] == "qgc-analysis-headers"
    for tool in DISPATCH_FAMILIES:
        assert "-dev" not in _dispatch(tool)["extra_packages"]
    assert _dispatch("iwyu")["extra_packages"].split()[-1] == "iwyu"
    build = STEPS["Build matching Clazy"]["run"]
    assert "libclang-cpp$LLVM_VERSION-dev" in build
    assert "python3 tools/setup/build_clazy.py --prefix .cache/clazy" in build
    assert "hashFiles('tools/setup/build_clazy.py')" in STEPS["Cache matching Clazy"]["with"]["key"]


def test_clang_tidy_comes_from_the_hash_pinned_wheel_ahead_of_llvm():
    # tools/tests/ci/test_analysis_tools.py covers the pinning and PATH order the script applies.
    assert TOOLCHAIN["clang_tidy_version"].startswith(("22.", "23."))
    step = STEPS["Select analysis executables"]
    assert step["run"] == "python3 tools/ci/runner/analysis_tools.py"
    # The LLVM and clang-tidy pins come from build-config.json (common.build_config).
    assert step["env"] == {"ANALYSIS_TOOLS": "${{ join(matrix.tools, ' ') }}"}
    names = [each["name"] for each in JOB["steps"]]
    # Build Setup provides tools/.venv and uv; the Clazy build provides clazy-standalone.
    assert names.index("Build Setup") < names.index("Select analysis executables")
    assert names.index("Build matching Clazy") < names.index("Select analysis executables")


def test_pull_requests_get_asan_and_ubsan_from_linux_and_tsan_stays_manual():
    options = ON["workflow_dispatch"]["inputs"]["tool"]["options"]
    assert set(options) == set(DISPATCH_FAMILIES)
    assert not re.search(r"QGC_ENABLE_[AU]B?SAN|[ALU]B?SAN_OPTIONS|'[au]b?san'", WORKFLOW_TEXT)
    assert _dispatch("tsan")["configure_args"] == [
        "-DQGC_ENABLE_COVERAGE=OFF",
        "-DQGC_ENABLE_TSAN=ON",
    ]
    # cmake/QGCTest.cmake sets TSAN_OPTIONS on every test; the workflow must not shadow it.
    assert "env" not in STEPS["Run Sanitizer Tests"]
    # linux.yml's debug-validation job runs ASan+UBSan, so analysis.yml runs only TSan.
    assert "(ASan+UBSan)" in load_workflow("linux.yml")["jobs"]["debug-validation"]["name"]


def test_result_cache_is_restored_everywhere_but_saved_only_by_master_pushes():
    restore = STEPS["Restore analysis results"]
    save = STEPS["Save analysis results"]
    assert restore["if"] == "matrix.family == 'compiler'"
    assert restore["uses"].startswith("actions/cache/restore@")
    assert save["uses"].startswith("actions/cache/save@")
    assert "steps.analysis.outcome != 'skipped'" in save["if"]
    assert "github.event_name == 'push'" in save["if"]
    assert "schedule" not in save["if"]
    assert restore["with"]["path"] == save["with"]["path"] == RESULT_CACHE_DIR
    assert save["with"]["key"].startswith(restore["with"]["restore-keys"])
    for leg in build_matrix("push", REPO_ROOT, "", llvm_version=LLVM_VERSION):
        args = leg["analyze_args"]
        assert isinstance(args, list)
        assert args[args.index("--cache-dir") + 1] == RESULT_CACHE_DIR
    assert ON["push"]["branches"] == ["master"]
    assert "schedule" not in ON
    concurrency = WORKFLOW["concurrency"]["cancel-in-progress"]
    assert "github.event_name != 'push'" in concurrency


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


def test_planned_matrix_runs_both_tools_on_every_shard_and_saves_one_build_cache():
    plan_step = next(step for step in PLAN["steps"] if step.get("id") == "plan")
    assert plan_step["env"]["PR_BASE_SHA"] == "${{ github.event.pull_request.base.sha }}"
    assert plan_step["env"]["PR_HEAD_SHA"] == "${{ github.event.pull_request.head.sha }}"
    # A merge checkout and its parents name a PR's diff; a dispatched "changed" scan needs the
    # default branch's history.
    checkout = next(step for step in PLAN["steps"] if step["name"] == "Checkout")
    assert checkout["with"]["fetch-depth"] == "${{ github.event_name == 'pull_request' && 2 || 1 }}"
    assert STEPS["Checkout"]["with"]["fetch-depth"] == (
        "${{ github.event_name == 'pull_request' && 2 || 0 }}"
    )
    assert PLAN["outputs"]["include"] == "${{ steps.plan.outputs.include }}"
    legs = build_matrix("pull_request", REPO_ROOT, "", llvm_version=LLVM_VERSION)
    assert len(legs) == 4
    assert all(leg["tools"] == ["clang-tidy", "clazy"] for leg in legs)
    for name in ("PR_BASE_SHA", "PR_HEAD_SHA"):
        assert STEPS["Run analysis"]["env"][name] == plan_step["env"][name]
    assert STEPS["Build Setup"]["with"]["save-cache"] == "${{ matrix.shard == 1 }}"
    assert "matrix.shard" in STEPS["Upload Output"]["with"]["name"]
