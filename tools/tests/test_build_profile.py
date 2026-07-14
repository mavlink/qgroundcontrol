#!/usr/bin/env python3
"""Build-profile parsing and report contracts."""

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


def test_ninja_log_parsing_classification_and_summary(tmp_path: Path) -> None:
    log = tmp_path / ".ninja_log"
    log.write_text(
        "# ninja log v5\n"
        "10\t60\t0\tCMakeFiles/app.dir/src/main.cc.o\tabc\n"
        "80\t130\t0\tqml/Foo.qmlc\tdef\n"
    )
    parsed = parse_ninja_log(log)
    assert parsed[0] == BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 10, 60, 0, "abc")
    assert parsed[0].duration_ms == 50

    classifications = {
        ".rcc/qmlcache/Foo_qml.cpp.o": "qmlcache",
        "src/Foo_autogen/mocs_compilation.cpp.o": "autogen/moc",
        "src/.qt/rcc/qrc_resources.cpp.o": "rcc",
        "src/qgroundcontrol_qmltyperegistrations.cpp.o": "qmltyperegistration",
        "Release/lib/libQGC.a": "link/archive",
        "CMakeFiles/app.dir/src/main.cc.o": "compile",
    }
    for output, expected in classifications.items():
        assert classify_output(output) == expected

    summary = summarize_ninja_log(
        [
            BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 0, 120, 0, "a"),
            BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 200, 330, 0, "b"),
            BuildEdge("src/Foo_autogen/mocs_compilation.cpp.o", 0, 90, 0, "c"),
            BuildEdge(".rcc/qmlcache/Foo_qml.cpp.o", 0, 180, 0, "d"),
            BuildEdge("Release/lib/libQGC.a", 0, 220, 0, "e"),
        ],
        limit=2,
    )
    assert [edge.output for edge in summary.slowest_edges] == [
        "Release/lib/libQGC.a",
        ".rcc/qmlcache/Foo_qml.cpp.o",
    ]
    assert summary.rebuilt_outputs[0].count == 2
    assert [edge.output for edge in summary.generated_edges] == [
        ".rcc/qmlcache/Foo_qml.cpp.o",
        "src/Foo_autogen/mocs_compilation.cpp.o",
    ]


def test_time_trace_parsing_and_discovery_ignore_unrelated_json(tmp_path: Path) -> None:
    (tmp_path / "compile_commands.json").write_text("[]")
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
        )
    )
    expected = TimeTrace(
        path=trace,
        total_ms=250.0,
        top_events=[("Source: QtCore/QObject", 90.0), ("ParseClass: Vehicle", 50.0)],
    )
    assert parse_time_trace(trace) == expected
    assert find_time_traces(tmp_path) == [expected]


def test_report_includes_each_profile_section() -> None:
    summary = summarize_ninja_log(
        [
            BuildEdge("CMakeFiles/app.dir/src/main.cc.o", 0, 120, 0, "a"),
            BuildEdge(".rcc/qmlcache/Foo_qml.cpp.o", 0, 180, 0, "b"),
        ],
        limit=5,
    )
    trace = TimeTrace(Path("build/main.json"), 250.0, [("Source: QtCore/QObject", 90.0)])
    report = build_report(summary, [trace], limit=5)
    for text in (
        "Slowest Ninja Edges",
        "Generated Step Hotspots",
        "Most Rebuilt Outputs",
        "Slowest Time Traces",
        ".rcc/qmlcache/Foo_qml.cpp.o",
        "Source: QtCore/QObject",
    ):
        assert text in report
