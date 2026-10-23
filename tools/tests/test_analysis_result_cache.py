"""Result-cache keys, replay and eviction for compiler analysis."""

from __future__ import annotations

import json
import os
import sys
from pathlib import Path
from subprocess import CompletedProcess, TimeoutExpired
from unittest.mock import patch

import analyze
import pytest
from analyzers.clang_tidy import ClangTidyAnalyzer
from analyzers.clazy import ClazyAnalyzer
from analyzers.dependencies import DependencyScanError
from analyzers.result_cache import CachedResult, ResultCache, compile_arguments

ENTRY = {
    "directory": "/build",
    "file": "/src/a.cc",
    "command": "clang++ -DX=1 -Iinc -MD -MT a.o -MF a.o.d -o a.o -c /src/a.cc",
}


def test_compile_arguments_drop_output_only_options():
    expected = ["clang++", "-DX=1", "-Iinc", "-c", "/src/a.cc"]
    assert compile_arguments(ENTRY) == expected
    listed = {**ENTRY, "arguments": ["clang++", "-o", "b.o", "-DX=1", "-Iinc", "-c", "/src/a.cc"]}
    assert compile_arguments(listed) == expected


def test_key_covers_identity_commands_and_dependency_contents(tmp_path):
    cache = ResultCache(tmp_path / "cache")
    header = tmp_path / "a.h"
    header.write_text("int a;\n")
    base = cache.key(("clang-tidy", "23.1.3"), [ENTRY], [header])
    assert base is not None
    moved_output = {**ENTRY, "command": ENTRY["command"].replace("-o a.o", "-o elsewhere/a.o")}
    assert cache.key(("clang-tidy", "23.1.3"), [moved_output], [header]) == base
    assert cache.key(("clang-tidy", "22.1.8"), [ENTRY], [header]) != base
    changed_flag = {**ENTRY, "command": ENTRY["command"].replace("-DX=1", "-DX=2")}
    assert cache.key(("clang-tidy", "23.1.3"), [changed_flag], [header]) != base
    assert cache.key(("clang-tidy", "23.1.3"), [ENTRY, changed_flag], [header]) != base
    assert ResultCache(tmp_path / "cache").key(("clang-tidy", "23.1.3"), [ENTRY], [header]) == base
    header.write_text("int b;\n")
    assert ResultCache(tmp_path / "cache").key(("clang-tidy", "23.1.3"), [ENTRY], [header]) != base
    assert cache.key(("clang-tidy", "23.1.3"), [ENTRY], [tmp_path / "missing.h"]) is None


def test_replay_returns_stored_output_and_refreshes_entry_age(tmp_path):
    cache = ResultCache(tmp_path)
    key = "ab" + "0" * 62
    assert cache.load(key) is None
    stored = CachedResult("a.cc:1:1: warning: x [check]\n", "1 warning generated.\n", 0)
    assert cache.store(key, stored)
    entry = tmp_path / "ab" / f"{key}.json"
    os.utime(entry, (1, 1))
    assert cache.load(key) == stored
    assert entry.stat().st_mtime > 1


@pytest.mark.parametrize(
    "result",
    [
        CachedResult("partial output\n", "", -11),
        CachedResult("", "Stack dump:\n0. Program arguments: clang-tidy\n", 1),
        CachedResult("", "PLEASE submit a bug report to https://github.com/llvm\n", 0),
    ],
)
def test_crashes_are_never_stored(tmp_path, result):
    cache = ResultCache(tmp_path)
    assert not cache.store("cd" + "0" * 62, result)
    assert not any(tmp_path.rglob("*.json"))


def test_prune_drops_expired_then_least_recently_used_entries(tmp_path):
    cache = ResultCache(tmp_path, max_bytes=250, max_age_seconds=100)
    now = 1_000_000.0
    for name, age in (("aa", 500), ("bb", 50), ("cc", 40), ("dd", 10)):
        key = name + "0" * 62
        cache.store(key, CachedResult("x" * 40, "", 0))
        os.utime(tmp_path / name / f"{key}.json", (now - age, now - age))
    fresh_temporary = tmp_path / "ee" / "partial.tmp"
    fresh_temporary.parent.mkdir()
    fresh_temporary.write_text("writing")
    os.utime(fresh_temporary, (now - 10, now - 10))
    assert cache.prune(now) == 2
    assert sorted(path.name[:2] for path in tmp_path.glob("*/*.json")) == ["cc", "dd"]
    assert fresh_temporary.exists()


