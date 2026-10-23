"""Tests for qgc_tools/build_profile.py and the tools/build_profile.py CLI."""

from __future__ import annotations

import json
import os
from pathlib import Path

import build_profile as build_profile_cli
import pytest
from common.build_config import load_build_config
from qgc_tools.build_profile import (
    BuildEdge,
    TimeTrace,
    build_report,
    classify_output,
    find_time_traces,
    parse_ninja_log,
    parse_time_trace,
    summarize_ninja_log,
    write_snapshot,
)

from ._helpers import get_step, get_steps, load_workflow


def test_parse_ninja_log_reads_edges_and_skips_header(tmp_path: Path) -> None:
    log = tmp_path / ".ninja_log"
    log.write_text(
        "# ninja log v5\n"
        "10\t60\t0\tCMakeFiles/app.dir/src/main.cc.o\tabc\n"
        "80\t130\t0\tqml/Foo.qmlc\tdef\n",
        encoding="utf-8",
    )

    edges = parse_ninja_log(log)

    assert edges == [
        BuildEdge(
            output="CMakeFiles/app.dir/src/main.cc.o",
            start_ms=10,
            end_ms=60,
            mtime=0,
            command_hash="abc",
        ),
        BuildEdge(output="qml/Foo.qmlc", start_ms=80, end_ms=130, mtime=0, command_hash="def"),
    ]
    assert edges[0].duration_ms == 50


def test_summarize_ninja_log_reports_slowest_rebuilds_and_generated_steps() -> None:
    edges = [
        BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 0, 120, 0, "a"),
        BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 200, 330, 0, "b"),
        BuildEdge("src/Foo_autogen/mocs_compilation.cpp.o", 0, 90, 0, "c"),
        BuildEdge(".rcc/qmlcache/Foo_qml.cpp.o", 0, 180, 0, "d"),
        BuildEdge("Release/lib/libQGC.a", 0, 220, 0, "e"),
    ]

    summary = summarize_ninja_log(edges, limit=2)

    assert [edge.output for edge in summary.slowest_edges] == [
        "Release/lib/libQGC.a",
        ".rcc/qmlcache/Foo_qml.cpp.o",
    ]
    assert summary.rebuilt_outputs[0].output == "CMakeFiles/app.dir/src/main.cc.o"
    assert summary.rebuilt_outputs[0].count == 2
    assert [edge.output for edge in summary.generated_edges] == [
        ".rcc/qmlcache/Foo_qml.cpp.o",
        "src/Foo_autogen/mocs_compilation.cpp.o",
    ]


def test_classify_output_identifies_common_build_hotspots() -> None:
    assert classify_output(".rcc/qmlcache/Foo_qml.cpp.o") == "qmlcache"
    assert classify_output("src/Foo_autogen/mocs_compilation.cpp.o") == "autogen/moc"
    assert classify_output("src/.qt/rcc/qrc_resources.cpp.o") == "rcc"
    assert classify_output("src/qgroundcontrol_qmltyperegistrations.cpp.o") == "qmltyperegistration"
    assert classify_output("Release/lib/libQGC.a") == "link/archive"
    assert classify_output("CMakeFiles/app.dir/src/main.cc.o") == "compile"
    assert classify_output("CMakeFiles/app.dir/Release/main.cc.obj") == "compile"
    assert classify_output("Release/QGroundControl") == "link/archive"


def test_parse_time_trace_reads_compile_duration_and_expensive_events(tmp_path: Path) -> None:
    trace = tmp_path / "main.json"
    trace.write_text(
        json.dumps(
            {
                "traceEvents": [
                    {
                        "ph": "X",
                        "name": "ExecuteCompiler",
                        "dur": 250000,
                        "args": {"detail": "main.cc"},
                    },
                    {
                        "ph": "X",
                        "name": "Source",
                        "dur": 90000,
                        "args": {"detail": "QtCore/QObject"},
                    },
                    {"ph": "X", "name": "ParseClass", "dur": 50000, "args": {"detail": "Vehicle"}},
                ]
            }
        ),
        encoding="utf-8",
    )

    parsed = parse_time_trace(trace)

    assert parsed == TimeTrace(
        path=trace,
        total_ms=250.0,
        top_events=[("Source: QtCore/QObject", 90.0), ("ParseClass: Vehicle", 50.0)],
    )


def test_find_time_traces_ignores_non_trace_json(tmp_path: Path) -> None:
    (tmp_path / "compile_commands.json").write_text("[]", encoding="utf-8")
    trace = tmp_path / "main.json"
    trace.write_text(
        json.dumps({"traceEvents": [{"ph": "X", "name": "ExecuteCompiler", "dur": 1000}]}),
        encoding="utf-8",
    )

    traces = find_time_traces(tmp_path)

    assert [item.path for item in traces] == [trace]


