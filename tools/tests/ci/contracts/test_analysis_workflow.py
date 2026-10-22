"""Static checks of the analysis.yml wiring and of the arguments its planner hands to analyze.py."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import analyze
import pytest
import yaml
from analyze import FileCollector
from ci.planning.analysis_matrix import DISPATCH_FAMILIES, RESULT_CACHE_DIR, build_matrix

from ..._helpers import REPO_ROOT, git, load_workflow

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
    assert 'python3 tools/analyze.py --build-dir "${RUNNER_TEMP}/build" "${args[@]}"' in script
    assert '"${GITHUB_WORKSPACE}/${ANALYSIS_PATH}"' in STEPS["Run IWYU"]["run"]


def test_toolchain_comes_from_the_planner():
    for key in ("llvm_version", "clazy_sha256", "clang_tidy_version", "clang_tidy_sha256"):
        assert PLAN["outputs"][key] == f"${{{{ steps.plan.outputs.{key} }}}}"
    checkout = next(step for step in PLAN["steps"] if step["name"] == "Checkout")
    assert ".github/build-config.json" in checkout["with"]["sparse-checkout"].split()
    assert "needs.plan.outputs.llvm_version" in STEPS["Configure (analysis)"]["with"]["extra-args"]
    assert "read_config" not in WORKFLOW_TEXT


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
    assert TOOLCHAIN["clang_tidy_version"].startswith(("22.", "23."))
    step = STEPS["Select analysis executables"]
    assert "--require-hashes" in step["run"]
    assert "--hash=sha256:%s" in step["run"]
    assert step["run"].index("/usr/lib/llvm-") < step["run"].index('echo "$tidy_bin"')


def test_pull_requests_get_asan_and_ubsan_from_linux_and_tsan_stays_manual():
    options = ON["workflow_dispatch"]["inputs"]["tool"]["options"]
    assert set(options) == set(DISPATCH_FAMILIES)
    assert not re.search(r"QGC_ENABLE_[AU]B?SAN|[ALU]B?SAN_OPTIONS|'[au]b?san'", WORKFLOW_TEXT)
    assert STEPS["Configure (tsan)"]["with"]["extra-args"].split() == [
        "-DQGC_ENABLE_COVERAGE=OFF",
        "-DQGC_ENABLE_TSAN=ON",
    ]
    # cmake/QGCTest.cmake sets TSAN_OPTIONS on every test; the workflow must not shadow it.
    assert "env" not in STEPS["Run Sanitizer Tests"]
    linux = (REPO_ROOT / ".github/workflows/linux.yml").read_text()
    assert "(ASan+UBSan)" in linux


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


def _parse(monkeypatch: pytest.MonkeyPatch, leg: dict):
    monkeypatch.setattr(sys, "argv", ["analyze.py", "--build-dir", "build", *leg["analyze_args"]])
    return analyze.parse_args()


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
def test_planned_arguments_are_accepted_by_analyze_py(monkeypatch, leg, expected):
    event = leg.pop("event", "workflow_dispatch")
    legs = build_matrix(event, REPO_ROOT, "", llvm_version=LLVM_VERSION, **leg)
    for planned in legs:
        args = _parse(monkeypatch, planned)
        assert args.advisory
        assert args.build_dir == "build"
        assert {key: getattr(args, key) for key in expected} == expected
        if planned["family"] == "compiler":
            assert args.cache_dir == Path(RESULT_CACHE_DIR)
            assert args.shard == planned["shard"]


def test_changed_scope_selects_only_the_pull_request_sources(git_repo, monkeypatch):
    (git_repo / "src").mkdir()
    (git_repo / "src/changed.cc").write_text("int value = 1;\n")
    (git_repo / "src/unchanged.cc").write_text("int other = 2;\n")
    git(git_repo, "add", ".")
    git(git_repo, "commit", "-qm", "base")
    base = git(git_repo, "rev-parse", "HEAD")
    (git_repo / "src/changed.cc").write_text("int value = 3;\n")
    git(git_repo, "commit", "-qam", "change")

    (leg,) = build_matrix("pull_request", git_repo, base, llvm_version=LLVM_VERSION)
    assert leg["scan"] == "changed"
    args = _parse(monkeypatch, leg)
    assert not args.all
    assert args.path == []
    monkeypatch.setenv("PR_BASE_SHA", base)
    selected = FileCollector(git_repo).get_cpp_files(analyze_all=False)
    assert {path.name for path in selected} == {"changed.cc"}


def test_planned_matrix_runs_both_tools_on_every_shard_and_saves_one_build_cache():
    plan_step = next(step for step in PLAN["steps"] if step.get("id") == "plan")
    assert plan_step["env"]["PR_BASE_SHA"] == "${{ github.event.pull_request.base.sha }}"
    checkout = next(step for step in PLAN["steps"] if step["name"] == "Checkout")
    assert checkout["with"]["fetch-depth"] == "${{ github.event_name != 'pull_request' && 1 || 0 }}"
    assert PLAN["outputs"]["include"] == "${{ steps.plan.outputs.include }}"
    legs = build_matrix("pull_request", REPO_ROOT, "", llvm_version=LLVM_VERSION)
    assert len(legs) == 4
    assert all(leg["tools"] == ["clang-tidy", "clazy"] for leg in legs)
    assert STEPS["Run analysis"]["env"]["PR_BASE_SHA"] == plan_step["env"]["PR_BASE_SHA"]
    assert STEPS["Build Setup"]["with"]["save-cache"] == "${{ matrix.shard == 1 }}"
    assert "matrix.shard" in STEPS["Upload Output"]["with"]["name"]
