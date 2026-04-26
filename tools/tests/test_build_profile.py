#!/usr/bin/env python3
"""Tests for tools/build_profile.py."""

from __future__ import annotations

import json
from pathlib import Path

from build_profile import (
    BuildEdge,
    TimeTrace,
    build_report,
    classify_output,
    find_time_traces,
    parse_ninja_log,
    parse_time_trace,
    summarize_ninja_log,
)


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
        BuildEdge(output="CMakeFiles/app.dir/src/main.cc.o", start_ms=10, end_ms=60, mtime=0, command_hash="abc"),
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


def test_parse_time_trace_reads_compile_duration_and_expensive_events(tmp_path: Path) -> None:
    trace = tmp_path / "main.json"
    trace.write_text(
        json.dumps(
            {
                "traceEvents": [
                    {"ph": "X", "name": "ExecuteCompiler", "dur": 250000, "args": {"detail": "main.cc"}},
                    {"ph": "X", "name": "Source", "dur": 90000, "args": {"detail": "QtCore/QObject"}},
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
    trace = TimeTrace(path=Path("build/main.json"), total_ms=250.0, top_events=[("Source: QtCore/QObject", 90.0)])

    report = build_report(summary, [trace], limit=5)

    assert "Slowest Ninja Edges" in report
    assert "Generated Step Hotspots" in report
    assert "Most Rebuilt Outputs" in report
    assert "Slowest Time Traces" in report
    assert ".rcc/qmlcache/Foo_qml.cpp.o" in report
    assert "Source: QtCore/QObject" in report
