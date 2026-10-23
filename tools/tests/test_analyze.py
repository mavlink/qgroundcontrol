"""Tests for tools/analyze.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock, patch

import analyze
import pytest
from analyze import FileCollector, get_analyzer, validate_path

from ._helpers import commit_files, git, pr_merge_checkout


class TestFileCollector:
    def test_get_compare_ref_symbolic(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        mock_result = MagicMock(returncode=0, stdout="origin/main\n")
        with patch("common.proc.subprocess.run", return_value=mock_result):
            ref = collector.get_compare_ref()
        assert ref == "main"

    @pytest.mark.parametrize(("branch", "expected"), [("master", "master"), ("trunk", None)])
    def test_get_compare_ref_uses_the_default_branch(
        self, git_repo: Path, monkeypatch: pytest.MonkeyPatch, branch: str, expected: str | None
    ) -> None:
        commit_files(git_repo, {"a.cc": "int a;\n"}, "base")
        git(git_repo, "branch", "-m", branch)
        git(git_repo, "checkout", "-qb", "feature")
        monkeypatch.delenv("PR_BASE_SHA", raising=False)
        monkeypatch.delenv("PR_HEAD_SHA", raising=False)
        assert FileCollector(git_repo).get_compare_ref() == expected

    @pytest.mark.parametrize("clone", ["full", "shallow"])
    def test_pr_merge_checkout_selects_only_the_pr_sources(
        self, tmp_path: Path, isolated_git: None, monkeypatch: pytest.MonkeyPatch, clone: str
    ) -> None:
        pr = pr_merge_checkout(
            tmp_path,
            base={"src/a.cc": "int a;\n"},
            newer_base={"src/a.cc": "int a = 1;\n", "src/base.cc": "int b;\n"},
            pr={"src/pr.cc": "int p;\n"},
        )
        monkeypatch.setenv("PR_BASE_SHA", pr.fork_point)
        monkeypatch.setenv("PR_HEAD_SHA", pr.pr_head)
        collector = FileCollector(getattr(pr, clone))
        assert [path.name for path in collector.get_cpp_files()] == ["pr.cc"]
        assert collector.diff_base == pr.base_tip

    def test_get_compare_ref_none(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        fail = MagicMock(returncode=1, stdout="")
        with patch("common.proc.subprocess.run", return_value=fail):
            ref = collector.get_compare_ref()
        assert ref is None

    def test_find_files(self, tmp_path: Path) -> None:
        src = tmp_path / "src"
        src.mkdir()
        (src / "foo.cpp").touch()
        (src / "bar.h").touch()
        (src / "readme.txt").touch()

        collector = FileCollector(tmp_path)
        files = collector._find_files(src, (".cpp", ".h"))
        names = [f.name for f in files]
        assert "foo.cpp" in names
        assert "bar.h" in names
        assert "readme.txt" not in names

    def test_changed_files_include_worktree_and_untracked(self, git_repo: Path) -> None:
        sources = dict.fromkeys(("base.cc", "edited.cc", "deleted.cc", "committed.cc"), "int x;\n")
        commit_files(git_repo, {**sources, ".gitignore": "ignored.cc\n"}, "base")
        git(git_repo, "checkout", "-q", "-b", "feature")
        (git_repo / "committed.cc").write_text("int y;\n")
        git(git_repo, "commit", "-q", "-am", "feature")
        (git_repo / "edited.cc").write_text("int z;\n")
        (git_repo / "deleted.cc").unlink()
        (git_repo / "untracked.cc").write_text("int u;\n")
        (git_repo / "ignored.cc").write_text("int i;\n")
        (git_repo / "notes.txt").write_text("skip\n")

        files = FileCollector(git_repo)._get_changed_files((".cc",), "master")

        assert [f.name for f in files] == ["committed.cc", "edited.cc", "untracked.cc"]


class TestValidatePath:
    def test_valid_relative_path(self, tmp_path: Path) -> None:
        (tmp_path / "src").mkdir()
        result = validate_path("src", tmp_path)
        assert result == Path("src")

    def test_rejects_parent_traversal(self, tmp_path: Path) -> None:
        with pytest.raises(ValueError, match="outside the repository"):
            validate_path("../etc/passwd", tmp_path)

    def test_rejects_absolute_path_outside_the_repository(self, tmp_path: Path) -> None:
        with pytest.raises(ValueError, match="outside the repository"):
            validate_path(str(tmp_path.parent), tmp_path)

    def test_accepts_paths_that_stay_inside_the_repository(self, tmp_path: Path) -> None:
        assert validate_path(str(tmp_path / "src" / "Vehicle"), tmp_path) == Path("src/Vehicle")
        assert validate_path("src/../test", tmp_path) == Path("test")


class TestGetAnalyzer:
    def test_returns_clang_tidy(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("clang-tidy", tmp_path, tmp_path / "build", jobs=4)
        assert analyzer.name == "clang-tidy"
        assert analyzer.jobs == 4  # type: ignore[attr-defined]

    def test_returns_clazy_with_jobs(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("clazy", tmp_path, tmp_path / "build", jobs=2)
        assert analyzer.name == "clazy"
        assert analyzer.jobs == 2  # type: ignore[attr-defined]

    def test_unknown_tool_raises(self, tmp_path: Path) -> None:
        with pytest.raises(ValueError, match="Unknown tool"):
            get_analyzer("nonexistent", tmp_path, tmp_path / "build")

    @pytest.mark.parametrize("tool", analyze.ANALYZERS)
    def test_every_tool_loads_its_analyzer(self, tmp_path: Path, tool: str) -> None:
        assert get_analyzer(tool, tmp_path, tmp_path / "build").name == tool

    def test_help_lists_every_tool(self, capsys: pytest.CaptureFixture[str]) -> None:
        with pytest.raises(SystemExit):
            analyze.parse_args(["--help"])
        out = capsys.readouterr().out
        for name, tool in analyze.ANALYZERS.items():
            assert f"  {name:<24} {tool.help}\n" in out


def test_missing_qmllint_is_execution_failure(tmp_path):
    from unittest.mock import patch

    from analyzers.qmllint import QmlLintAnalyzer

    with patch("common.proc.shutil.which", return_value=None):
        result = QmlLintAnalyzer(tmp_path, tmp_path).run([tmp_path / "file.qml"])
    assert not result.passed
    assert result.execution_error


def test_advisory_preserves_error_findings(tmp_path, monkeypatch):

    from analyzers.base import AnalysisResult

    monkeypatch.setattr(analyze, "find_repo_root", lambda: tmp_path)
    monkeypatch.setattr(analyze.FileCollector, "get_cpp_files", lambda *a, **kw: [])
    analyzer = MagicMock()
    monkeypatch.setattr(analyze, "get_analyzer", lambda *a, **kw: analyzer)
    for error_findings, execution_error, expected in [
        (False, False, 0),
        (True, False, 2),
        (False, True, 2),
    ]:
        analyzer.run.return_value = AnalysisResult(
            tool="clazy",
            passed=False,
            error_findings=error_findings,
            execution_error=execution_error,
        )
        assert analyze.main(["--tool", "clazy", "--advisory"]) == expected


def test_step_summary_reports_each_tool(tmp_path, monkeypatch, gh_summary, snapshot):
    from analyzers.base import AnalysisResult

    monkeypatch.setattr(analyze, "find_repo_root", lambda: tmp_path)
    monkeypatch.setattr(analyze.FileCollector, "get_cpp_files", lambda *a, **kw: [])
    analyzer = MagicMock()
    analyzer.run.return_value = AnalysisResult(
        tool="clazy",
        passed=False,
        files_checked=150,
        files_with_issues=["src/Vehicle/Vehicle.cc", "src/main.cc"],
        output=(
            "src/main.cc:11:1: warning: non-POD static (QGCLoggingCategory) "
            "[-Wclazy-non-pod-global-static]\n"
            "src/Vehicle/Vehicle.cc:5:10: warning: Module QtQmlIntegration should not be included "
            "directly [-Wclazy-no-module-include]\n"
            "src/Vehicle/Vehicle.cc:9:3: warning: unused [-Wclazy-non-pod-global-static]\n"
        ),
    )
    monkeypatch.setattr(analyze, "get_analyzer", lambda *a, **kw: analyzer)
    assert analyze.main(["--tool", "clazy", "--advisory", "--step-summary"]) == 0
    assert gh_summary.read_text(encoding="utf-8") == snapshot
    gh_summary.write_text("")
    assert analyze.main(["--tool", "clazy", "--advisory"]) == 0
    assert gh_summary.read_text(encoding="utf-8") == ""


def _late_errors(root: Path, count: int) -> str:
    """Clang-tidy output whose errors follow more warnings than the summary lists."""
    warnings = "".join(
        f"{root}/src/w{index}.cc:1:1: warning: unused [misc-unused]\n" for index in range(400)
    )
    errors = "".join(
        f"{root}/src/KMLSchemaValidator.cc:{100 + index}:13: error: incomplete type "
        "'QGeoCoordinate' named in nested name specifier [clang-diagnostic-error]\n"
        for index in range(count)
    )
    return warnings + errors


def _run_clang_tidy(monkeypatch: pytest.MonkeyPatch, root: Path, output: str, *args: str) -> int:
    from analyzers.base import AnalysisResult

    monkeypatch.setattr(analyze, "find_repo_root", lambda: root)
    monkeypatch.setattr(analyze.FileCollector, "get_cpp_files", lambda *a, **kw: [])
    analyzer = MagicMock()
    analyzer.run.return_value = AnalysisResult(
        tool="clang-tidy", passed=False, files_checked=401, error_findings=True, output=output
    )
    monkeypatch.setattr(analyze, "get_analyzer", lambda *a, **kw: analyzer)
    return analyze.main(["--tool", "clang-tidy", *args])


def test_error_diagnostics_are_unique_and_repository_relative(tmp_path: Path) -> None:
    output = (
        f"{tmp_path}/src/a.cc:3:5: error: no member named 'x' [clang-diagnostic-error]\n"
        f"{tmp_path}/src/a.cc:3:5: error: no member named 'x' [clang-diagnostic-error]\n"
        f"{tmp_path}/src/a.cc:4:1: warning: unused [misc-unused]\n"
        "src/b.h:7:10: fatal error: 'QtCore/QObject' file not found\n"
        "/usr/include/c++/14/bits/x.h:9:2: error: static assertion failed\n"
    )
    assert analyze.error_diagnostics(output, tmp_path) == [
        analyze.ErrorDiagnostic("src/a.cc", 3, 5, "no member named 'x'", "clang-diagnostic-error"),
        analyze.ErrorDiagnostic("src/b.h", 7, 10, "'QtCore/QObject' file not found", ""),
        analyze.ErrorDiagnostic(
            "/usr/include/c++/14/bits/x.h", 9, 2, "static assertion failed", ""
        ),
    ]


def test_step_summary_counts_errors_and_lists_them_before_the_truncated_diagnostics(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    assert _run_clang_tidy(monkeypatch, tmp_path, _late_errors(tmp_path, 25), "--step-summary") == 2
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Errors | 25 |\n| Diagnostics | 425 |" in summary
    listed = [
        f"src/KMLSchemaValidator.cc:{100 + index}:13: error: incomplete type 'QGeoCoordinate' "
        "named in nested name specifier [clang-diagnostic-error]"
        for index in range(20)
    ]
    errors = "Errors:\n\n```\n" + "\n".join(listed) + "\n…and 5 more\n```"
    assert errors in summary
    # Before every collapsed listing, including the diagnostics that stop before any error.
    assert summary.index(errors) < summary.index("<details>")
    assert "KMLSchemaValidator" not in summary.split("<summary>Diagnostics (unique)</summary>")[1]


def test_ci_annotates_the_first_ten_errors_on_their_lines(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    output = (
        "/usr/include/x.h:9:2: error: static assertion failed: a, b\n"
        f"{tmp_path}/src/b.h:7:10: fatal error: 'QtCore/QObject' file not found\n"
        f"{tmp_path}/src/c.cc:5:1: error: 100% moved [bugprone-use-after-move,-warnings-as-errors]\n"
        + _late_errors(tmp_path, 12)
    )
    assert _run_clang_tidy(monkeypatch, tmp_path, output) == 2
    assert not [line for line in capsys.readouterr().out.splitlines() if line.startswith("::")]

    monkeypatch.setenv("GITHUB_ACTIONS", "true")
    assert _run_clang_tidy(monkeypatch, tmp_path, output) == 2
    annotations = [line for line in capsys.readouterr().out.splitlines() if line.startswith("::")]
    assert annotations[:4] == [
        "::error title=clang-tidy::/usr/include/x.h:9:2: error: static assertion failed: a, b",
        "::error file=src/b.h,line=7,col=10,title=clang-tidy::'QtCore/QObject' file not found",
        "::error file=src/c.cc,line=5,col=1,"
        "title=clang-tidy [bugprone-use-after-move%2C-warnings-as-errors]::100%25 moved",
        "::error file=src/KMLSchemaValidator.cc,line=100,col=13,"
        "title=clang-tidy [clang-diagnostic-error]::incomplete type 'QGeoCoordinate' named in "
        "nested name specifier",
    ]
    assert len(annotations) == 10
    assert annotations[-1].startswith("::error file=src/KMLSchemaValidator.cc,line=106,")


def test_execution_errors_print_their_reason(tmp_path, monkeypatch, capsys):
    from analyzers.base import AnalysisResult

    monkeypatch.setattr(analyze, "find_repo_root", lambda: tmp_path)
    monkeypatch.setattr(analyze.FileCollector, "get_cpp_files", lambda *a, **kw: [])
    analyzer = MagicMock()
    reason = "clang-format 23.1.0 required; activate tools/.venv or install the lint group"
    analyzer.run.return_value = AnalysisResult(
        tool="clang-format", passed=False, execution_error=True, output=reason
    )
    monkeypatch.setattr(analyze, "get_analyzer", lambda *a, **kw: analyzer)
    assert analyze.main(["--tool", "clang-format"]) == 2
    assert f"clang-format: error (0 files): {reason}\n" in capsys.readouterr().out


def test_build_aware_qml_keeps_sdk_imports_and_fails_errors(tmp_path, monkeypatch):
    from subprocess import CompletedProcess

    from analyzers.qmllint import QmlLintAnalyzer

    imports = tmp_path / "qml"
    imports.mkdir()
    (imports / "fixture.qmltypes").touch()
    analyzer = QmlLintAnalyzer(tmp_path, tmp_path)
    analyzer.build_aware = True
    monkeypatch.setattr("common.proc.shutil.which", lambda _: "/qt/bin/qmllint")
    commands = []

    def run(command):
        commands.append(command)
        return CompletedProcess(command, 1, "Error: unknown property [missing-property]", "")

    monkeypatch.setattr("analyzers.qmllint.run_captured", run)
    result = analyzer.run([tmp_path / "fixture.qml"])
    assert result.error_findings
    assert "--bare" not in commands[0]
    assert str(imports) in commands[0]
    assert commands[0][commands[0].index("--missing-property") + 1] == "error"


def test_build_aware_qml_requires_generated_types(tmp_path, monkeypatch):
    from analyzers.qmllint import QmlLintAnalyzer

    analyzer = QmlLintAnalyzer(tmp_path, tmp_path)
    analyzer.build_aware = True
    monkeypatch.setattr("common.proc.shutil.which", lambda _: "/qt/bin/qmllint")
    assert analyzer.run([tmp_path / "fixture.qml"]).execution_error


def test_missing_repo_root_is_reported(monkeypatch):

    def no_root():
        raise RuntimeError("Could not find the QGroundControl repository root above /x")

    monkeypatch.setattr(analyze, "find_repo_root", no_root)
    with patch.object(analyze, "log_error") as log_error:
        assert analyze.main(["--tool", "clang-format"]) == 1
    log_error.assert_called_once_with("Could not find the QGroundControl repository root above /x")
