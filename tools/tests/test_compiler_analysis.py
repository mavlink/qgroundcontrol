"""Compiler analyzer regressions: diagnostics, invocation errors, and file selection."""

import json
import sys
from pathlib import Path
from subprocess import CompletedProcess, TimeoutExpired
from unittest.mock import patch

import analyze
import pytest
from analyze import FileCollector
from analyzers.clang_tidy import ClangTidyAnalyzer
from analyzers.clazy import ClazyAnalyzer


@pytest.fixture(autouse=True)
def compilation_database(tmp_path):
    (tmp_path / "compile_commands.json").write_text(
        json.dumps([{"directory": str(tmp_path), "file": "file.cc", "command": "c++ -c file.cc"}])
    )


@pytest.mark.parametrize("analyzer_type", [ClazyAnalyzer, ClangTidyAnalyzer])
@pytest.mark.parametrize(
    "code,diagnostic,error",
    [
        (0, "file.cc:2:3: warning: inefficient copy [clazy-range-loop]\n", False),
        (1, "file.cc:2:3: error: inefficient copy [performance-copy,-warnings-as-errors]\n", False),
        (1, "file.cc:2:3: error: missing header\n", True),
        (
            1,
            "clang-tidy: error: unable to handle compilation, expected exactly one compiler job\n",
            True,
        ),
        (1, "clazy-standalone: error: no input files\n", True),
        (1, "/usr/bin/clang++-18: error: unsupported option\n", True),
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
    assert result.error_findings == ("error:" in diagnostic and "[" in diagnostic)
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


@pytest.mark.parametrize("analyzer_type", [ClazyAnalyzer, ClangTidyAnalyzer])
@pytest.mark.parametrize("driver_error", [True, False])
def test_advisory_cli_rejects_driver_errors_but_allows_warnings(
    tmp_path, analyzer_type, driver_error
):
    source = tmp_path / "file.cc"
    source.write_text("int value = 1;\n")
    analyzer = analyzer_type(tmp_path, tmp_path)
    diagnostic = (
        f"{analyzer.executable}: error: unable to handle compilation, expected exactly one compiler job\n"
        if driver_error
        else "file.cc:1:1: warning: inefficient copy [performance-copy]\n"
    )
    with (
        patch.object(sys, "argv", ["analyze.py", "--tool", analyzer.name, "--advisory", "file.cc"]),
        patch.object(analyze, "find_repo_root", return_value=tmp_path),
        patch.object(analyze, "get_analyzer", return_value=analyzer),
        patch.object(analyzer, "require_compile_commands", return_value=True),
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], int(driver_error), "", diagnostic),
        ),
    ):
        assert analyze.main() == (2 if driver_error else 0)


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


@pytest.mark.parametrize("analyzer_type", [ClazyAnalyzer, ClangTidyAnalyzer])
def test_headers_select_active_project_units_once(tmp_path, analyzer_type):
    entries = ["src/active.cc", "src/active.cc", "src/other.cpp", "test/check.cc", ".cache/lib.cc"]
    (tmp_path / "compile_commands.json").write_text(
        json.dumps([{"directory": str(tmp_path), "file": file} for file in entries])
    )
    analyzer = analyzer_type(tmp_path, tmp_path)
    with (
        patch("analyzers.compiler.header_dependents", return_value=None),
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")
        ) as run,
    ):
        result = analyzer.run([tmp_path / "src/header.h", tmp_path / "src/inactive.cc"])
    assert result.passed
    assert result.files_checked == 3
    assert {call.args[0][-1] for call in run.call_args_list} == {
        str(tmp_path / file) for file in ("src/active.cc", "src/other.cpp", "test/check.cc")
    }


def test_header_selection_keeps_changed_sources_and_records_timings(tmp_path):
    entries = ["src/changed.cc", "src/dependent.cc", "src/unrelated.cc"]
    (tmp_path / "compile_commands.json").write_text(
        json.dumps([{"directory": str(tmp_path), "file": file} for file in entries])
    )
    analyzer = ClazyAnalyzer(tmp_path, tmp_path)
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("analyzers.compiler.header_dependents", return_value={tmp_path / "src/dependent.cc"}),
        patch("analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")),
    ):
        result = analyzer.run([tmp_path / "src/header.h", tmp_path / "src/changed.cc"])
    assert result.passed and result.files_checked == 2
    timings = json.loads((tmp_path / "clazy-timings.json").read_text())
    assert {row["file"] for row in timings["files"]} == {"src/changed.cc", "src/dependent.cc"}
    assert all(row["seconds"] >= 0 for row in timings["files"])


