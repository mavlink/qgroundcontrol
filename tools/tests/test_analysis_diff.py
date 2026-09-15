"""Changed-line compiler reporting, using real Git patches and mocked compilers."""

from __future__ import annotations

import json
import sys
from subprocess import CompletedProcess
from unittest.mock import patch

import analyze
import pytest
from analyzers.clang_tidy import ClangTidyAnalyzer
from analyzers.clazy import ClazyAnalyzer
from analyzers.compiler import ChangedLineFilter
from common.git import get_changed_line_ranges, run_git


@pytest.fixture
def git_repo(tmp_path):
    def git(*args):
        return run_git(*args, cwd=tmp_path, check=True).stdout.strip()

    git("init", "-q")
    git("config", "user.email", "test@example.invalid")
    git("config", "user.name", "Test")
    git("config", "commit.gpgsign", "false")
    git("config", "core.hooksPath", str(tmp_path / "no-hooks"))
    (tmp_path / "src").mkdir()
    for name in ("changed.cc", "header.h", "deleted.cc", "deletion.cc", "old name.hpp"):
        (tmp_path / "src" / name).write_text("".join(f"int value{i};\n" for i in range(1, 11)))
    git("add", ".")
    git("commit", "-qm", "base")
    base = git("rev-parse", "HEAD")
    return tmp_path, git, base


def test_real_diff_tracks_new_lines_deletions_and_renames(git_repo):
    root, git, base = git_repo
    (root / "src/changed.cc").write_text("new first\nint value1;\nnew third\n")
    header = root / "src/header.h"
    header.write_text(header.read_text().replace("value4", "changed4"))
    (root / "src/deleted.cc").unlink()
    deletion = root / "src/deletion.cc"
    deletion.write_text(deletion.read_text().replace("int value3;\n", ""))
    renamed = root / "src/new name \u00e9.hpp"
    (root / "src/old name.hpp").rename(renamed)
    renamed.write_text(renamed.read_text().replace("value8", "changed8"))
    (root / "src/added file.cc").write_text("first\nsecond\n")
    (root / "README.md").write_text("not C++\n")
    git("add", ".")
    git("commit", "-qm", "changes")

    ranges = get_changed_line_ranges(root, base, analyze.FileCollector.CPP_EXTENSIONS)

    assert ranges == {
        (root / "src/changed.cc").resolve(): [(1, 1), (3, 3)],
        header.resolve(): [(4, 4)],
        deletion.resolve(): [],
        renamed.resolve(): [(8, 8)],
        (root / "src/added file.cc").resolve(): [(1, 2)],
    }


def test_diff_uses_merge_base_and_ignores_worktree_changes(git_repo):
    root, git, base = git_repo
    branch = git("branch", "--show-current")
    git("checkout", "-qb", "base-side")
    (root / "src/base-only.cc").write_text("base only\n")
    git("add", ".")
    git("commit", "-qm", "base side")
    base_tip = git("rev-parse", "HEAD")
    git("checkout", "-q", branch)
    renamed = root / "src/renamed.hpp"
    (root / "src/old name.hpp").rename(renamed)
    git("add", ".")
    git("commit", "-qm", "rename only")
    (root / "src/changed.cc").write_text("uncommitted\n")

    assert get_changed_line_ranges(root, base_tip, (".cc", ".hpp")) == {renamed.resolve(): []}
    assert get_changed_line_ranges(root, base, (".cc", ".hpp")) == {renamed.resolve(): []}


def test_invalid_diff_base_fails_explicitly(git_repo, monkeypatch, capsys):
    root, _, _ = git_repo
    monkeypatch.setattr(sys, "argv", ["analyze.py", "--diff-base", "missing-ref"])
    monkeypatch.setattr(analyze, "find_repo_root", lambda: root)
    with patch.object(ClangTidyAnalyzer, "run") as run:
        assert analyze.main() == 2
    run.assert_not_called()
    assert "Unable to determine changed lines" in capsys.readouterr().err


