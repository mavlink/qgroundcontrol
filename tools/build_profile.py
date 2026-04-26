#!/usr/bin/env python3
"""Summarize Ninja and Clang time-trace build hotspots."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class BuildEdge:
    """One completed Ninja build edge from .ninja_log."""

    output: str
    start_ms: int
    end_ms: int
    mtime: int
    command_hash: str

    @property
    def duration_ms(self) -> int:
        return max(0, self.end_ms - self.start_ms)


@dataclass(frozen=True)
class RebuiltOutput:
    """Output rebuilt multiple times in the parsed Ninja log."""

    output: str
    count: int
    total_ms: int
    max_ms: int


@dataclass(frozen=True)
class NinjaSummary:
    """Aggregated Ninja log data."""

    edges: list[BuildEdge]
    slowest_edges: list[BuildEdge]
    generated_edges: list[BuildEdge]
    rebuilt_outputs: list[RebuiltOutput]


@dataclass(frozen=True)
class TimeTrace:
    """Summary of one Clang -ftime-trace JSON file."""

    path: Path
    total_ms: float
    top_events: list[tuple[str, float]]


def parse_ninja_log(path: Path) -> list[BuildEdge]:
    """Parse Ninja's tab-separated .ninja_log file."""
    edges: list[BuildEdge] = []
    if not path.exists():
        raise FileNotFoundError(f"Ninja log not found: {path}")

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue

        parts = line.split("\t")
        if len(parts) < 5:
            raise ValueError(f"Malformed Ninja log line {line_number}: expected at least 5 tab-separated fields")

        start, end, mtime, output, command_hash = parts[:5]
        try:
            edges.append(
                BuildEdge(
                    output=output,
                    start_ms=int(start),
                    end_ms=int(end),
                    mtime=int(mtime),
                    command_hash=command_hash,
                )
            )
        except ValueError as exc:
            raise ValueError(f"Malformed Ninja log line {line_number}: invalid numeric field") from exc

    return edges


def classify_output(output: str) -> str:
    """Classify a build output into a coarse build-hotspot category."""
    normalized = output.replace("\\", "/")
    lower = normalized.lower()
    name = Path(normalized).name

    if "qmlcache" in lower:
        return "qmlcache"
    if "qmltyperegistrations" in lower:
        return "qmltyperegistration"
    if "_autogen/" in lower or "mocs_compilation" in lower or name.startswith("moc_"):
        return "autogen/moc"
    if "/.qt/rcc/" in lower or "/.rcc/" in lower or name.startswith("qrc_"):
        return "rcc"
    if name.endswith((".a", ".lib", ".so", ".dylib", ".dll", ".exe")) or "/release/" in lower or "/debug/" in lower:
        return "link/archive"
    if name.endswith((".o", ".obj")):
        return "compile"
    return "other"


def summarize_ninja_log(edges: list[BuildEdge], *, limit: int) -> NinjaSummary:
    """Build a sorted summary from Ninja log edges."""
    slowest = sorted(edges, key=lambda edge: edge.duration_ms, reverse=True)[:limit]
    generated = [
        edge
        for edge in sorted(edges, key=lambda edge: edge.duration_ms, reverse=True)
        if classify_output(edge.output) in {"autogen/moc", "qmlcache", "qmltyperegistration", "rcc"}
    ][:limit]

    by_output: dict[str, list[BuildEdge]] = {}
    for edge in edges:
        by_output.setdefault(edge.output, []).append(edge)

    rebuilt = [
        RebuiltOutput(
            output=output,
            count=len(output_edges),
            total_ms=sum(edge.duration_ms for edge in output_edges),
            max_ms=max(edge.duration_ms for edge in output_edges),
        )
        for output, output_edges in by_output.items()
        if len(output_edges) > 1
    ]
    rebuilt.sort(key=lambda item: (item.count, item.total_ms), reverse=True)

    return NinjaSummary(
        edges=edges,
        slowest_edges=slowest,
        generated_edges=generated,
        rebuilt_outputs=rebuilt[:limit],
    )


def parse_time_trace(path: Path, *, event_limit: int = 8) -> TimeTrace:
    """Parse one Clang -ftime-trace JSON file."""
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"Not a Clang time-trace file: {path}")
    events = data.get("traceEvents")
    if not isinstance(events, list):
        raise ValueError(f"Not a Clang time-trace file: {path}")

    complete_events = [
        event
        for event in events
        if event.get("ph") == "X" and isinstance(event.get("dur"), int | float)
    ]
    if not complete_events:
        raise ValueError(f"No complete time-trace events found: {path}")

    total_us = max(
        (
            event["dur"]
            for event in complete_events
            if event.get("name") in {"ExecuteCompiler", "Total ExecuteCompiler"}
        ),
        default=max(event["dur"] for event in complete_events),
    )

    detail_events: list[tuple[str, float]] = []
    for event in complete_events:
        name = str(event.get("name", "unknown"))
        if name in {"ExecuteCompiler", "Total ExecuteCompiler"} or name.startswith("Total "):
            continue
        args = event.get("args") if isinstance(event.get("args"), dict) else {}
        detail = args.get("detail") if isinstance(args, dict) else None
        label = f"{name}: {detail}" if detail else name
        detail_events.append((label, event["dur"] / 1000.0))

    detail_events.sort(key=lambda item: item[1], reverse=True)
    return TimeTrace(path=path, total_ms=total_us / 1000.0, top_events=detail_events[:event_limit])