def test_source_selection_skips_inactive_platforms(tmp_path):
    analyzer = ClazyAnalyzer(tmp_path, tmp_path)
    with patch.object(analyzer, "require_tool", return_value=True):
        assert analyzer.run([tmp_path / "windows.cc"]).skipped
    assert analyzer._translation_units([tmp_path / "file.cc"] * 2) == [tmp_path / "file.cc"]


def test_tidy_uses_checkout_config_for_external_cached_headers(tmp_path):
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")
        ) as run,
    ):
        assert analyzer.run([tmp_path / "file.cc"]).passed
    assert f"--config-file={tmp_path / '.clang-tidy'}" in run.call_args.args[0]


@pytest.mark.parametrize("contents", ["invalid json", "{}", '[{"file": 1}]'])
def test_invalid_database_is_an_execution_error(tmp_path, contents):
    (tmp_path / "compile_commands.json").write_text(contents)
    with patch.object(ClazyAnalyzer, "require_tool", return_value=True):
        assert ClazyAnalyzer(tmp_path, tmp_path).run([tmp_path / "file.cc"]).execution_error


def test_repeated_header_diagnostics_keep_distinct_notes_and_raw_output(tmp_path, capsys):
    entries = [{"directory": str(tmp_path), "file": name} for name in ("a.cc", "b.cc")]
    (tmp_path / "compile_commands.json").write_text(json.dumps(entries))
    diagnostic = "header.h:2:3: warning: inefficient copy [performance-copy]\n  value;\n  ^~~~~\n"
    note = "header.h:4:3: note: instantiated here\n"
    analyzer = ClazyAnalyzer(tmp_path, tmp_path, jobs=2)
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], 0, diagnostic + note, ""),
        ),
    ):
        result = analyzer.run([tmp_path / entry["file"] for entry in entries])
    assert result.issues == 2
    assert not result.passed
    assert result.output.count(diagnostic) == 1
    assert capsys.readouterr().out.count(diagnostic) == 1
    assert (tmp_path / "clazy-raw.txt").read_text().count(diagnostic + note) == 2

    from analyzers.compiler import DiagnosticDeduplicator

    dedup = DiagnosticDeduplicator()
    assert dedup.filter(diagnostic + note) == diagnostic + note
    assert dedup.filter(diagnostic + note) == ""
    changed_note = note.replace("4:3", "5:3")
    assert dedup.filter(diagnostic + changed_note) == diagnostic + changed_note
    driver_error = "clang-tidy: error: cannot load database\n"
    assert dedup.filter(driver_error) == driver_error
    assert dedup.filter(driver_error) == driver_error


def test_clang_tidy_profile_paths_do_not_collide_for_same_basename(tmp_path):
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    assert analyzer._file_arguments(tmp_path / "first/file.cc") == ()
    assert not (tmp_path / "clang-tidy-profiles").exists()
    analyzer.profile_checks = True
    first = analyzer._file_arguments(tmp_path / "first/file.cc")
    second = analyzer._file_arguments(tmp_path / "second/file.cc")
    assert "--enable-check-profile" in first
    assert first != second
    for arguments in (first, second):
        prefix = arguments[-1].split("=", 1)[1]
        assert Path(prefix).is_dir()
        assert Path(prefix).is_relative_to(tmp_path / "clang-tidy-profiles")


def test_check_profiles_sum_wall_time_without_stale_or_unselected_results(tmp_path):
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    analyzer.profile_checks = True
    files = [tmp_path / "a.cc", tmp_path / "b.cc"]
    for file in files:
        arguments = analyzer._file_arguments(file)
        directory = Path(arguments[-1].split("=", 1)[1])
        (directory / "run.json").write_text(
            json.dumps(
                {
                    "profile": {
                        "time.clang-tidy.readability-name.wall": 2.5,
                        "time.clang-tidy.readability-name.user": 2.0,
                        "time.clang-tidy.performance-copy.wall": 1.0,
                    }
                }
            )
        )
        (directory / "incomplete.json").write_text("{")
    assert analyzer._check_timings(files) == {"readability-name": 5.0, "performance-copy": 2.0}
    assert analyzer._check_timings(files[:1]) == {"readability-name": 2.5, "performance-copy": 1.0}
    analyzer.profile_checks = False
    assert analyzer._check_timings(files) == {}
    analyzer.profile_checks = True
    analyzer._file_arguments(files[0])
    assert analyzer._check_timings(files[:1]) == {}


