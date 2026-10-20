"""Inline exports reuse compiler execution without changing selection or exit status."""

import json
import sys
from subprocess import CompletedProcess, TimeoutExpired
from unittest.mock import Mock

import analyze
import pytest
from analyzers.clang_tidy import ClangTidyAnalyzer
from analyzers.clazy import ClazyAnalyzer
from analyzers.review import MAX_FINDINGS, ReviewFindings


def test_collector_deduplicates_tus_and_filters_changed_lines(tmp_path):
    source = tmp_path / "src/new é.h"
    collector = ReviewFindings(tmp_path, {source: [(5, 5)]})
    output = (
        f"{source}:5:2: warning: avoid copy [clazy-range-loop]\n"
        "  5 | for (auto item : items)\n    |      ^\n"
        f"{source}:6:2: warning: unrelated [clazy-range-loop]\n"
        f"{source}:5:2: note: note without check\n"
        f"{tmp_path.parent}/external.h:5:2: warning: external [check]\n"
    )
    collector.collect(output, tmp_path)
    collector.collect(output.replace(str(source), "src/new é.h"), tmp_path / "build")
    assert len(collector.findings) == 1
    finding = next(iter(collector.findings.values()))
    assert finding["path"] == "src/new é.h" and finding["line"] == 5
    assert finding["check"] == "clazy-range-loop" and "for (auto" in finding["context"]


def test_collector_limits_and_colors(tmp_path):
    source = tmp_path / "file.cc"
    collector = ReviewFindings(tmp_path, {source: [(1, 100)]})
    collector.collect(
        "\n".join(
            f"\x1b[31m{source}:{line}:2: warning: {'m' * 2100} [check]\x1b[0m"
            for line in range(1, 101)
        ),
        tmp_path,
    )
    assert len(collector.findings) == MAX_FINDINGS and collector.truncated
    assert all(len(item["message"]) == 2000 for item in collector.findings.values())


@pytest.mark.parametrize("analyzer_class", [ClazyAnalyzer, ClangTidyAnalyzer])
@pytest.mark.parametrize("mode", ["warning", "error", "timeout", "empty", "inactive"])
def test_cli_exports_same_pass_and_preserves_errors(tmp_path, monkeypatch, analyzer_class, mode):
    source = tmp_path / "file.cc"
    source.write_text("int value;\n")
    (tmp_path / "compile_commands.json").write_text(
        json.dumps(
            []
            if mode == "inactive"
            else [{"directory": str(tmp_path), "file": "file.cc", "command": "c++ -c file.cc"}]
        )
    )
    event = tmp_path / "event.json"
    event.write_text(json.dumps({"pull_request": {"number": 42, "head": {"sha": "a" * 40}}}))
    for key, value in {
        "GITHUB_EVENT_PATH": str(event),
        "GITHUB_EVENT_NAME": "pull_request",
        "GITHUB_REPOSITORY": "mavlink/qgroundcontrol",
        "GITHUB_RUN_ID": "100",
        "GITHUB_RUN_ATTEMPT": "2",
    }.items():
        monkeypatch.setenv(key, value)
    destination = tmp_path / "review/report.json"
    analyzer = analyzer_class(tmp_path, tmp_path, jobs=2)
    monkeypatch.setattr(analyzer, "require_compile_commands", lambda: True)
    monkeypatch.setattr(analyzer, "require_tool", lambda tool: True)
    monkeypatch.setattr(analyze, "get_analyzer", lambda *args, **kwargs: analyzer)
    monkeypatch.setattr(analyze, "find_repo_root", lambda: tmp_path)
    monkeypatch.setattr(
        analyze,
        "get_changed_line_ranges",
        lambda *args: {} if mode == "empty" else {source: [(1, 1)]},
    )
    level = "error" if mode == "error" else "warning"
    diagnostic = f"file.cc:1:1: {level}: avoid copy [check]\n"
    command = Mock(
        return_value=CompletedProcess([], int(mode == "error"), diagnostic, ""),
        side_effect=TimeoutExpired("analyzer", 300) if mode == "timeout" else None,
    )
    monkeypatch.setattr("analyzers.compiler.run_captured", command)
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "analyze.py",
            "--tool",
            analyzer.name,
            "--advisory",
            "--diff-base",
            "base",
            "--review-output",
            str(destination),
        ],
    )
    assert analyze.main() == (2 if mode in {"error", "timeout"} else 0)
    report = json.loads(destination.read_text())
    assert report["manifest"]["run_id"] == 100 and report["manifest"]["run_attempt"] == 2
    assert report["manifest"]["pr_number"] == 42 and report["manifest"]["head_sha"] == "a" * 40
    assert report["manifest"]["incomplete"] == (mode in {"error", "timeout"})
    assert len(report["findings"]) == (1 if mode in {"error", "warning"} else 0)
    assert command.call_count == (0 if mode in {"empty", "inactive"} else 1)
    if mode in {"error", "warning"}:
        assert diagnostic in (tmp_path / f"{analyzer.name}-raw.txt").read_text()


def test_review_export_requires_diff(monkeypatch):
    monkeypatch.setattr(sys, "argv", ["analyze.py", "--review-output", "report.json"])
    with pytest.raises(SystemExit, match="2"):
        analyze.parse_args()