@pytest.mark.parametrize("tool", ["clang-tidy", "clazy"])
def test_no_cpp_diff_skips_without_compiler_prerequisites(git_repo, monkeypatch, capsys, tool):
    root, git, base = git_repo
    (root / "README.md").write_text("documentation only\n")
    git("add", ".")
    git("commit", "-qm", "docs")
    monkeypatch.setattr(sys, "argv", ["analyze.py", "--tool", tool, "--diff-base", base])
    monkeypatch.setattr(analyze, "find_repo_root", lambda: root)
    with patch("analyzers.compiler.run_captured") as run:
        assert analyze.main() == 0
    run.assert_not_called()
    assert f"{tool}: skipped (0 files)" in capsys.readouterr().out


@pytest.mark.parametrize(
    "arguments",
    [
        ["--all"],
        ["src/"],
        ["--tool", "cppcheck"],
        ["--tool", "clang-format"],
    ],
)
def test_diff_base_rejects_conflicting_modes(monkeypatch, arguments):
    monkeypatch.setattr(sys, "argv", ["analyze.py", "--diff-base", "HEAD", *arguments])
    with pytest.raises(SystemExit) as exc:
        analyze.parse_args()
    assert exc.value.code == 2


@pytest.mark.parametrize("analyzer_type", [ClangTidyAnalyzer, ClazyAnalyzer])
@pytest.mark.parametrize("jobs", [1, 3])
def test_diff_reporting_keeps_dependency_selection_raw_logs_and_workers(
    tmp_path, analyzer_type, jobs
):
    root = tmp_path.resolve()
    build = root / "build"
    build.mkdir()
    entries = [
        {"directory": str(build), "file": f"../src/{name}.cc"}
        for name in ("changed", "dependent", "unrelated")
    ]
    (build / "compile_commands.json").write_text(json.dumps(entries))
    analyzer = analyzer_type(root, build, jobs=jobs)
    analyzer.changed_lines = {root / "src/changed.cc": [(2, 2)], root / "src/header.h": [(4, 5)]}
    output = (
        "../src/changed.cc:2:1: warning: changed source [performance-copy]\n"
        "../src/header.h:4:1: warning: changed header [clazy-range-loop]\n"
        "../src/header.h:9:1: note: context for retained header finding\n"
        "../src/changed.cc:8:1: warning: unchanged source [performance-copy]\n"
        "  8 | value;\n"
        "    | ^~~~~\n"
        "../src/header.h:8:1: warning: unchanged header [clazy-range-loop]\n"
        "  value;\n"
        "  ^~~~~\n"
        "../src/header.h:9:1: note: context for suppressed finding\n"
    )
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.header_dependents", return_value={root / "src/dependent.cc"}
        ) as dependencies,
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], 0, output, ""),
        ) as run,
    ):
        result = analyzer.run(list(analyzer.changed_lines))
    assert dependencies.call_args.args[1:] == ({root / "src/header.h"}, jobs)
    assert {call.args[0][-1] for call in run.call_args_list} == {
        str(root / "src/changed.cc"),
        str(root / "src/dependent.cc"),
    }
    assert result.files_checked == 2 and result.issues == 2
    assert "changed source" in result.output and "changed header" in result.output
    assert "retained header finding" in result.output
    assert "unchanged" not in result.output and "suppressed finding" not in result.output
    assert "value;" not in result.output and "^~~~~" not in result.output
    assert result.output.count("changed header") == 1
    assert (build / f"{analyzer.name}-raw.txt").read_text().count(output) == 2
    timings = json.loads((build / f"{analyzer.name}-timings.json").read_text())
    assert timings["workers"] == jobs
    assert len(timings["files"]) == 2


@pytest.mark.parametrize("analyzer_type", [ClangTidyAnalyzer, ClazyAnalyzer])
@pytest.mark.parametrize("focused", [False, True])
def test_only_outside_warnings_pass_only_in_diff_mode(tmp_path, analyzer_type, focused):
    (tmp_path / "compile_commands.json").write_text(
        json.dumps([{"directory": str(tmp_path), "file": "file.cc"}])
    )
    analyzer = analyzer_type(tmp_path, tmp_path)
    if focused:
        analyzer.changed_lines = {tmp_path / "file.cc": []}
    warning = "file.cc:9:1: warning: old code [performance-copy]\n"
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, warning, "")),
    ):
        result = analyzer.run([tmp_path / "file.cc"])
    assert result.passed == focused
    assert result.files_checked == 1
    assert ("old code" in result.output) != focused
    assert warning in (tmp_path / f"{analyzer.name}-raw.txt").read_text()