def _scan(dependencies: dict[Path, set[Path]]):
    def scan(entries, jobs):
        return {
            source: frozenset(dependencies.get(source, set()) | {source})
            for source in {
                (Path(entry["directory"]) / entry["file"]).resolve() for entry in entries
            }
        }

    return scan


@pytest.fixture
def project(tmp_path):
    """Three active sources sharing a header, a fake tool binary and its compile commands."""
    sources = [tmp_path / "src" / name for name in ("a.cc", "b.cc", "c.cc")]
    header = tmp_path / "src" / "shared.h"
    header.parent.mkdir()
    header.write_text("#pragma once\n")
    for source in sources:
        source.write_text('#include "shared.h"\n')
    (tmp_path / ".clang-tidy").write_text("Checks: '-*,bugprone-*'\n")
    (tmp_path / "compile_commands.json").write_text(
        json.dumps(
            [
                {"directory": str(tmp_path), "file": str(source), "command": f"c++ -c {source}"}
                for source in sources
            ]
        )
    )
    binary = tmp_path / "tool"
    binary.write_bytes(b"tool-binary")
    return tmp_path, sources, header, binary


def _analyze(project, analyzer_type, *, runner, cache_dir=None, profile=False):
    root, sources, header, binary = project
    analyzer = analyzer_type(root, root, jobs=2)
    analyzer.result_cache = ResultCache(cache_dir or root / "cache")
    analyzer.profile_checks = profile
    calls: list[str] = []

    def run(command, **kwargs):
        if command[-1] == "--version":
            return CompletedProcess(command, 0, f"{analyzer.name} version 1\n", "")
        calls.append(Path(command[-1]).name)
        return runner(Path(command[-1]))

    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("common.proc.shutil.which", return_value=str(binary)),
        patch(
            "analyzers.compiler.scan_dependencies",
            side_effect=_scan({source: {header} for source in sources}),
        ),
        patch("analyzers.compiler.run_captured", side_effect=run),
    ):
        result = analyzer.run(sources)
    timings = json.loads((root / f"{analyzer.name}-timings.json").read_text())
    return result, sorted(calls), timings


def _warning(file: Path) -> CompletedProcess:
    return CompletedProcess([], 0, f"{file}:1:1: warning: issue [bugprone-x]\n", "1 warning.\n")


@pytest.mark.parametrize("analyzer_type", [ClangTidyAnalyzer, ClazyAnalyzer])
def test_second_run_replays_every_unchanged_unit(project, analyzer_type, capsys):
    first, calls, timings = _analyze(project, analyzer_type, runner=_warning)
    first_output = capsys.readouterr().out
    assert calls == ["a.cc", "b.cc", "c.cc"]
    assert timings["result_cache"]["hits"] == 0
    assert timings["result_cache"]["misses"] == 3
    second, calls, timings = _analyze(project, analyzer_type, runner=_warning)
    assert calls == []
    assert timings["result_cache"] == {"hits": 3, "misses": 0, "uncacheable": 0, "pruned": 0}
    assert all(row["cached"] for row in timings["files"])
    # Workers finish in any order, so compare each unit's output.
    assert (second.passed, second.issues) == (first.passed, first.issues)

    def units(output: str) -> list[str]:
        return sorted(part.strip() for part in output.split("\n\n"))

    assert units(second.output) == units(first.output)
    assert "(cached)" in capsys.readouterr().out
    assert "(cached)" not in first_output


def test_changed_dependency_or_config_reanalyzes_only_affected_units(project):
    root, sources, header, _ = project
    _analyze(project, ClangTidyAnalyzer, runner=_warning)
    sources[1].write_text('#include "shared.h"\nint b;\n')
    _, calls, _ = _analyze(project, ClangTidyAnalyzer, runner=_warning)
    assert calls == ["b.cc"]
    header.write_text("#pragma once\nint shared;\n")
    _, calls, _ = _analyze(project, ClangTidyAnalyzer, runner=_warning)
    assert calls == ["a.cc", "b.cc", "c.cc"]
    (root / ".clang-tidy").write_text("Checks: '-*,performance-*'\n")
    _, calls, _ = _analyze(project, ClangTidyAnalyzer, runner=_warning)
    assert calls == ["a.cc", "b.cc", "c.cc"]
    _, calls, _ = _analyze(project, ClazyAnalyzer, runner=_warning)
    assert calls == ["a.cc", "b.cc", "c.cc"], "tools never share entries"


