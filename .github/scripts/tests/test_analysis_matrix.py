"""Tests for analysis_matrix.py."""

from __future__ import annotations

import json
import math
import subprocess
from typing import TYPE_CHECKING

import pytest
import yaml
from _helpers import REPO_ROOT
from analysis_matrix import (
    MAX_TIDY_SHARDS,
    UNITS_PER_SHARD,
    build_matrix,
    main,
    may_select_full_scan,
)

if TYPE_CHECKING:
    from pathlib import Path


def _git(root: Path, *args: str) -> str:
    return subprocess.run(
        ["git", "-c", "user.name=Test", "-c", "user.email=test@example.invalid", *args],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


@pytest.fixture
def repo(tmp_path: Path) -> tuple[Path, str]:
    """A repository where Wide.h reaches 120 sources through Middle.h; Narrow.h reaches one."""
    root = tmp_path / "repo"
    files = {
        "src/Wide.h": "#pragma once\n",
        "src/Middle.h": '#pragma once\n#include "Wide.h"\n',
        "src/Narrow.h": "#pragma once\n",
        "src/narrow.cc": '#include "Narrow.h"\n',
        "test/TestOnly.h": "#pragma once\n",
        "src/CMakeLists.txt": "\n",
    }
    files.update({f"src/wide/user{i}.cc": '#include "sub/Middle.h"\n' for i in range(120)})
    for name, text in files.items():
        (root / name).parent.mkdir(parents=True, exist_ok=True)
        (root / name).write_text(text)
    _git(root.parent, "init", "-q", str(root))
    _git(root, "add", ".")
    _git(root, "commit", "-qm", "base")
    return root, _git(root, "rev-parse", "HEAD")


def _change(root: Path, *paths: str) -> None:
    for path in paths:
        (root / path).parent.mkdir(parents=True, exist_ok=True)
        with (root / path).open("a") as file:
            file.write("// changed\n")
    _git(root, "add", ".")
    _git(root, "commit", "-qm", "change")


def _tidy_counts(matrix: list[dict[str, str | int]]) -> set[int | str]:
    tidy = [leg for leg in matrix if leg["tool"] == "clang-tidy"]
    assert [leg["shard"] for leg in tidy] == list(range(1, len(tidy) + 1))
    return {leg["shard_count"] for leg in tidy}


def test_dispatch_runs_only_the_selected_tool(tmp_path: Path) -> None:
    assert build_matrix("workflow_dispatch", "asan", tmp_path, "") == [
        {"tool": "asan", "shard": 1, "shard_count": 1}
    ]


@pytest.mark.parametrize(
    ("changed", "count"),
    [
        (("src/narrow.cc",), 1),
        (("src/Narrow.h",), 1),
        (("test/TestOnly.h", "src/view.qml"), 1),
        (("src/Wide.h",), math.ceil(120 / UNITS_PER_SHARD)),
        (("src/CMakeLists.txt",), MAX_TIDY_SHARDS),
        (("cmake/Helpers.cmake",), MAX_TIDY_SHARDS),
        (("tools/analyze.py",), MAX_TIDY_SHARDS),
        ((".clang-tidy",), MAX_TIDY_SHARDS),
        ((".github/workflows/analysis.yml",), MAX_TIDY_SHARDS),
    ],
)
def test_pull_requests_scale_tidy_shards_to_estimated_units(repo, changed, count) -> None:
    root, base = repo
    _change(root, *changed)
    matrix = build_matrix("pull_request", "clazy", root, base)
    assert matrix[0] == {"tool": "clazy", "shard": 1, "shard_count": 1}
    assert _tidy_counts(matrix) == {count}


def test_header_fan_out_caps_at_max_shards(repo) -> None:
    root, base = repo
    for i in range(MAX_TIDY_SHARDS * UNITS_PER_SHARD):
        (root / f"src/more/extra{i}.cc").parent.mkdir(parents=True, exist_ok=True)
        (root / f"src/more/extra{i}.cc").write_text("#include <Wide.h>\n")
    _change(root, "src/Wide.h")
    assert _tidy_counts(build_matrix("pull_request", "clazy", root, base)) == {MAX_TIDY_SHARDS}


def test_missing_base_keeps_every_shard(tmp_path: Path) -> None:
    assert _tidy_counts(build_matrix("pull_request", "clazy", tmp_path, "")) == {MAX_TIDY_SHARDS}


def test_invalid_base_stops_planning(repo) -> None:
    root, _ = repo
    with pytest.raises(RuntimeError, match="Unable to determine changed files"):
        build_matrix("pull_request", "clazy", root, "missing-ref")


def test_every_non_source_analysis_trigger_keeps_every_shard() -> None:
    workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/analysis.yml").read_text())
    # PyYAML reads the bare "on" key as a boolean.
    paths = (workflow.get("on") or workflow[True])["pull_request"]["paths"]
    assert ".github/scripts/analysis_matrix.py" in paths
    for path in paths:
        if not path.startswith("src/"):
            assert may_select_full_scan(path.replace("**", "Example.txt")), path


def test_main_writes_include_output(repo, gh_output: Path) -> None:
    root, base = repo
    _change(root, "src/narrow.cc")
    assert main(["--event", "pull_request", "--base", base, "--root", str(root)]) == 0
    include = json.loads(gh_output.read_text().split("include=", 1)[1])
    assert _tidy_counts(include) == {1}
