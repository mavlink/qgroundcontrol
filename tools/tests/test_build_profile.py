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
    write_snapshot,
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