def test_build_report_includes_all_sections() -> None:
    summary = summarize_ninja_log(
        [
            BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 0, 120, 0, "a"),
            BuildEdge(".rcc/qmlcache/Foo_qml.cpp.o", 0, 180, 0, "b"),
        ],
        limit=5,
    )
    trace = TimeTrace(
        path=Path("build/main.json"), total_ms=250.0, top_events=[("Source: QtCore/QObject", 90.0)]
    )

    report = build_report(summary, [trace], limit=5)

    assert "Slowest Ninja Edges" in report
    assert "Generated Step Hotspots" in report
    assert "Most Rebuilt Outputs" in report
    assert "Slowest Time Traces" in report
    assert ".rcc/qmlcache/Foo_qml.cpp.o" in report
    assert "Source: QtCore/QObject" in report


def test_command_with_many_outputs_is_counted_once():
    edges = [
        BuildEdge("QGroundControl_autogen/timestamp", 0, 1000, 0, "a"),
        BuildEdge("/absolute/build/QGroundControl_autogen/timestamp", 0, 1000, 0, "a"),
        BuildEdge("QGroundControl_autogen/mocs_compilation.cpp", 0, 1000, 0, "a"),
        BuildEdge("Release/QGroundControl", 1000, 2000, 0, "b"),
    ]
    summary = summarize_ninja_log(edges, limit=15)
    assert len(summary.edges) == 2
    assert "Link and Archive Hotspots" in build_report(summary, [], limit=15)


def test_snapshot_does_not_recount_previous_build_or_noop(tmp_path):
    log = tmp_path / ".ninja_log"
    original = "# ninja log v5\n0\t100\t1\told.o\ta\n"
    log.write_text(original)
    previous = parse_ninja_log(log)
    log.write_text(original + "0\t250\t2\tRelease/app\tb\n")
    output = tmp_path / "profile"
    write_snapshot(tmp_path, output, previous=previous, wall_seconds=0.3)
    report = json.loads((output / "report.json").read_text())
    assert report["edge_count"] == 1
    assert report["link_edges"][0]["duration_ms"] == 250
    assert report["wall_seconds"] == 0.3
    assert (output / "ninja.log").read_text() == log.read_text()
    write_snapshot(tmp_path, output, previous=parse_ninja_log(log))
    assert json.loads((output / "report.json").read_text())["edge_count"] == 0


def test_snapshot_handles_ninja_log_compaction(tmp_path):
    log = tmp_path / ".ninja_log"
    log.write_text("0\t100\t1\ta.o\ta\n0\t200\t2\ta.o\tb\n")
    previous = parse_ninja_log(log)
    log.write_text("0\t200\t2\ta.o\tb\n0\t300\t3\tnew.o\tc\n")
    write_snapshot(tmp_path, tmp_path / "profile", previous=previous)
    report = json.loads((tmp_path / "profile/report.json").read_text())
    assert report["edge_count"] == 1
    assert report["slowest_edges"][0]["output"] == "new.o"


