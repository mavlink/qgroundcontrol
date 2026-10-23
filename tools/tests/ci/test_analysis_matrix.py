"""Tests for analysis_matrix.py."""

from __future__ import annotations

import json
import math
from typing import TYPE_CHECKING

import pytest
from ci.planning.analysis_matrix import (
    COMPILER_TOOLS,
    DISPATCH_FAMILIES,
    FULL_SCAN_INPUTS,
    MAX_TIDY_SHARDS,
    RESULT_CACHE_DIR,
    UNITS_PER_SHARD,
    build_matrix,
    main,
    may_select_full_scan,
    validate_path,
)

from .._helpers import (
    REPO_ROOT,
    commit_files,
    git,
    load_workflow,
    pr_merge_checkout,
    read_github_file,
)

if TYPE_CHECKING:
    from pathlib import Path

BUILD_CONFIG = REPO_ROOT / ".github/build-config.json"
LLVM_VERSION = json.loads(BUILD_CONFIG.read_text())["analysis"]["llvm_version"]


def _matrix(event: str, root: Path, base: str = "", **dispatch) -> list[dict]:
    return build_matrix(event, root, base, llvm_version=LLVM_VERSION, **dispatch)


@pytest.fixture
def repo(tmp_path: Path, isolated_git: None) -> tuple[Path, str]:
    """A repository where Wide.h reaches 120 sources through Middle.h; Narrow.h reaches one."""
    root = tmp_path / "repo"
    files = {
        "src/Wide.h": "#pragma once\n",
        "src/Middle.h": '#pragma once\n#include "Wide.h"\n',
        "src/Narrow.h": "#pragma once\n",
        "src/narrow.cc": '#include "Narrow.h"\n',
        "test/TestOnly.h": "#pragma once\n",
        "src/CMakeLists.txt": "\n",
        "cmake/Helpers.cmake": "\n",
    }
    files.update({f"src/wide/user{i}.cc": '#include "sub/Middle.h"\n' for i in range(120)})
    root.mkdir()
    git(root, "init", "-q")
    return root, commit_files(root, files, "base")


def _change(root: Path, *paths: str) -> None:
    for path in paths:
        (root / path).parent.mkdir(parents=True, exist_ok=True)
        with (root / path).open("a") as file:
            file.write("// changed\n")
    git(root, "add", ".")
    git(root, "commit", "-qm", "change")


def _plan(matrix: list[dict]) -> tuple[int, str | None]:
    """Return the shard count and scan scope after checking every leg runs both tools."""
    assert [leg["shard"] for leg in matrix] == list(range(1, len(matrix) + 1))
    for leg in matrix:
        assert leg["tool"] == "clang-tidy"
        assert leg["tools"] == COMPILER_TOOLS
        assert leg["family"] == "compiler"
        assert leg["shard_count"] == len(matrix)
        shard_args = ["--shard", str(leg["shard"]), "--shard-count", str(len(matrix))]
        assert leg["analyze_args"] == [
            *("--tool", "clang-tidy", "--tool", "clazy"),
            *("--advisory", "--cache-dir", RESULT_CACHE_DIR),
            *(shard_args if len(matrix) > 1 else []),
            *(["--all"] if leg["scan"] == "all" else []),
        ]
    assert len({leg["scan"] for leg in matrix}) <= 1
    return len(matrix), matrix[0]["scan"] if matrix else None


@pytest.mark.parametrize(
    ("tool", "family", "packages"),
    [
        ("clazy", "compiler", f"clang-{LLVM_VERSION} clang-tools-{LLVM_VERSION}"),
        ("clang-tidy", "compiler", f"clang-{LLVM_VERSION} clang-tools-{LLVM_VERSION}"),
        ("qmllint", "qml", f"clang-{LLVM_VERSION} clang-tools-{LLVM_VERSION}"),
        ("iwyu", "iwyu", f"clang-{LLVM_VERSION} clang-tools-{LLVM_VERSION} iwyu"),
        ("tsan", "tsan", ""),
    ],
)
def test_dispatch_runs_only_the_selected_tool(
    tmp_path: Path, tool: str, family: str, packages: str
) -> None:
    (leg,) = _matrix("workflow_dispatch", tmp_path, tool=tool)
    assert leg["tools"] == [tool]
    assert (leg["tool"], leg["family"], leg["shard"], leg["shard_count"]) == (tool, family, 1, 1)
    assert leg["extra_packages"] == packages
    assert ("analyze_args" in leg) == (family in {"compiler", "qml"})
    assert ("path" in leg) == (family == "iwyu")