def test_failures_and_timeouts_are_not_replayed(project):
    def failing(file: Path):
        if file.name == "a.cc":
            return CompletedProcess([], 1, "", f"{file}:1:1: fatal error: 'x.h' file not found\n")
        if file.name == "b.cc":
            raise TimeoutExpired("clang-tidy", 300)
        return _warning(file)

    result, calls, _ = _analyze(project, ClangTidyAnalyzer, runner=failing)
    assert result.execution_error
    assert calls == ["a.cc", "b.cc", "c.cc"]
    result, calls, _ = _analyze(project, ClangTidyAnalyzer, runner=failing)
    assert result.execution_error
    assert calls == ["a.cc", "b.cc"]


def test_error_level_findings_replay_as_errors(project):
    def error_finding(file: Path):
        return CompletedProcess([], 1, f"{file}:1:1: error: moved [bugprone-use-after-move]\n", "")

    first, _, _ = _analyze(project, ClangTidyAnalyzer, runner=error_finding)
    second, calls, _ = _analyze(project, ClangTidyAnalyzer, runner=error_finding)
    assert calls == []
    assert first.error_findings and second.error_findings
    assert not second.execution_error


def test_profiling_and_incomplete_scans_bypass_the_cache(project, capsys):
    root, sources, _, binary = project
    _analyze(project, ClangTidyAnalyzer, runner=_warning)
    _, calls, timings = _analyze(project, ClangTidyAnalyzer, runner=_warning, profile=True)
    assert calls == ["a.cc", "b.cc", "c.cc"]
    assert timings["result_cache"] == {}
    analyzer = ClangTidyAnalyzer(root, root)
    analyzer.result_cache = ResultCache(root / "cache")
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("common.proc.shutil.which", return_value=str(binary)),
        patch("analyzers.compiler.scan_dependencies", side_effect=DependencyScanError("gone")),
        patch(
            "analyzers.compiler.run_captured", side_effect=lambda cmd, **_: _warning(Path(cmd[-1]))
        ),
    ):
        assert analyzer.run(sources).issues == 3
    assert "Result cache disabled: gone" in capsys.readouterr().out


def test_cli_runs_both_compiler_tools_over_one_dependency_scan(project, monkeypatch):
    root, sources, header, binary = project
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "analyze.py",
            "--tool",
            "clang-tidy",
            "--tool",
            "clazy",
            "-b",
            ".",
            "--cache-dir",
            "cache",
            "--advisory",
            "src",
        ],
    )
    monkeypatch.setattr(analyze, "find_repo_root", lambda: root)
    scans: list[int] = []
    scan = _scan({source: {header} for source in sources})

    def counting_scan(entries, jobs):
        scans.append(len(entries))
        return scan(entries, jobs)

    def run(command, **kwargs):
        if command[-1] == "--version":
            return CompletedProcess(command, 0, "version\n", "")
        return _warning(Path(command[-1]))

    with (
        patch("common.proc.shutil.which", return_value=str(binary)),
        patch("analyzers.compiler.scan_dependencies", side_effect=counting_scan),
        patch("analyzers.compiler.run_captured", side_effect=run),
    ):
        assert analyze.main() == 0
    assert scans == [3]
    for tool in ("clang-tidy", "clazy"):
        report = json.loads((root / f"{tool}-timings.json").read_text())
        assert report["result_cache"]["misses"] == 3


@pytest.mark.parametrize(
    "arguments",
    [
        ["--tool", "clang-tidy", "--tool", "qmllint"],
        ["--tool", "clang-format", "--cache-dir", "cache"],
        ["--tool", "clazy", "--tool", "clazy", "--profile-checks"],
    ],
)
def test_cli_rejects_mixed_or_unsupported_cached_tools(arguments):
    with pytest.raises(SystemExit) as exc:
        analyze.parse_args(arguments)
    assert exc.value.code == 2