def test_cli_reports_json_and_skips_snapshot_without_ninja_log(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    (tmp_path / ".ninja_log").write_text("# ninja log v5\n0\t40\t0\ta.o\th\n", encoding="utf-8")
    assert build_profile_cli.main(["-B", str(tmp_path), "--json"]) == 0
    assert json.loads(capsys.readouterr().out)["edge_count"] == 1

    empty = tmp_path / "empty"
    empty.mkdir()
    assert build_profile_cli.main(["-B", str(empty), "--output-dir", str(tmp_path / "out")]) == 0
    assert "No Ninja log" in capsys.readouterr().out
    assert not (tmp_path / "out").exists()


def test_cli_snapshot_honors_ninja_log_and_limit(tmp_path: Path) -> None:
    log = tmp_path / "logs" / "custom.ninja_log"
    log.parent.mkdir()
    log.write_text("# ninja log v5\n0\t300\t0\ta.o\ta\n0\t200\t0\tb.o\tb\n0\t100\t0\tc.o\tc\n")
    output = tmp_path / "out"
    args = ["-B", str(tmp_path / "build"), "--ninja-log", str(log), "--limit", "1"]

    assert build_profile_cli.main([*args, "--output-dir", str(output)]) == 0

    report = json.loads((output / "report.json").read_text(encoding="utf-8"))
    assert [edge["output"] for edge in report["slowest_edges"]] == ["a.o"]
    assert (output / "ninja.log").read_text() == log.read_text()


def test_cli_json_out_writes_both_reports_from_one_trace_scan(
    tmp_path: Path, capsys: pytest.CaptureFixture[str], monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / ".ninja_log").write_text("# ninja log v5\n0\t40\t0\ta.o\th\n", encoding="utf-8")
    scans: list[Path] = []
    monkeypatch.setattr(
        build_profile_cli, "find_time_traces", lambda root: scans.append(root) or []
    )
    json_out = tmp_path / "report.json"

    assert build_profile_cli.main(["-B", str(tmp_path), "--json-out", str(json_out)]) == 0

    assert capsys.readouterr().out.startswith("# Build Profile Report")
    assert json.loads(json_out.read_text(encoding="utf-8"))["edge_count"] == 1
    assert scans == [tmp_path.resolve()]


def test_cli_rejects_json_options_with_snapshot(tmp_path: Path) -> None:
    for option in (["--json"], ["--json-out", str(tmp_path / "r.json")]):
        with pytest.raises(SystemExit):
            build_profile_cli.parse_args(["--output-dir", str(tmp_path), *option])


def test_cli_step_summary_collapses_the_report_and_extra_reports(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
    gh_summary: Path,
) -> None:
    (tmp_path / ".ninja_log").write_text("# ninja log v5\n0\t40\t0\ta.o\th\n", encoding="utf-8")
    (tmp_path / "CMakeCache.txt").write_text(
        "CMAKE_BUILD_TYPE:STRING=Release\nCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++\n",
        encoding="utf-8",
    )
    trace_report = tmp_path / "trace.txt"
    trace_report.write_text("**** Expensive headers:\n```\nQObject\n", encoding="utf-8")
    monkeypatch.setenv("GITHUB_SHA", "0123abc")
    compilers: list[str] = []
    monkeypatch.setattr(
        build_profile_cli,
        "_compiler_version",
        lambda compiler: compilers.append(compiler) or "Ubuntu clang version 14.0.0",
    )

    argv = ["-B", str(tmp_path), "--step-summary", "--extra-report", f"Headers={trace_report}"]
    assert build_profile_cli.main(argv) == 0

    report = capsys.readouterr().out
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Build profile\n\n| Item | Value |")
    assert "| Build type | Release |" in summary
    assert "| Commit | `0123abc` |" in summary
    assert "| Compiler | `Ubuntu clang version 14.0.0` |" in summary
    assert compilers == ["/usr/bin/clang++"]
    assert (
        "<details><summary>Ninja and per-TU hotspots (tools/build_profile.py)</summary>\n\n```\n"
        + report.rstrip()
        + "\n```\n\n</details>"
    ) in summary
    # The fence outgrows the report's own backtick runs.
    assert (
        "<details><summary>Headers</summary>\n\n````\n**** Expensive headers:\n```\nQObject\n````"
    ) in summary


def test_cli_extra_report_title_ends_at_the_first_equals_sign(tmp_path: Path) -> None:
    args = build_profile_cli.parse_args(
        ["--step-summary", "--extra-report", f"Headers (ClangBuildAnalyzer)={tmp_path}/a=b.txt"]
    )
    assert args.extra_reports == [("Headers (ClangBuildAnalyzer)", tmp_path / "a=b.txt")]


@pytest.mark.parametrize(
    "argv",
    [
        ["--extra-report", "Title=file.txt"],
        ["--step-summary", "--extra-report", "file.txt"],
        ["--step-summary", "--extra-report", "=file.txt"],
        ["--step-summary", "--output-dir", "out"],
    ],
)
def test_cli_rejects_invalid_summary_options(argv: list[str]) -> None:
    with pytest.raises(SystemExit):
        build_profile_cli.parse_args(argv)


def test_install_clang_build_analyzer_downloads_the_pinned_binary(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    pin = load_build_config()["tools"]["clang_build_analyzer"]
    downloads: list[tuple[str, Path, str | None]] = []

    def fake_download(url: str, dest: Path, *, expected_sha256: str | None = None) -> None:
        downloads.append((url, dest, expected_sha256))
        dest.write_bytes(b"\x7fELF")

    monkeypatch.setattr(build_profile_cli, "download_file", fake_download)
    monkeypatch.setattr(build_profile_cli, "is_linux", lambda: True)
    monkeypatch.setattr(build_profile_cli, "host_arch", lambda: "x86_64")
    binary = tmp_path / "bin" / "ClangBuildAnalyzer"

    assert build_profile_cli.main(["--install-clang-build-analyzer", str(binary)]) == 0

    assert downloads == [
        (
            f"https://github.com/aras-p/ClangBuildAnalyzer/releases/download/v{pin['version']}/"
            "ClangBuildAnalyzer-linux",
            binary,
            pin["sha256"]["linux-x86_64"],
        )
    ]
    assert os.access(binary, os.X_OK)


def test_install_clang_build_analyzer_needs_linux_x86_64(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(build_profile_cli, "is_linux", lambda: True)
    monkeypatch.setattr(build_profile_cli, "host_arch", lambda: "aarch64")
    with pytest.raises(SystemExit):
        build_profile_cli.parse_args(["--install-clang-build-analyzer", str(tmp_path / "cba")])


def test_build_profile_workflow_summarizes_through_the_cli() -> None:
    steps = get_steps(load_workflow("build-profile.yml"), "profile")
    runs = "\n".join(step.get("run", "") for step in steps)
    assert "GITHUB_STEP_SUMMARY" not in runs
    assert "curl " not in runs
    assert "/tmp/" not in runs
    install = get_step(steps, "Install ClangBuildAnalyzer")["run"]
    assert "--install-clang-build-analyzer" in install
    profile = get_step(steps, "Run tools/build_profile.py (ninja + per-TU)")["run"]
    assert "--step-summary" in profile
    assert (
        '--extra-report "Headers, templates, and functions (ClangBuildAnalyzer)='
        'qgc-trace-report.txt"' in profile
    )
    names = [step.get("name") for step in steps]
    assert names.index("Analyze ClangBuildAnalyzer capture") < names.index(
        "Run tools/build_profile.py (ninja + per-TU)"
    )
