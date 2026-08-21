#!/usr/bin/env python3
"""Sample Android app memory usage over adb for soak testing.

Capture: start the script, then launch the app on the device and run the test
sequence. The script waits for the process, samples `dumpsys meminfo` at a
fixed interval, and writes a CSV. Press Enter to drop a phase marker (type a
label first for a named marker, e.g. "start idle"). Ctrl+C stops the capture.
Markers require a POSIX terminal (no-op on Windows); capture itself works
everywhere adb does. Originally written for the GeoMap go/no-go (issue #14880).

    python3 tools/android_mem_capture.py --label geomap --interval 10

Compare two runs (e.g. QtLocation vs GeoMap):

    python3 tools/android_mem_capture.py --compare mem-qtlocation-*.csv mem-geomap-*.csv
"""

from __future__ import annotations

import argparse
import csv
import math
import re
import select
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

DEFAULT_PACKAGE = "org.mavlink.qgroundcontrol"

CSV_FIELDS = [
    "time_s",
    "wallclock",
    "pid",
    "total_pss_kb",
    "java_heap_kb",
    "native_heap_kb",
    "graphics_kb",
    "egl_mtrack_kb",
    "gl_mtrack_kb",
    "mem_available_kb",
    "event",
]

# Columns worth trending in the compare summary
TREND_FIELDS = ["total_pss_kb", "graphics_kb", "native_heap_kb", "mem_available_kb"]


def _run_adb(serial: str | None, *args: str) -> subprocess.CompletedProcess[str]:
    cmd = ["adb"]
    if serial:
        cmd += ["-s", serial]
    cmd += list(args)
    try:
        return subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired as exc:
        # Surface stalls as RuntimeError so callers log a sample-failed row instead of crashing
        raise RuntimeError(f"adb {' '.join(args)} timed out after 30s") from exc


def _adb(serial: str | None, *args: str) -> str:
    result = _run_adb(serial, *args)
    if result.returncode != 0:
        raise RuntimeError(f"adb {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def _pidof(serial: str | None, package: str) -> int | None:
    """Return the pid, or None when the process is simply not running.

    adb transport failures raise RuntimeError so they are never mistaken for an app exit.
    """
    result = _run_adb(serial, "shell", "pidof", package)
    out = result.stdout.strip()
    if out:
        return int(out.split()[0])
    # pidof exits non-zero with empty output on a clean no-match; stderr text means adb trouble
    if result.stderr.strip():
        raise RuntimeError(f"adb pidof failed: {result.stderr.strip()}")
    return None


_SUMMARY_RE = {
    "total_pss_kb": re.compile(r"^\s*TOTAL(?: PSS)?:\s*(\d+)", re.MULTILINE),
    "java_heap_kb": re.compile(r"^\s*Java Heap:\s*(\d+)", re.MULTILINE),
    "native_heap_kb": re.compile(r"^\s*Native Heap:\s*(\d+)", re.MULTILINE),
    "graphics_kb": re.compile(r"^\s*Graphics:\s*(\d+)", re.MULTILINE),
}
_TABLE_RE = {
    "egl_mtrack_kb": re.compile(r"^\s*EGL mtrack\s+(\d+)", re.MULTILINE),
    "gl_mtrack_kb": re.compile(r"^\s*GL mtrack\s+(\d+)", re.MULTILINE),
}


def _sample_meminfo(serial: str | None, package: str) -> dict[str, int]:
    """Parse app PSS breakdown (kB) from dumpsys meminfo."""
    out = _adb(serial, "shell", "dumpsys", "meminfo", package)
    sample: dict[str, int] = {}
    for key, rx in {**_SUMMARY_RE, **_TABLE_RE}.items():
        m = rx.search(out)
        if m:
            sample[key] = int(m.group(1))
    return sample


def _mem_available_kb(serial: str | None) -> int | None:
    out = _adb(serial, "shell", "cat", "/proc/meminfo")
    m = re.search(r"^MemAvailable:\s*(\d+)", out, re.MULTILINE)
    return int(m.group(1)) if m else None


def _parse_duration(text: str) -> float:
    m = re.fullmatch(r"(\d+(?:\.\d+)?)\s*([smh]?)", text.strip())
    if not m:
        raise argparse.ArgumentTypeError(f"invalid duration: {text!r} (use e.g. 600, 30m, 1h)")
    value = float(m.group(1))
    return value * {"": 1, "s": 1, "m": 60, "h": 3600}[m.group(2)]


def _read_marker() -> str | None:
    """Non-blocking read of a pending stdin line; Enter alone means 'mark'."""
    if not sys.stdin.isatty():
        return None
    try:
        ready, _, _ = select.select([sys.stdin], [], [], 0)
    except (OSError, ValueError):
        # select() on stdin is unsupported on Windows; markers degrade to no-op
        return None
    if not ready:
        return None
    line = sys.stdin.readline().strip()
    return line or "mark"


def capture(args: argparse.Namespace) -> int:
    package = args.package
    serial = args.serial
    try:
        _adb(serial, "get-state")
    except RuntimeError as exc:
        print(f"error: no adb device: {exc}", file=sys.stderr)
        return 1

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    path = Path(args.output_dir) / f"mem-{args.label}-{timestamp}.csv"
    path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Waiting for {package} to start on the device...")
    pid = None
    try:
        while pid is None:
            try:
                pid = _pidof(serial, package)
            except RuntimeError as exc:
                print(f"  adb error: {exc} — retrying")
            if pid is None:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopped.")
        return 1

    print(f"Capturing pid {pid} every {args.interval:g}s -> {path}")
    if sys.stdin.isatty():
        print("Press Enter to drop a marker (type a label first for a named one). Ctrl+C to stop.")

    start = time.monotonic()
    deadline = start + args.duration if args.duration else None
    next_sample = start
    pending_events: list[str] = []

    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_FIELDS)
        writer.writeheader()
        try:
            while deadline is None or time.monotonic() < deadline:
                # Poll stdin between samples so markers land near the moment they were typed
                while (remaining := next_sample - time.monotonic()) > 0:
                    if deadline is not None and time.monotonic() >= deadline:
                        break
                    marker = _read_marker()
                    if marker:
                        pending_events.append(marker)
                        print(f"  marker: {marker}")
                    time.sleep(min(0.2, remaining))
                if deadline is not None and time.monotonic() >= deadline:
                    break
                # Skip missed slots so a slow/stalled sample never causes a catch-up burst
                next_sample += args.interval
                while next_sample <= time.monotonic():
                    next_sample += args.interval

                adb_failed = False
                current_pid = None
                try:
                    current_pid = _pidof(serial, package)
                except RuntimeError as exc:
                    adb_failed = True
                    pending_events.append(f"adb-error: {exc}")
                    print(f"  adb error: {exc} — will retry next sample")
                if not adb_failed:
                    if current_pid is None:
                        pending_events.append("process-gone")
                        print("  process gone (LMK kill or exit) — waiting for restart")
                    elif current_pid != pid:
                        pending_events.append(f"restart pid {pid}->{current_pid}")
                        print(f"  process restarted: pid {pid} -> {current_pid}")
                        pid = current_pid

                mem_available = None
                try:
                    mem_available = _mem_available_kb(serial)
                except RuntimeError as exc:
                    pending_events.append(f"sample-failed: {exc}")

                row: dict[str, object] = {
                    "time_s": f"{time.monotonic() - start:.1f}",
                    "wallclock": datetime.now().strftime("%H:%M:%S"),
                    "pid": current_pid or "",
                    "mem_available_kb": mem_available or "",
                    "event": "; ".join(pending_events),
                }
                pending_events.clear()
                if current_pid is not None:
                    try:
                        row.update(_sample_meminfo(serial, package))
                    except RuntimeError as exc:
                        row["event"] = f"{row['event']}; sample-failed: {exc}".strip("; ")
                writer.writerow(row)
                f.flush()

                total = row.get("total_pss_kb", "?")
                gfx = row.get("graphics_kb", "?")
                native = row.get("native_heap_kb", "?")
                avail = row.get("mem_available_kb", "?")
                print(
                    f"  {row['time_s']:>7}s  total {total} kB  graphics {gfx} kB"
                    f"  native {native} kB  sys-avail {avail} kB"
                )
        except KeyboardInterrupt:
            print("\nStopped.")

    print(f"Capture written: {path}")
    return 0


