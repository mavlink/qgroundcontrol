"""Tests for tools/analyze.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock, patch

from analyze import FileCollector, get_analyzer, validate_path


class TestFileCollector:
    def test_get_compare_ref_symbolic(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        mock_result = MagicMock(returncode=0, stdout="origin/main\n")
        with patch("common.proc.subprocess.run", return_value=mock_result):
            ref = collector.get_compare_ref()
        assert ref == "main"

    def test_get_compare_ref_fallback_master(self, tmp_path: Path) -> None:
        collector = FileCollector(tmp_path)
        symbolic_fail = MagicMock(returncode=1, stdout="")
        master_ok = MagicMock(returncode=0)

        def side_effect(cmd, **kw):
            if "symbolic-ref" in cmd:
                return symbolic_fail
            if cmd[-1] == "master":
                return master_ok
            return MagicMock(returncode=1)

        with patch("common.proc.subprocess.run", side_effect=side_effect):
            ref = collector.get_compare_ref()
        assert ref == "master"

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


class TestValidatePath:
    def test_valid_relative_path(self, tmp_path: Path) -> None:
        (tmp_path / "src").mkdir()
        result = validate_path("src", tmp_path)
        assert result == Path("src")

    def test_rejects_parent_traversal(self, tmp_path: Path) -> None:
        import pytest

        with pytest.raises(ValueError, match="must not contain"):
            validate_path("../etc/passwd", tmp_path)

    def test_rejects_absolute_path(self, tmp_path: Path) -> None:
        import pytest

        with pytest.raises(ValueError, match="must be relative"):
            validate_path("/etc/passwd", tmp_path)


class TestGetAnalyzer:
    def test_returns_clang_tidy(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("clang-tidy", tmp_path, tmp_path / "build", jobs=4)
        assert analyzer.name == "clang-tidy"
        assert analyzer.jobs == 4  # type: ignore[attr-defined]

    def test_returns_clazy_with_jobs(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("clazy", tmp_path, tmp_path / "build", jobs=2)
        assert analyzer.name == "clazy"
        assert analyzer.jobs == 2  # type: ignore[attr-defined]

    def test_returns_cppcheck(self, tmp_path: Path) -> None:
        analyzer = get_analyzer("cppcheck", tmp_path, tmp_path / "build")
        assert analyzer.name == "cppcheck"

    def test_unknown_tool_raises(self, tmp_path: Path) -> None:
        import pytest

        with pytest.raises(ValueError, match="Unknown tool"):
            get_analyzer("nonexistent", tmp_path, tmp_path / "build")


def test_format_execution_failure_is_not_success(tmp_path):
    import subprocess
    from unittest.mock import patch

    from analyzers.clang_format import ClangFormatAnalyzer

    failure = subprocess.CompletedProcess([], 1, "", "permission denied")
    with (
        patch("analyzers.clang_format.run_captured", return_value=failure),
        patch("analyzers.clang_format.run_git", return_value=failure),
    ):
        result = ClangFormatAnalyzer(tmp_path, tmp_path)._run_fix([tmp_path / "file.cc"])
    assert not result.passed
    assert result.execution_error


def test_missing_qmllint_is_execution_failure(tmp_path):
    from unittest.mock import patch

    from analyzers.qmllint import QmlLintAnalyzer

    with patch("analyzers.qmllint.shutil.which", return_value=None):
        result = QmlLintAnalyzer(tmp_path, tmp_path).run([tmp_path / "file.qml"])
    assert not result.passed
    assert result.execution_error


def test_advisory_preserves_error_findings(tmp_path, monkeypatch):
    import sys

    import analyze
    from common.analyzer import AnalysisResult

    monkeypatch.setattr(sys, "argv", ["analyze.py", "--tool", "clazy", "--advisory"])
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
        assert analyze.main() == expected


def test_build_aware_qml_keeps_sdk_imports_and_fails_errors(tmp_path, monkeypatch):
    from subprocess import CompletedProcess

    from analyzers.qmllint import QmlLintAnalyzer

    imports = tmp_path / "qml"
    imports.mkdir()
    (imports / "fixture.qmltypes").touch()
    analyzer = QmlLintAnalyzer(tmp_path, tmp_path)
    analyzer.build_aware = True
    monkeypatch.setattr("analyzers.qmllint.shutil.which", lambda _: "/qt/bin/qmllint")
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
    monkeypatch.setattr("analyzers.qmllint.shutil.which", lambda _: "/qt/bin/qmllint")
    assert analyzer.run([tmp_path / "fixture.qml"]).execution_error