def test_dispatch_offers_no_sanitizer_that_linux_already_runs(gh_output: Path) -> None:
    assert set(DISPATCH_FAMILIES) == {"clazy", "clang-tidy", "qmllint", "iwyu", "tsan"}
    with pytest.raises(SystemExit) as exc:
        main(["--event", "workflow_dispatch", "--tool", "asan"])
    assert exc.value.code == 2


@pytest.mark.parametrize(
    ("analyze_all", "path", "scan", "scope_args", "iwyu_path"),
    [
        (True, "src/Vehicle/", "all", ["--all"], "src"),
        (False, "src/Vehicle/", "path", ["src/Vehicle/"], "src/Vehicle/"),
        (
            False,
            "  src/Vehicle/Vehicle.cc \n",
            "path",
            ["src/Vehicle/Vehicle.cc"],
            "src/Vehicle/Vehicle.cc",
        ),
        (False, "", "changed", [], "src"),
    ],
)
def test_dispatch_inputs_become_the_scope_arguments(
    tmp_path: Path, analyze_all: bool, path: str, scan: str, scope_args: list[str], iwyu_path: str
) -> None:
    dispatch = {"path": path, "analyze_all": analyze_all}
    (tidy,) = _matrix("workflow_dispatch", tmp_path, tool="clang-tidy", **dispatch)
    assert tidy["scan"] == scan
    assert tidy["analyze_args"] == [
        *("--tool", "clang-tidy", "--advisory", "--cache-dir", RESULT_CACHE_DIR),
        *scope_args,
    ]
    (qml,) = _matrix("workflow_dispatch", tmp_path, tool="qmllint", **dispatch)
    assert qml["scan"] == scan
    assert qml["analyze_args"] == ["--tool", "qmllint", "--qml-build", "--advisory", *scope_args]
    (iwyu,) = _matrix("workflow_dispatch", tmp_path, tool="iwyu", **dispatch)
    assert iwyu["path"] == iwyu_path


@pytest.mark.parametrize("tool", ["clazy", "clang-tidy"])
@pytest.mark.parametrize("profile_checks", [True, False])
def test_only_explicit_clang_tidy_dispatches_profile_checks(
    tmp_path: Path, tool: str, profile_checks: bool
) -> None:
    (leg,) = _matrix("workflow_dispatch", tmp_path, tool=tool, profile_checks=profile_checks)
    expected = tool == "clang-tidy" and profile_checks
    assert ("--profile-checks" in leg["analyze_args"]) == expected


@pytest.mark.parametrize(
    "path",
    [
        "/etc",
        "../outside",
        "src/../../outside",
        "-rf",
        "--all",
        "src/a\nb.cc",
        "src/\x00",
        "src/\tx",
    ],
)
@pytest.mark.parametrize("tool", ["clang-tidy", "qmllint", "iwyu"])
def test_dispatch_paths_must_stay_in_the_checkout(tmp_path: Path, tool: str, path: str) -> None:
    with pytest.raises(ValueError, match="Analysis path must be relative"):
        _matrix("workflow_dispatch", tmp_path, tool=tool, path=path, analyze_all=False)


def test_analyze_all_ignores_the_path_input(tmp_path: Path) -> None:
    (leg,) = _matrix("workflow_dispatch", tmp_path, tool="iwyu", path="../outside")
    assert leg["path"] == "src"
    assert validate_path(" src/Vehicle/ ") == "src/Vehicle/"


