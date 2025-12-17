#!/usr/bin/env python3
"""
QGroundControl Log Analyzer

Analyzes QGC application logs and telemetry logs for debugging and diagnostics.

Usage:
    ./analyze_log.py <logfile>                    # Analyze a log file
    ./analyze_log.py --errors <logfile>           # Show only errors
    ./analyze_log.py --warnings <logfile>         # Show errors and warnings
    ./analyze_log.py --component Vehicle <log>    # Filter by component
    ./analyze_log.py --timeline <logfile>         # Show timeline of events
    ./analyze_log.py --stats <logfile>            # Show statistics

Supports:
    - QGC application logs
    - MAVLink telemetry logs (.tlog)
    - Console output logs

Requirements:
    pip install pymavlink  # For .tlog files
"""

import argparse
import contextlib
import os
import re
import sys
from collections import Counter, defaultdict
from datetime import datetime
from pathlib import Path
from typing import ClassVar

# Try to import pymavlink for tlog support
try:
    from pymavlink import mavutil

    HAS_PYMAVLINK = True
except ImportError:
    HAS_PYMAVLINK = False


class LogEntry:
    """Represents a single log entry."""

    def __init__(self, timestamp=None, level=None, component=None, message=None, raw=None):
        self.timestamp = timestamp
        self.level = level or "INFO"
        self.component = component or "unknown"
        self.message = message or ""
        self.raw = raw or ""

    def __str__(self):
        ts = self.timestamp.strftime("%H:%M:%S.%f")[:-3] if self.timestamp else "??:??:??"
        return f"[{ts}] [{self.level:5}] [{self.component}] {self.message}"


class QGCLogParser:
    """Parser for QGC application logs."""

    # Common QGC log patterns
    PATTERNS: ClassVar[list] = [
        # Qt debug format: "qgc.component: message"
        re.compile(r"^(?P<component>qgc\.[a-z.]+):\s*(?P<message>.*)$", re.IGNORECASE),
        # Timestamped format: "[HH:MM:SS.mmm] message"
        re.compile(r"^\[(?P<time>\d{2}:\d{2}:\d{2}\.?\d*)\]\s*(?P<message>.*)$"),
        # Level format: "WARNING: message" or "[WARNING] message"
        re.compile(
            r"^(?:\[)?(?P<level>DEBUG|INFO|WARNING|ERROR|CRITICAL)(?:\])?\s*:?\s*(?P<message>.*)$",
            re.IGNORECASE,
        ),
    ]

    LEVEL_KEYWORDS: ClassVar[dict] = {
        "error": "ERROR",
        "err": "ERROR",
        "warning": "WARN",
        "warn": "WARN",
        "debug": "DEBUG",
        "info": "INFO",
        "critical": "CRIT",
        "fatal": "CRIT",
    }

    def __init__(self):
        self.entries = []

    def parse_file(self, filepath):
        """Parse a QGC log file."""
        with open(filepath, encoding="utf-8", errors="replace") as f:
            for _line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue

                entry = self._parse_line(line)
                entry.raw = line
                self.entries.append(entry)

        return self.entries

    def _parse_line(self, line):
        """Parse a single log line."""
        entry = LogEntry()

        # Try each pattern
        for pattern in self.PATTERNS:
            match = pattern.match(line)
            if match:
                groups = match.groupdict()
                if "component" in groups:
                    entry.component = groups["component"]
                if "message" in groups:
                    entry.message = groups["message"]
                if "level" in groups:
                    entry.level = groups["level"].upper()
                if "time" in groups:
                    try:
                        entry.timestamp = datetime.strptime(groups["time"], "%H:%M:%S.%f")
                    except ValueError:
                        with contextlib.suppress(ValueError):
                            entry.timestamp = datetime.strptime(groups["time"], "%H:%M:%S")
                break

        # If no pattern matched, use the whole line as message
        if not entry.message:
            entry.message = line

        # Detect level from keywords in message
        if entry.level == "INFO":
            lower_msg = line.lower()
            for keyword, level in self.LEVEL_KEYWORDS.items():
                if keyword in lower_msg:
                    entry.level = level
                    break

        return entry


class TLogParser:
    """Parser for MAVLink telemetry logs."""

    def __init__(self):
        if not HAS_PYMAVLINK:
            raise ImportError(
                "pymavlink required for .tlog files. Install with: pip install pymavlink"
            )
        self.entries = []

    def parse_file(self, filepath):
        """Parse a MAVLink telemetry log."""
        mlog = mavutil.mavlink_connection(filepath)

        while True:
            msg = mlog.recv_match(blocking=False)
            if msg is None:
                break

            entry = LogEntry(
                timestamp=datetime.fromtimestamp(msg._timestamp)
                if hasattr(msg, "_timestamp")
                else None,
                level="INFO",
                component=f"MAV:{msg.get_srcSystem()}",
                message=f"{msg.get_type()}: {msg.to_dict()}",
            )
            self.entries.append(entry)

        return self.entries


