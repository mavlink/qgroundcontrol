"""Compiler analyzer regressions: diagnostics, invocation errors, and file selection."""

import json
import re
import sys
from contextlib import nullcontext
from pathlib import Path
from subprocess import CompletedProcess, TimeoutExpired
from unittest.mock import patch

import analyze
import pytest
from analyze import FileCollector
from analyzers.clang_tidy import HEADER_DIRS, ClangTidyAnalyzer
from analyzers.clazy import ClazyAnalyzer
from analyzers.dependencies import DependencyScanError

from ._helpers import REPO_ROOT, commit_files, git


def _scan_including(header: Path, dependents: set[Path]):
    """Fake dependency scan in which only the given dependents include the header."""

    def scan(entries, jobs):
        sources = {(Path(entry["directory"]) / entry["file"]).resolve() for entry in entries}
        return {
            source: frozenset({source, header} if source in dependents else {source})
            for source in sources
        }

    return scan


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
        patch.object(analyze, "find_repo_root", return_value=tmp_path),
        patch.object(analyze, "get_analyzer", return_value=analyzer),
        patch.object(analyzer, "require_compile_commands", return_value=True),
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], int(driver_error), "", diagnostic),
        ),
    ):
        argv = ["--tool", analyzer.name, "--advisory", "file.cc"]
        assert analyze.main(argv) == (2 if driver_error else 0)


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
        patch(
            "analyzers.compiler.scan_dependencies", side_effect=DependencyScanError("unavailable")
        ),
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
        patch(
            "analyzers.compiler.scan_dependencies",
            side_effect=_scan_including(tmp_path / "src/header.h", {tmp_path / "src/dependent.cc"}),
        ),
        patch("analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")),
    ):
        result = analyzer.run([tmp_path / "src/header.h", tmp_path / "src/changed.cc"])
    assert result.passed and result.files_checked == 2
    timings = json.loads((tmp_path / "clazy-timings.json").read_text())
    assert {row["file"] for row in timings["files"]} == {"src/changed.cc", "src/dependent.cc"}
    assert all(row["seconds"] >= 0 for row in timings["files"])


def test_unique_diagnostics_are_recorded_once_per_location(tmp_path, monkeypatch):
    monkeypatch.setenv("GITHUB_SHA", "merge")
    source = tmp_path / "source"
    build = tmp_path / "build"
    build.mkdir()
    (build / "compile_commands.json").write_text(
        json.dumps([{"directory": str(source), "file": f"src/{name}"} for name in ("a.cc", "b.cc")])
    )
    header = f"{source}/src/shared.h:3:7: warning: shared [modernize-use-nodiscard]\n"
    outputs = {
        "a.cc": header + f"{source}/src/a.cc:1:2: warning: own [readability-braces]\n",
        "b.cc": header
        + f"{build}/gen/../gen/moc.h:4:1: error: bad [bugprone-macro]\n"
        + f"{source}/src/b.cc:2:1: fatal error: 'x.h' file not found\n",
    }

    def run(command, **kwargs):
        return CompletedProcess(command, 1, outputs[Path(command[-1]).name], "")

    analyzer = ClangTidyAnalyzer(source, build)
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("analyzers.compiler.run_captured", side_effect=run),
    ):
        analyzer.run([source / "src/a.cc", source / "src/b.cc"])

    assert json.loads((build / "clang-tidy-diagnostics.json").read_text()) == {
        "tool": "clang-tidy",
        "shard": 1,
        "commit": "merge",
        "diagnostics": {
            "build/gen/moc.h": [[4, 1, "error", "bugprone-macro"]],
            "src/a.cc": [[1, 2, "warning", "readability-braces"]],
            "src/b.cc": [[2, 1, "error", ""]],
            "src/shared.h": [[3, 7, "warning", "modernize-use-nodiscard"]],
        },
    }


