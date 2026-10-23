"""Ninja log and Clang time-trace build-hotspot summaries."""

from __future__ import annotations

import json
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from common.format import format_count, format_duration
from common.io import read_json, write_json
from common.markdown import code_span, md_table


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
            raise ValueError(
                f"Malformed Ninja log line {line_number}: expected at least 5 tab-separated fields"
            )

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
            raise ValueError(
                f"Malformed Ninja log line {line_number}: invalid numeric field"
            ) from exc

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
    if name.endswith((".o", ".obj")):
        return "compile"
    if (
        name.endswith((".a", ".lib", ".so", ".dylib", ".dll", ".exe"))
        or lower.startswith(("release/", "debug/", "relwithdebinfo/", "minsizerel/"))
        or "/release/" in lower
        or "/debug/" in lower
    ):
        return "link/archive"
    return "other"


def summarize_ninja_log(edges: list[BuildEdge], *, limit: int) -> NinjaSummary:
    """Build a sorted summary from Ninja log edges."""
    # Ninja logs every output of a command, including absolute/relative aliases.
    # Count each execution once; otherwise large autogen rules dominate totals.
    commands: dict[tuple[int, int, str], BuildEdge] = {}
    for edge in edges:
        identity = (edge.start_ms, edge.end_ms, edge.command_hash)
        previous = commands.get(identity)
        if previous is None or len(edge.output) < len(previous.output):
            commands[identity] = edge
    edges = list(commands.values())
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
    data = read_json(path)
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
    links = sorted(
        (edge for edge in summary.edges if classify_output(edge.output) == "link/archive"),
        key=lambda edge: edge.duration_ms,
        reverse=True,
    )
    lines.extend(_edge_section("Link and Archive Hotspots", links[:limit]))
    lines.extend(
        [
            "## Task Time by Category",
            "",
            "Task durations overlap; these totals are not wall time.",
            "",
        ]
    )
    for category, values in category_totals(summary.edges).items():
        lines.append(
            f"- {category}: {values['count']} commands, {format_ms(values['duration_ms'])}"
        )
    lines.append("")

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
        "categories": category_totals(summary.edges),
        "link_edges": [
            _edge_to_json(edge)
            for edge in sorted(summary.edges, key=lambda edge: edge.duration_ms, reverse=True)
            if classify_output(edge.output) == "link/archive"
        ][:limit],
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
                "top_events": [
                    {"label": label, "duration_ms": duration_ms}
                    for label, duration_ms in trace.top_events
                ],
            }
            for trace in traces[:limit]
        ],
    }


def _edge_to_json(edge: BuildEdge) -> dict[str, Any]:
    return {
        "output": edge.output,
        "duration_ms": edge.duration_ms,
        "start_ms": edge.start_ms,
        "end_ms": edge.end_ms,
        "category": classify_output(edge.output),
        "command_hash": edge.command_hash,
    }


def category_totals(edges: list[BuildEdge]) -> dict[str, dict[str, int]]:
    totals: dict[str, dict[str, int]] = {}
    for edge in edges:
        entry = totals.setdefault(classify_output(edge.output), {"count": 0, "duration_ms": 0})
        entry["count"] += 1
        entry["duration_ms"] += edge.duration_ms
    return totals


def write_snapshot(
    build_dir: Path,
    output_dir: Path,
    *,
    previous: list[BuildEdge] | None = None,
    wall_seconds: float | None = None,
    ninja_log: Path | None = None,
    limit: int = 15,
) -> str:
    """Save cheap Ninja timings and raw evidence without scanning the SDK for traces."""
    log = ninja_log or build_dir / ".ninja_log"
    edges = parse_ninja_log(log)
    if previous is not None:
        if edges[: len(previous)] == previous:
            edges = edges[len(previous) :]
        else:
            # Ninja may compact its accumulated log before starting a build.
            old = set(previous)
            edges = [edge for edge in edges if edge not in old]
    summary = summarize_ninja_log(edges, limit=limit)
    report = build_report(summary, [], limit=limit)
    payload = build_json(summary, [], limit=limit)
    payload["wall_seconds"] = wall_seconds
    output_dir.mkdir(parents=True, exist_ok=True)
    # A non-hidden name is uploaded by actions/upload-artifact without opting
    # into unrelated hidden build files.
    shutil.copyfile(log, output_dir / "ninja.log")
    (output_dir / "report.md").write_text(report, encoding="utf-8")
    write_json(output_dir / "report.json", payload)
    return report


def snapshot_summary(
    profile_dir: Path, *, limit: int = 10
) -> tuple[list[tuple[str, object]], list[tuple[str, str]]]:
    """Return job-summary facts and collapsed tables for a write_snapshot() directory."""
    try:
        report = read_json(profile_dir / "report.json")
    except (OSError, ValueError):
        return [], []
    if not isinstance(report, dict):
        return [], []
    edges = report.get("slowest_edges") or []
    slowest = md_table(
        ["Output", "Kind", "Time"],
        [
            [
                code_span(str(edge.get("output", ""))[-100:]),
                str(edge.get("category", "")),
                format_duration(int(edge.get("duration_ms", 0)) / 1000),
            ]
            for edge in edges[:limit]
        ],
        align=["left", "left", "right"],
        markdown=True,
    )
    categories = sorted(
        (report.get("categories") or {}).items(),
        key=lambda item: item[1].get("duration_ms", 0),
        reverse=True,
    )
    by_kind = md_table(
        ["Kind", "Commands", "Time (summed; commands overlap)"],
        [
            [
                kind,
                format_count(int(values.get("count", 0))),
                format_duration(int(values.get("duration_ms", 0)) / 1000),
            ]
            for kind, values in categories
        ],
        align=["left", "right", "right"],
    )
    facts: list[tuple[str, object]] = [
        ("Build steps run", format_count(int(report.get("edge_count", 0))))
    ]
    return facts, [
        (f"Slowest build steps (top {limit})", slowest if edges else ""),
        ("Build time by kind", by_kind if categories else ""),
    ]