def find_time_traces(root: Path) -> list[TimeTrace]:
    """Find and parse Clang time-trace JSON files below *root*."""
    traces: list[TimeTrace] = []
    if not root.exists():
        return traces

    for path in root.rglob("*.json"):
        try:
            traces.append(parse_time_trace(path))
        except (OSError, ValueError, json.JSONDecodeError):
            continue

    traces.sort(key=lambda trace: trace.total_ms, reverse=True)
    return traces


def format_ms(duration_ms: int | float) -> str:
    """Format milliseconds as seconds with compact precision."""
    return f"{duration_ms / 1000.0:.2f}s"


def build_report(summary: NinjaSummary, traces: list[TimeTrace], *, limit: int) -> str:
    """Build a Markdown-style text report."""
    lines: list[str] = [
        "# Build Profile Report",
        "",
        f"Parsed Ninja edges: {len(summary.edges)}",
        "",
    ]

    lines.extend(_edge_section("Slowest Ninja Edges", summary.slowest_edges))
    lines.extend(_edge_section("Generated Step Hotspots", summary.generated_edges))

    lines.extend(["## Most Rebuilt Outputs"])
    if summary.rebuilt_outputs:
        for item in summary.rebuilt_outputs[:limit]:
            lines.append(
                f"- {item.count}x, total {format_ms(item.total_ms)}, max {format_ms(item.max_ms)}: `{item.output}`"
            )
    else:
        lines.append("- No repeated outputs in the parsed Ninja log.")
    lines.append("")

    lines.extend(["## Slowest Time Traces"])
    if traces:
        for trace in traces[:limit]:
            lines.append(f"- {format_ms(trace.total_ms)}: `{trace.path}`")
            for label, duration_ms in trace.top_events[:3]:
                lines.append(f"  - {format_ms(duration_ms)}: {label}")
    else:
        lines.append("- No Clang time-trace JSON files found.")
    lines.append("")

    return "\n".join(lines)


def _edge_section(title: str, edges: list[BuildEdge]) -> list[str]:
    lines = [f"## {title}"]
    if not edges:
        lines.append("- No matching Ninja edges.")
    else:
        for edge in edges:
            category = classify_output(edge.output)
            lines.append(f"- {format_ms(edge.duration_ms)} [{category}]: `{edge.output}`")
    lines.append("")
    return lines


def build_json(summary: NinjaSummary, traces: list[TimeTrace], *, limit: int) -> dict[str, Any]:
    """Build a machine-readable report payload."""
    return {
        "edge_count": len(summary.edges),
        "slowest_edges": [_edge_to_json(edge) for edge in summary.slowest_edges[:limit]],
        "generated_edges": [_edge_to_json(edge) for edge in summary.generated_edges[:limit]],
        "rebuilt_outputs": [
            {
                "output": item.output,
                "count": item.count,
                "total_ms": item.total_ms,
                "max_ms": item.max_ms,
            }
            for item in summary.rebuilt_outputs[:limit]
        ],
        "time_traces": [
            {
                "path": str(trace.path),
                "total_ms": trace.total_ms,
                "top_events": [{"label": label, "duration_ms": duration_ms} for label, duration_ms in trace.top_events],
            }
            for trace in traces[:limit]
        ],
    }


def _edge_to_json(edge: BuildEdge) -> dict[str, Any]:
    return {
        "output": edge.output,
        "duration_ms": edge.duration_ms,
        "category": classify_output(edge.output),
        "command_hash": edge.command_hash,
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Report Ninja and Clang time-trace build hotspots.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  %(prog)s
  %(prog)s --build-dir build-debug --limit 20
  %(prog)s --trace-dir build --json

Enable Clang traces with CMake option QGC_TIME_TRACE=ON, then rebuild.
""",
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        default=Path("build"),
        type=Path,
        help="Build directory containing .ninja_log (default: build)",
    )
    parser.add_argument(
        "--ninja-log",
        type=Path,
        help="Explicit .ninja_log path (default: <build-dir>/.ninja_log)",
    )
    parser.add_argument(
        "--trace-dir",
        type=Path,
        help="Directory to scan for Clang -ftime-trace JSON files (default: build dir)",
    )
    parser.add_argument("--limit", type=int, default=15, help="Rows to show per section (default: 15)")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    build_dir = args.build_dir.resolve()
    ninja_log = (args.ninja_log or build_dir / ".ninja_log").resolve()
    trace_dir = (args.trace_dir or build_dir).resolve()
    limit = max(1, args.limit)

    edges = parse_ninja_log(ninja_log)
    summary = summarize_ninja_log(edges, limit=limit)
    traces = find_time_traces(trace_dir)

    if args.json:
        print(json.dumps(build_json(summary, traces, limit=limit), indent=2))
    else:
        print(build_report(summary, traces, limit=limit))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