@pytest.mark.parametrize("github_sha", [None, "merge"])
def test_pull_request_diagnostics_repeat_those_on_changed_lines(git_repo, monkeypatch, github_sha):
    base = commit_files(git_repo, {"src/a.cc": "1\n2\n3\n4\n", "src/shared.h": "h\n"}, "base")
    head = commit_files(git_repo, {"src/a.cc": "1\n2\nthree\n4\n", "src/b.cc": "b\nb\n"}, "pr")
    monkeypatch.setenv("PR_BASE_SHA", base)
    monkeypatch.setenv("PR_HEAD_SHA", head)
    if github_sha:
        monkeypatch.setenv("GITHUB_SHA", github_sha)
    build = git_repo / "build"
    build.mkdir()
    (build / "compile_commands.json").write_text(
        json.dumps(
            [{"directory": str(git_repo), "file": f"src/{name}"} for name in ("a.cc", "b.cc")]
        )
    )
    src = git_repo / "src"
    outputs = {
        "a.cc": f"{src}/a.cc:2:1: warning: old [check-old]\n"
        f"{src}/a.cc:3:5: warning: new [check-new]\n"
        "    3 | three\n"
        "      |     ^\n"
        f"{src}/shared.h:1:1: warning: header [check-header]\n"
        # Another unit repeats one of b.cc's diagnostics; the repeat collapses.
        f"{src}/b.cc:2:1: warning: added file [check-added]\n",
        "b.cc": f"{src}/b.cc:2:1: warning: added file [check-added]\n"
        f"{src}/b.cc:2:1: warning: other message [check-added]\n",
    }

    def run(command, **kwargs):
        return CompletedProcess(command, 0, outputs[Path(command[-1]).name], "")

    analyzer = ClangTidyAnalyzer(git_repo, build)
    with (
        patch.object(analyzer, "require_tool", return_value=True),
        patch("analyzers.compiler.run_captured", side_effect=run),
    ):
        analyzer.run([src / "a.cc", src / "b.cc"])

    report = json.loads((build / "clang-tidy-diagnostics.json").read_text())
    # Line numbers are those of the checkout: GITHUB_SHA in CI, else HEAD.
    assert report["commit"] == (github_sha or head)
    assert len(report["diagnostics"]["src/a.cc"]) == 2
    # Only line 3 of a.cc changed, b.cc is new, and the header did not change.
    # Changed rows add the message and the source excerpt printed under the diagnostic.
    assert report["changed"] == {
        "src/a.cc": [[3, 5, "warning", "check-new", "new", "    3 | three\n      |     ^"]],
        # Distinct messages at one location each keep their own row.
        "src/b.cc": [
            [2, 1, "warning", "check-added", "added file", ""],
            [2, 1, "warning", "check-added", "other message", ""],
        ],
    }
    assert len(report["diagnostics"]["src/b.cc"]) == 1


@pytest.mark.parametrize("checkout", ["no repository", "no commits", "no git"])
def test_diagnostics_without_a_checked_out_commit_name_none(tmp_path, isolated_git, checkout):
    if checkout == "no commits":
        git(tmp_path, "init", "-q")
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    with (
        patch.object(analyzer, "require_compile_commands", return_value=True),
        patch.object(analyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured",
            return_value=CompletedProcess([], 0, "file.cc:1:2: warning: w [check]\n", ""),
        ),
        patch("common.git.run_captured", side_effect=FileNotFoundError("git"))
        if checkout == "no git"
        else nullcontext(),
    ):
        analyzer.run([tmp_path / "file.cc"])
    assert json.loads((tmp_path / "clang-tidy-diagnostics.json").read_text()) == {
        "tool": "clang-tidy",
        "shard": 1,
        "diagnostics": {"file.cc": [[1, 2, "warning", "check"]]},
    }


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


MAVLINK_TEST_HEADER = "build/_deps/mavlink-build/include/mavlink/test/mavlink_msg_test_types.h"