def test_master_push_scans_everything_in_one_cache_seeding_job(tmp_path: Path) -> None:
    assert _plan(_matrix("push", tmp_path)) == (1, "all")


@pytest.mark.parametrize(
    ("changed", "plan"),
    [
        (("src/narrow.cc",), (1, "changed")),
        (("src/Narrow.h",), (1, "changed")),
        (("src/Wide.h",), (math.ceil(120 / UNITS_PER_SHARD), "changed")),
        (("src/CMakeLists.txt",), (MAX_TIDY_SHARDS, "all")),
        (("src/qgc_version.h.in",), (MAX_TIDY_SHARDS, "all")),
        (("src/Utilities/library.patch",), (MAX_TIDY_SHARDS, "all")),
        (("cmake/Helpers.cmake",), (MAX_TIDY_SHARDS, "all")),
        (("tools/analyze.py",), (MAX_TIDY_SHARDS, "all")),
        (("tools/analyzers/result_cache.py",), (MAX_TIDY_SHARDS, "all")),
        (("tools/generators/mavlink_enums.py",), (MAX_TIDY_SHARDS, "all")),
        ((".clang-tidy",), (MAX_TIDY_SHARDS, "all")),
        ((".github/workflows/analysis.yml",), (MAX_TIDY_SHARDS, "all")),
        # Nothing compiled changed: no analysis jobs at all.
        (("test/TestOnly.h", "src/view.qml"), (0, None)),
        (("tools/uv.lock", "tools/pyproject.toml"), (0, None)),
        (("tools/common/markdown.py", "tools/analyzers/qmllint.py", ".qmllint.ini"), (0, None)),
    ],
)
def test_pull_requests_scale_shards_to_estimated_units(repo, changed, plan) -> None:
    root, base = repo
    _change(root, *changed)
    assert _plan(_matrix("pull_request", root, base)) == plan


def test_renamed_analysis_input_still_selects_a_full_scan(repo) -> None:
    root, base = repo
    (root / "docs").mkdir()
    git(root, "mv", "cmake/Helpers.cmake", "docs/Helpers.cmake")
    git(root, "commit", "-qm", "move")
    assert _plan(_matrix("pull_request", root, base)) == (MAX_TIDY_SHARDS, "all")


def test_header_fan_out_caps_at_max_shards(repo) -> None:
    root, base = repo
    for i in range(MAX_TIDY_SHARDS * UNITS_PER_SHARD):
        (root / f"src/more/extra{i}.cc").parent.mkdir(parents=True, exist_ok=True)
        (root / f"src/more/extra{i}.cc").write_text("#include <Wide.h>\n")
    _change(root, "src/Wide.h")
    assert _plan(_matrix("pull_request", root, base)) == (MAX_TIDY_SHARDS, "changed")


def test_missing_base_scans_everything(tmp_path: Path) -> None:
    assert _plan(_matrix("pull_request", tmp_path)) == (MAX_TIDY_SHARDS, "all")


def test_unusable_base_scans_everything(repo, capsys) -> None:
    root, _ = repo
    assert _plan(_matrix("pull_request", root, "missing-ref")) == (MAX_TIDY_SHARDS, "all")
    assert "No merge base of missing-ref and HEAD" in capsys.readouterr().err


@pytest.mark.parametrize("clone", ["full", "shallow"])
def test_pr_merge_checkout_plans_only_the_pr_changes(
    tmp_path: Path, isolated_git: None, clone: str
) -> None:
    pr = pr_merge_checkout(
        tmp_path,
        base={"src/a.cc": "int a;\n", "cmake/Helpers.cmake": "\n"},
        newer_base={"cmake/Helpers.cmake": "# newer\n"},
        pr={"src/a.cc": "int a = 1;\n"},
    )
    root = getattr(pr, clone)
    # The event's base SHA predates the base branch's full-scan input change.
    assert _plan(_matrix("pull_request", root, pr.fork_point, head=pr.pr_head)) == (1, "changed")


def _trigger_paths(event: str) -> list[str]:
    return load_workflow("analysis.yml")["on"][event]["paths"]