def _load_run(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def _column(rows: list[dict[str, str]], field: str) -> list[tuple[float, float]]:
    points = []
    for row in rows:
        if row.get(field) and row.get("time_s"):
            points.append((float(row["time_s"]), float(row[field])))
    return points


def _slope_kb_per_min(points: list[tuple[float, float]]) -> float | None:
    """Least-squares slope over the second half of the run (post warm-up)."""
    half = points[len(points) // 2 :]
    if len(half) < 3:
        return None
    n = len(half)
    mean_t = sum(t for t, _ in half) / n
    mean_v = sum(v for _, v in half) / n
    denom = sum((t - mean_t) ** 2 for t, _ in half)
    if denom == 0:
        return None
    slope = sum((t - mean_t) * (v - mean_v) for t, v in half) / denom
    return slope * 60


def compare(paths: list[str]) -> int:
    runs = [(Path(p), _load_run(Path(p))) for p in paths]
    for path, rows in runs:
        events = [(r["time_s"], r["event"]) for r in rows if r.get("event") and r.get("time_s")]
        times = [float(r["time_s"]) for r in rows if r.get("time_s")]
        duration_s = times[-1] if times else 0.0
        print(f"\n{path.name}  ({len(rows)} samples, {duration_s / 60:.1f} min)")
        for field in TREND_FIELDS:
            points = _column(rows, field)
            if not points:
                continue
            values = [v for _, v in points]
            slope = _slope_kb_per_min(points)
            slope_text = f"{slope:+8.0f} kB/min (2nd half)" if slope is not None else "     n/a"
            print(
                f"  {field:>18}: start {values[0]:>9.0f}  peak {max(values):>9.0f}"
                f"  end {values[-1]:>9.0f} kB   trend {slope_text}"
            )
        for time_s, event in events:
            print(f"  event @ {time_s}s: {event}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--label", default="run", help="run label used in the output filename")
    parser.add_argument(
        "--package", default=DEFAULT_PACKAGE, help=f"app package (default: {DEFAULT_PACKAGE})"
    )
    parser.add_argument("--serial", help="adb device serial (for multiple devices)")
    parser.add_argument(
        "--interval", type=float, default=10.0, help="sample interval seconds (default: 10)"
    )
    parser.add_argument(
        "--duration",
        type=_parse_duration,
        help="stop after this long (e.g. 40m); default: until Ctrl+C",
    )
    parser.add_argument("--output-dir", default=".", help="directory for the output CSV")
    parser.add_argument(
        "--compare",
        nargs="+",
        metavar="CSV",
        help="summarize/compare capture CSVs instead of capturing",
    )
    args = parser.parse_args()

    if args.compare:
        return compare(args.compare)
    if not math.isfinite(args.interval) or args.interval <= 0:
        parser.error("--interval must be a positive number of seconds")
    if args.duration is not None and (not math.isfinite(args.duration) or args.duration <= 0):
        parser.error("--duration must be positive")
    return capture(args)


if __name__ == "__main__":
    sys.exit(main())