def _header_filter(analyzer: ClangTidyAnalyzer) -> re.Pattern[str]:
    """Return the header filter of the analyzer's runs: its argument, else .clang-tidy's."""
    for argument in analyzer._tool_arguments():
        if argument.startswith("--header-filter="):
            return re.compile(argument.partition("=")[2])
    config = (REPO_ROOT / ".clang-tidy").read_text(encoding="utf-8")
    configured = re.search(r"^HeaderFilterRegex: '(.+)'$", config, re.M)
    assert configured is not None
    return re.compile(configured[1])


@pytest.mark.parametrize(
    ("header", "reported"),
    [
        ("{checkout}/src/Vehicle/Vehicle.h", True),
        ("{checkout}/test/UnitTestFramework/UnitTest.h", True),
        ("{checkout}/tools/coding-style/CodingStyle.h", True),
        # MAVLink's test dialect in a local build tree, and in CI's ${RUNNER_TEMP}/build.
        ("{checkout}/" + MAVLINK_TEST_HEADER, False),
        ("{outside}/" + MAVLINK_TEST_HEADER, False),
        # CPM sources in the default source cache, and in one outside the checkout.
        ("{checkout}/.cache/CPM/sdl3/1/src/SDL_internal.h", False),
        ("{outside}/CPM/sdl3/1/src/SDL_internal.h", False),
        ("{checkout}/build/src/generated.h", False),
    ],
)
def test_tidy_reports_only_this_checkout_s_headers(tmp_path, header, reported):
    checkout = (tmp_path / "qgroundcontrol").resolve()
    outside = (tmp_path / "runner-temp").resolve()
    analyzer = ClangTidyAnalyzer(checkout, checkout / "build")
    name = header.format(checkout=checkout.as_posix(), outside=outside.as_posix())
    assert bool(_header_filter(analyzer).match(name)) is reported


def test_tidy_header_filter_anchors_the_configured_header_filter():
    config = (REPO_ROOT / ".clang-tidy").read_text(encoding="utf-8")
    match = re.search(
        r"^HeaderFilterRegex: '\^\.\*/\(([^)]*)\)/\.\*\\\.\(h\|hpp\)\$'$", config, re.M
    )
    assert match is not None
    assert tuple(match.group(1).split("|")) == HEADER_DIRS


@pytest.mark.skipif(sys.platform == "win32", reason="the fake tool is a POSIX script")
def test_tidy_results_are_cached_under_the_header_filter_and_the_config(tmp_path, monkeypatch):
    # Master pushes seed the result cache that pull requests replay.
    (tmp_path / ".clang-tidy").write_text("Checks: bugprone-*\n", encoding="utf-8")
    tool = tmp_path / "bin/clang-tidy"
    tool.parent.mkdir()
    tool.write_text(f"#!{sys.executable}\nprint('version 1')\n", encoding="utf-8")
    tool.chmod(0o755)
    monkeypatch.setenv("PATH", str(tool.parent))
    analyzer = ClangTidyAnalyzer(tmp_path, tmp_path)
    identity = analyzer._cache_identity()
    assert f"--header-filter={_header_filter(analyzer).pattern}" in identity
    assert "Checks: bugprone-*\n" in identity


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
            patch(
                "analyzers.compiler.scan_dependencies",
                side_effect=_scan_including(tmp_path / "src/header.h", dependents),
            ),
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
    args = ["--tool", "clang-tidy", "-b", ".", "--advisory", "file.cc"]
    if profile_checks:
        args.append("--profile-checks")
    monkeypatch.setattr(analyze, "find_repo_root", lambda: tmp_path)
    with (
        patch.object(ClangTidyAnalyzer, "require_tool", return_value=True),
        patch(
            "analyzers.compiler.run_captured", return_value=CompletedProcess([], 0, "", "")
        ) as run,
    ):
        assert analyze.main(args) == 0
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
def test_invalid_analysis_modes_are_rejected(arguments):
    with pytest.raises(SystemExit) as exc:
        analyze.parse_args(arguments)
    assert exc.value.code == 2
