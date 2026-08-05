#!/usr/bin/env python3
"""Parse GStreamer `latency` tracer output and emit per-element latency stats.

Usage:
    GST_TRACERS="latency(flags=pipeline+element+reported)" \\
    GST_DEBUG="GST_TRACER:7" \\
    GST_DEBUG_FILE=/tmp/gst.log \\
    QGroundControl

    python3 tools/debuggers/gst_latency_parser.py /tmp/gst.log
    python3 tools/debuggers/gst_latency_parser.py --threshold-ms 100 /tmp/gst.log
    python3 tools/debuggers/gst_latency_parser.py --json /tmp/gst.log | jq

Exit code 1 if any element's p95 exceeds --threshold-ms (CI gate).
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from collections import defaultdict
from pathlib import Path

# Tracer line shape (gst_tracer_record_log):
#   <ts> <pid> <thread> TRACE GST_TRACER :0:: <recordname>, key=(type)value, key=(type)value, ...;
# Records emitted by the latency tracer:
#   pipeline-latency: source-to-sink wall-clock latency
#   element-latency:  per-element processing time
#   element-reported-latency: latency claimed by the element's LATENCY query
_RECORD_RE = re.compile(r"TRACE\s+GST_TRACER\s+:\d+::\s+(?P<rec>[\w-]+),\s*(?P<rest>.*?);?\s*$")
_FIELD_RE = re.compile(r"(?P<k>[\w-]+)=\((?P<t>[\w]+)\)(?P<v>[^,;]+)(?:,|$)")


def _parse_fields(rest: str) -> dict[str, str]:
    out: dict[str, str] = {}
    # _FIELD_RE leaves the final field unterminated; pad with a comma so the regex matches uniformly.
    for m in _FIELD_RE.finditer(rest + ","):
        v = m.group("v").strip()
        if v.startswith('"') and v.endswith('"'):
            v = v[1:-1]
        out[m.group("k")] = v
    return out


def parse(path: Path) -> dict[str, list[int]]:
    """Return {bucket_label: [ns, ns, ...]} for each tracer record kind we care about."""
    buckets: dict[str, list[int]] = defaultdict(list)
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = _RECORD_RE.search(line)
            if not m:
                continue
            rec = m.group("rec")
            fields = _parse_fields(m.group("rest"))
            time_ns_str = fields.get("time")
            if time_ns_str is None:
                continue
            try:
                time_ns = int(time_ns_str)
            except ValueError:
                continue
            if rec == "pipeline-latency":
                buckets[f"pipeline:{fields.get('element-id', '?')}"].append(time_ns)
            elif rec == "element-latency":
                src = fields.get("src-element") or fields.get("element") or "?"
                sink = fields.get("sink-element") or "?"
                buckets[f"element:{src}->{sink}"].append(time_ns)
            elif rec == "element-reported-latency":
                el = fields.get("element") or "?"
                buckets[f"reported:{el}"].append(time_ns)
    return buckets


def _percentile(sorted_vals: list[int], pct: float) -> int:
    if not sorted_vals:
        return 0
    # Nearest-rank, no interpolation — keeps the integer ns and avoids picking a value that didn't occur.
    k = max(0, min(len(sorted_vals) - 1, math.ceil(pct / 100.0 * len(sorted_vals)) - 1))
    return sorted_vals[k]


def summarize(buckets: dict[str, list[int]]) -> list[dict]:
    rows: list[dict] = []
    for label, vals in sorted(buckets.items()):
        s = sorted(vals)
        rows.append({
            "label": label,
            "count": len(s),
            "p50_ms": _percentile(s, 50) / 1e6,
            "p95_ms": _percentile(s, 95) / 1e6,
            "p99_ms": _percentile(s, 99) / 1e6,
            "max_ms": s[-1] / 1e6 if s else 0,
        })
    return rows


def _format_table(rows: list[dict]) -> str:
    if not rows:
        return "(no latency records found — was GST_TRACERS=latency set with GST_DEBUG=GST_TRACER:7?)"
    widths = {"label": max(48, max(len(r["label"]) for r in rows))}
    header = f"{'bucket':<{widths['label']}}  {'count':>7}  {'p50ms':>8}  {'p95ms':>8}  {'p99ms':>8}  {'maxms':>8}"
    sep = "-" * len(header)
    lines = [header, sep]
    for r in rows:
        lines.append(
            f"{r['label']:<{widths['label']}}  {r['count']:>7d}  "
            f"{r['p50_ms']:>8.2f}  {r['p95_ms']:>8.2f}  {r['p99_ms']:>8.2f}  {r['max_ms']:>8.2f}"
        )
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("log", type=Path, help="GStreamer debug log file (GST_DEBUG_FILE output)")
    p.add_argument("--threshold-ms", type=float, default=None,
                   help="Fail (exit 1) if any bucket's p95 exceeds this. CI gate.")
    p.add_argument("--json", action="store_true", help="Emit JSON instead of a table")
    p.add_argument("--filter", default=None, help="Only show buckets containing this substring")
    args = p.parse_args(argv)

    if not args.log.exists():
        print(f"error: {args.log} not found", file=sys.stderr)
        return 2

    buckets = parse(args.log)
    if args.filter:
        buckets = {k: v for k, v in buckets.items() if args.filter in k}
    rows = summarize(buckets)

    if args.json:
        json.dump(rows, sys.stdout, indent=2)
        sys.stdout.write("\n")
    else:
        print(_format_table(rows))

    if args.threshold_ms is not None:
        bad = [r for r in rows if r["p95_ms"] > args.threshold_ms]
        if bad:
            print(f"\nFAIL: {len(bad)} bucket(s) exceeded p95 threshold {args.threshold_ms:.1f} ms:",
                  file=sys.stderr)
            for r in bad:
                print(f"  {r['label']}: p95={r['p95_ms']:.2f} ms", file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