def test_triggers_and_planner_share_one_full_scan_list() -> None:
    paths = _trigger_paths("pull_request")
    assert _trigger_paths("push") == paths
    # Push reuses the pull_request list through a YAML alias rather than a second copy.
    workflow = (REPO_ROOT / ".github/workflows/analysis.yml").read_text()
    assert workflow.count("- 'src/**'") == 1
    outside_src = {
        path.removesuffix("**") for path in paths if not path.lstrip("!").startswith("src/")
    }
    assert outside_src == set(FULL_SCAN_INPUTS)
    for path in outside_src:
        assert may_select_full_scan(path + "Example.txt" if path.endswith("/") else path), path


@pytest.mark.parametrize(
    ("path", "triggers"),
    [
        ("src/Vehicle/Vehicle.cc", True),
        ("src/Vehicle/Vehicle.h", True),
        ("src/Vehicle/CMakeLists.txt", True),
        ("src/qgc_version.h.in", True),
        ("src/Utilities/geographiclib.patch", True),
        ("src/FlyView/FlyView.qml", False),
        ("src/Settings/App.SettingsGroup.json", False),
        ("src/ui/toolbar/Images/Plan.svg", False),
        ("src/README.md", False),
    ],
)
def test_source_tree_triggers_skip_files_that_cannot_change_compilation(path, triggers) -> None:
    def matches(pattern: str) -> bool:
        if pattern == "src/**":
            return path.startswith("src/")
        return path.startswith("src/") and path.endswith(pattern.removeprefix("!src/**/*"))

    selected = False
    for pattern in _trigger_paths("pull_request"):
        if pattern.lstrip("!").startswith("src/") and matches(pattern):
            selected = not pattern.startswith("!")
    assert selected == triggers


@pytest.fixture
def build_config(monkeypatch: pytest.MonkeyPatch) -> dict[str, str]:
    monkeypatch.setenv("CONFIG_FILE", str(BUILD_CONFIG))
    return json.loads(BUILD_CONFIG.read_text())["analysis"]


def test_main_writes_include_and_toolchain_outputs(repo, gh_output: Path, build_config) -> None:
    root, base = repo
    _change(root, "src/narrow.cc")
    assert main(["--event", "pull_request", "--base", base, "--root", str(root)]) == 0
    outputs = read_github_file(gh_output)
    assert _plan(json.loads(outputs.pop("include"))) == (1, "changed")
    assert outputs == {key: str(value) for key, value in build_config.items()}


def test_main_writes_an_empty_include_when_nothing_needs_analysis(
    repo, gh_output: Path, build_config
) -> None:
    root, base = repo
    _change(root, "src/view.qml")
    assert main(["--event", "pull_request", "--base", base, "--root", str(root)]) == 0
    assert read_github_file(gh_output)["include"] == "[]"


def test_main_reads_dispatch_inputs_from_the_environment(
    tmp_path: Path, gh_output: Path, build_config, monkeypatch: pytest.MonkeyPatch
) -> None:
    for name, value in {
        "EVENT_NAME": "workflow_dispatch",
        "DISPATCH_TOOL": "clang-tidy",
        "DISPATCH_PATH": "src/Vehicle/",
        "DISPATCH_ANALYZE_ALL": "false",
        "DISPATCH_PROFILE_CHECKS": "true",
    }.items():
        monkeypatch.setenv(name, value)
    assert main(["--root", str(tmp_path)]) == 0
    (leg,) = json.loads(read_github_file(gh_output)["include"])
    assert leg["analyze_args"][-2:] == ["--profile-checks", "src/Vehicle/"]


def test_main_reports_an_invalid_dispatch_path(
    tmp_path: Path, gh_output: Path, build_config, capsys: pytest.CaptureFixture[str]
) -> None:
    argv = ["--event", "workflow_dispatch", "--tool", "iwyu", "--no-analyze-all", "--path", "/etc"]
    assert main([*argv, "--root", str(tmp_path)]) == 1
    assert capsys.readouterr().out.startswith("::error::Analysis path must be relative")
    assert gh_output.read_text() == ""
