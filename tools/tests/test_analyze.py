"""Path, file-selection, analyzer-factory, and parallel-run contracts."""

from __future__ import annotations

from pathlib import Path

import common.proc as proc
import pytest
from analyze import FileCollector, get_analyzer, validate_path
from common.analyzer import AnalysisResult, AnalyzerBase, FileAnalysis

from ._helpers import completed


class _Analyzer(AnalyzerBase):
    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        return AnalysisResult(tool="test", passed=True, files_checked=len(files))


def test_file_collector_resolves_refs_and_filters_extensions(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    collector = FileCollector(tmp_path)
    monkeypatch.setattr(
        proc.subprocess, "run", lambda *_args, **_kwargs: completed("origin/main\n")
    )
    assert collector.get_compare_ref() == "main"

    def master_fallback(command, **_kwargs):
        if "symbolic-ref" in command:
            return completed(returncode=1)
        return completed(returncode=0 if command[-1] == "master" else 1)

    monkeypatch.setattr(proc.subprocess, "run", master_fallback)
    assert collector.get_compare_ref() == "master"
    monkeypatch.setattr(proc.subprocess, "run", lambda *_args, **_kwargs: completed(returncode=1))
    assert collector.get_compare_ref() is None

    src = tmp_path / "src"
    src.mkdir()
    for name in ("foo.cpp", "bar.h", "readme.txt"):
        (src / name).touch()
    assert {path.name for path in collector._find_files(src, (".cpp", ".h"))} == {
        "foo.cpp",
        "bar.h",
    }


def test_validate_path_accepts_repo_relative_and_rejects_escape(tmp_path: Path) -> None:
    (tmp_path / "src").mkdir()
    assert validate_path("src", tmp_path) == Path("src")
    with pytest.raises(ValueError, match="must not contain"):
        validate_path("../etc/passwd", tmp_path)
    with pytest.raises(ValueError, match="must be relative"):
        validate_path("/etc/passwd", tmp_path)


def test_analyzer_factory_constructs_supported_tools(tmp_path: Path) -> None:
    cases = (("clang-tidy", 4), ("clazy", 2), ("cppcheck", None))
    for name, jobs in cases:
        analyzer = get_analyzer(name, tmp_path, tmp_path / "build", jobs=jobs or 0)
        assert analyzer.name == name
        if jobs is not None:
            assert analyzer.jobs == jobs  # type: ignore[attr-defined]
    with pytest.raises(ValueError, match="Unknown tool"):
        get_analyzer("nonexistent", tmp_path, tmp_path / "build")


def test_parallel_analyzer_orders_results_and_surfaces_errors(tmp_path: Path) -> None:
    analyzer = _Analyzer(tmp_path, tmp_path / "build")
    results = analyzer.run_files_parallel(
        [tmp_path / "z.cpp", tmp_path / "a.cpp"],
        lambda path: FileAnalysis(path=path.name, has_issues=path.name.startswith("z")),
        jobs=2,
    )
    assert [(result.path, result.has_issues) for result in results] == [
        ("a.cpp", False),
        ("z.cpp", True),
    ]

    def fail(path: Path) -> FileAnalysis:
        raise RuntimeError(f"failed {path.name}")

    with pytest.raises(RuntimeError, match=r"failed broken\.cpp"):
        analyzer.run_files_parallel([tmp_path / "broken.cpp"], fail, jobs=1)