class LogAnalyzer:
    """Analyzes parsed log entries."""

    def __init__(self, entries):
        self.entries = entries

    def filter_by_level(self, min_level):
        """Filter entries by minimum level."""
        levels = {
            "DEBUG": 0,
            "INFO": 1,
            "WARN": 2,
            "WARNING": 2,
            "ERROR": 3,
            "CRIT": 4,
            "CRITICAL": 4,
        }
        min_val = levels.get(min_level.upper(), 0)
        return [e for e in self.entries if levels.get(e.level, 0) >= min_val]

    def filter_by_component(self, component):
        """Filter entries by component."""
        pattern = re.compile(component, re.IGNORECASE)
        return [e for e in self.entries if pattern.search(e.component)]

    def filter_by_message(self, pattern):
        """Filter entries by message pattern."""
        regex = re.compile(pattern, re.IGNORECASE)
        return [e for e in self.entries if regex.search(e.message)]

    def get_statistics(self):
        """Get log statistics."""
        stats = {
            "total_entries": len(self.entries),
            "by_level": Counter(e.level for e in self.entries),
            "by_component": Counter(e.component for e in self.entries),
            "errors": [e for e in self.entries if e.level in ("ERROR", "CRIT", "CRITICAL")],
            "warnings": [e for e in self.entries if e.level in ("WARN", "WARNING")],
        }

        # Time range
        timestamps = [e.timestamp for e in self.entries if e.timestamp]
        if timestamps:
            stats["time_range"] = (min(timestamps), max(timestamps))

        return stats

    def get_timeline(self, interval_seconds=60):
        """Group entries by time interval."""
        timeline = defaultdict(list)

        for entry in self.entries:
            if entry.timestamp:
                # Round to interval
                ts = entry.timestamp.replace(
                    second=(entry.timestamp.second // interval_seconds) * interval_seconds,
                    microsecond=0,
                )
                timeline[ts].append(entry)

        return dict(sorted(timeline.items()))


def print_entries(entries, max_entries=None):
    """Print log entries."""
    for i, entry in enumerate(entries):
        if max_entries and i >= max_entries:
            print(f"... and {len(entries) - max_entries} more entries")
            break
        print(entry)


def print_statistics(stats):
    """Print log statistics."""
    print(f"\n{'=' * 60}")
    print("LOG STATISTICS")
    print(f"{'=' * 60}")

    print(f"\nTotal entries: {stats['total_entries']}")

    print("\nBy level:")
    for level, count in sorted(stats["by_level"].items()):
        print(f"  {level}: {count}")

    print("\nTop components:")
    for component, count in stats["by_component"].most_common(10):
        print(f"  {component}: {count}")

    if "time_range" in stats:
        start, end = stats["time_range"]
        print(f"\nTime range: {start} - {end}")

    if stats["errors"]:
        print(f"\nErrors ({len(stats['errors'])}):")
        for entry in stats["errors"][:5]:
            print(f"  {entry.message[:80]}")
        if len(stats["errors"]) > 5:
            print(f"  ... and {len(stats['errors']) - 5} more")


def main():
    parser = argparse.ArgumentParser(
        description="Analyze QGroundControl logs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("logfile", help="Log file to analyze")
    parser.add_argument("-e", "--errors", action="store_true", help="Show only errors")
    parser.add_argument("-w", "--warnings", action="store_true", help="Show errors and warnings")
    parser.add_argument("-c", "--component", metavar="PATTERN", help="Filter by component name")
    parser.add_argument("-m", "--message", metavar="PATTERN", help="Filter by message pattern")
    parser.add_argument("-s", "--stats", action="store_true", help="Show statistics")
    parser.add_argument("-t", "--timeline", action="store_true", help="Show timeline")
    parser.add_argument(
        "-n",
        "--max",
        type=int,
        default=100,
        help="Maximum entries to show (default: 100)",
    )

    args = parser.parse_args()

    if not os.path.exists(args.logfile):
        print(f"Error: File not found: {args.logfile}", file=sys.stderr)
        sys.exit(1)

    # Select parser based on file extension
    filepath = Path(args.logfile)

    if filepath.suffix.lower() == ".tlog":
        if not HAS_PYMAVLINK:
            print("Error: pymavlink required for .tlog files", file=sys.stderr)
            print("Install with: pip install pymavlink", file=sys.stderr)
            sys.exit(1)
        log_parser = TLogParser()
    else:
        log_parser = QGCLogParser()

    # Parse the file
    print(f"Parsing {args.logfile}...")
    entries = log_parser.parse_file(args.logfile)
    print(f"Found {len(entries)} entries")

    # Analyze
    analyzer = LogAnalyzer(entries)

    # Apply filters
    if args.errors:
        entries = analyzer.filter_by_level("ERROR")
    elif args.warnings:
        entries = analyzer.filter_by_level("WARN")

    if args.component:
        entries = [e for e in entries if re.search(args.component, e.component, re.IGNORECASE)]

    if args.message:
        entries = [e for e in entries if re.search(args.message, e.message, re.IGNORECASE)]

    # Output
    if args.stats:
        stats = LogAnalyzer(entries).get_statistics()
        print_statistics(stats)
    elif args.timeline:
        timeline = LogAnalyzer(entries).get_timeline()
        for ts, group in timeline.items():
            print(f"\n[{ts}] ({len(group)} entries)")
            for entry in group[:3]:
                print(f"  {entry.level}: {entry.message[:60]}")
            if len(group) > 3:
                print(f"  ... and {len(group) - 3} more")
    else:
        print_entries(entries, args.max)


if __name__ == "__main__":
    main()