@pytest.mark.parametrize("analyzer_type", [ClangTidyAnalyzer, ClazyAnalyzer])
@pytest.mark.parametrize(
    "code,output,execution_error,error_findings",
    [
        (1, "file.cc:9:1: fatal error: missing header\n", True, False),
        (1, "file.cc:9:1: error: bad syntax [clang-diagnostic-error]\n", True, True),
        (
            1,
            "file.cc:9:1: error: old finding [performance-copy,-warnings-as-errors]\n",
            False,
            True,
        ),
        (1, "clang-tidy: error: invalid arguments\n", True, False),
        (2, "file.cc:9:1: warning: old warning [performance-copy]\ncrashed\n", True, False),
        (-11, "file.cc:9:1: warning: old warning [performance-copy]\n", True, False),
        (0, "file.cc:?:1: warning: malformed location [check]\n", False, False),
        (0, "file.cc:0:1: warning: invalid location [check]\n", False, False),
        (0, "<built-in>:1:1: warning: unknown location [check]\n", False, False),
        (0, "file.cc:9:1: warning: unrecognized diagnostic\n", False, False),
    ],
)
def test_diff_preserves_errors_failures_and_unfamiliar_diagnostics(
    tmp_path, analyzer_type, code, output, execution_error, error_findings
):
    (tmp_path / "compile_commands.json").write_text(
        json.dumps([{"directory": str(tmp_path), "file": "file.cc"}])
    )
    analyzer = analyzer_type(tmp_path, tmp_path)
    analyzer.changed_lines = {tmp_path / "file.cc": [(1, 1)]}
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured", return_value=CompletedProcess([], code, output, "")
        ),
    ):
        result = analyzer.run([tmp_path / "file.cc"])
    assert not result.passed
    assert result.execution_error == execution_error
    assert result.error_findings == error_findings
    assert output in (tmp_path / f"{analyzer.name}-raw.txt").read_text()
    if "old warning" not in output:
        assert output in result.output


def test_unknown_output_ends_suppressed_context_and_paths_are_not_basename_matched(tmp_path):
    output = (
        "other/header.h:2:1: warning: unrelated [check]\n"
        "unexpected output\n"
        "other/header.h:3:1: note: keep unknown context\n"
        "src/header.h:2:1: warning: selected [check]\n"
    )
    filtered = ChangedLineFilter({tmp_path / "src/header.h": [(2, 2)]}, tmp_path).filter(
        output, tmp_path
    )
    assert filtered == output.split("\n", 1)[1]


@pytest.mark.parametrize("tool", ["clang-tidy", "clazy"])
@pytest.mark.parametrize("advisory", [False, True])
@pytest.mark.parametrize(
    "diagnostic,expected",
    [
        ("src/changed.cc:1:1: warning: unchanged source [check]\n", 0),
        ("src/header.h:4:1: warning: changed header [check]\n", 1),
        ("src/changed.cc:1:1: fatal error: broken compile\n", 2),
    ],
)
def test_cli_applies_real_header_diff_and_preserves_exit_policy(
    git_repo, monkeypatch, tool, advisory, diagnostic, expected
):
    root, git, base = git_repo
    header = root / "src/header.h"
    header.write_text(header.read_text().replace("value4", "changed4"))
    git("add", ".")
    git("commit", "-qm", "header change")
    source = root / "src/changed.cc"
    (root / "compile_commands.json").write_text(
        json.dumps([{"directory": str(root), "file": str(source)}])
    )
    args = ["analyze.py", "--tool", tool, "-b", ".", "--diff-base", base, "--jobs", "2"]
    if advisory:
        args.append("--advisory")
    monkeypatch.setattr(sys, "argv", args)
    monkeypatch.setattr(analyze, "find_repo_root", lambda: root)
    with (
        patch("analyzers.compiler.CompilerAnalyzer.require_tool", return_value=True),
        patch("analyzers.compiler.header_dependents", return_value={source}) as dependencies,
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], int(expected == 2), diagnostic, ""),
        ),
    ):
        assert analyze.main() == (0 if advisory and expected == 1 else expected)
    assert dependencies.call_args.args[1:] == ({header}, 2)