def test_analysis_progress_survives_before_final_summary(tmp_path):
    analyzer = ClazyAnalyzer(tmp_path, tmp_path)
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")),
    ):
        assert analyzer.run([tmp_path / "file.cc"]).passed
    progress = [
        json.loads(line) for line in (tmp_path / "clazy-progress.jsonl").read_text().splitlines()
    ]
    report = json.loads((tmp_path / "clazy-timings.json").read_text())
    assert progress == report["files"]
    assert report["wall_seconds"] >= 0
    assert report["workers"] == 1


@pytest.mark.parametrize("shard_count", [1, 4, 8])
def test_shards_cover_each_selected_dependency_once_and_preserve_errors(tmp_path, shard_count):
    entries = ["src/a.cc", "src/b.cc", "src/c.cc", "src/d.cc", "src/changed.cc", "src/a.cc"]
    (tmp_path / "compile_commands.json").write_text(
        json.dumps([{"directory": str(tmp_path), "file": file} for file in entries])
    )
    dependents = {tmp_path / file for file in ("src/a.cc", "src/b.cc", "src/d.cc")}
    expected = dependents | {tmp_path / "src/changed.cc"}
    checked = []
    results = []

    def run(command, **kwargs):
        file = Path(command[-1])
        checked.append(file)
        return CompletedProcess(
            command,
            int(file.name == "b.cc"),
            "",
            "b.cc:1:1: fatal error: missing header\n" if file.name == "b.cc" else "",
        )

    for shard in range(1, shard_count + 1):
        analyzer = ClangTidyAnalyzer(tmp_path, tmp_path, shard=shard, shard_count=shard_count)
        with (
            patch.object(analyzer, "require_tool", return_value=True),
            patch("analyzers.compiler.header_dependents", return_value=dependents),
            patch("analyzers.compiler.run_captured", side_effect=run),
        ):
            result = analyzer.run([tmp_path / "src/header.h", tmp_path / "src/changed.cc"])
        results.append(result)
        if not result.skipped:
            report = json.loads((tmp_path / "clang-tidy-timings.json").read_text())
            assert report["selected_files"] == len(expected)
            assert report["shard"] == shard
            assert report["shard_count"] == shard_count
    assert set(checked) == expected
    assert len(checked) == len(expected)
    assert sum(result.execution_error for result in results) == 1
    assert sum(result.skipped for result in results) == max(0, shard_count - len(expected))


@pytest.mark.parametrize("profile_checks", [False, True])
def test_cli_check_profiling_is_opt_in(tmp_path, monkeypatch, profile_checks):
    (tmp_path / "file.cc").touch()
    args = ["analyze.py", "--tool", "clang-tidy", "-b", ".", "--advisory", "file.cc"]
    if profile_checks:
        args.append("--profile-checks")
    monkeypatch.setattr(sys, "argv", args)
    monkeypatch.setattr(analyze, "find_repo_root", lambda: tmp_path)
    with (
        patch.object(ClangTidyAnalyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")
        ) as run,
    ):
        assert analyze.main() == 0
    assert ("--enable-check-profile" in run.call_args.args[0]) == profile_checks


@pytest.mark.parametrize(
    "arguments",
    [
        ["--shard", "0"],
        ["--shard", "2"],
        ["--shard-count", "0"],
        ["--tool", "qmllint", "--shard-count", "2"],
        ["--tool", "clazy", "--profile-checks"],
    ],
)
def test_invalid_analysis_modes_are_rejected(monkeypatch, arguments):
    monkeypatch.setattr(sys, "argv", ["analyze.py", *arguments])
    with pytest.raises(SystemExit) as exc:
        analyze.parse_args()
    assert exc.value.code == 2
