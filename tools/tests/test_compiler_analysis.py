"""Compiler analyzer regressions: diagnostics, invocation errors, and file selection."""

from pathlib import Path
from subprocess import CompletedProcess, TimeoutExpired
from unittest.mock import patch

import pytest
from analyze import FileCollector
from analyzers.clang_tidy import ClangTidyAnalyzer
from analyzers.clazy import ClazyAnalyzer


@pytest.mark.parametrize("analyzer_type", [ClazyAnalyzer, ClangTidyAnalyzer])
@pytest.mark.parametrize(
    "code,diagnostic,error",
    [
        (0, "file.cc:2:3: warning: inefficient copy [clazy-range-loop]\n", False),
        (1, "file.cc:2:3: error: inefficient copy [performance-copy,-warnings-as-errors]\n", False),
        (1, "file.cc:2:3: error: missing header\n", True),
        (2, "cannot load compilation database\n", True),
    ],
)
def test_diagnostics_survive_success_and_error_exits(
    tmp_path, analyzer_type, code, diagnostic, error, capsys
):
    analyzer = analyzer_type(tmp_path, tmp_path)
    source = tmp_path / "file.cc"
    with (
        patch.object(analyzer, "require_compile_commands", return_value=True),
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], code, "", diagnostic),
        ),
    ):
        result = analyzer.run([source])
    assert not result.passed
    assert result.execution_error == error
    assert result.files_checked == 1
    assert diagnostic in result.output
    assert diagnostic in capsys.readouterr().out


def test_timeout_is_an_execution_error(tmp_path):
    analyzer = ClazyAnalyzer(tmp_path, tmp_path)
    with (
        patch.object(analyzer, "require_compile_commands", return_value=True),
        patch.object(analyzer, "require_tool", return_value=True),
        patch("analyzers.compiler.run_captured", side_effect=TimeoutExpired("clazy", 300)),
    ):
        assert analyzer.run([tmp_path / "file.cc"]).execution_error


def test_no_files_is_explicitly_skipped(tmp_path):
    assert ClazyAnalyzer(tmp_path, tmp_path).run([]).status == "skipped"


def test_precommit_individual_filename_is_analyzed(tmp_path):
    source = tmp_path / "file.cc"
    source.touch()
    assert FileCollector(tmp_path).get_cpp_files(Path("file.cc")) == [source]


def test_diff_failure_is_not_a_clean_scan(tmp_path):
    with (
        patch("analyze.run_git", return_value=CompletedProcess([], 128, "", "bad revision")),
        pytest.raises(RuntimeError, match="Unable to determine"),
    ):
        FileCollector(tmp_path)._get_changed_files((".cc",), "bad")
