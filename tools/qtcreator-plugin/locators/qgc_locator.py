#!/usr/bin/env python3
"""
QGC Locator - Search for Facts, FactGroups, and MAVLink messages

A CLI tool for quickly finding QGC-specific code elements.
Can be integrated with QtCreator's External Tools or any editor.

Usage:
    qgc_locator.py fact <query>       # Search Fact names
    qgc_locator.py factgroup <query>  # Search FactGroup classes
    qgc_locator.py mavlink <query>    # Search MAVLink message usage
    qgc_locator.py param <query>      # Search parameter names in JSON

Output format: name<TAB>path:line (for editor integration)

Examples:
    qgc_locator.py fact lat           # Find all Facts containing 'lat'
    qgc_locator.py factgroup gps      # Find FactGroup classes with 'gps'
    qgc_locator.py mavlink HEARTBEAT  # Find MAVLINK_MSG_ID_HEARTBEAT usage
    qgc_locator.py param BATT         # Find parameters starting with BATT
"""

import re
import sys
import json
from pathlib import Path
from typing import Generator, NamedTuple


class SearchResult(NamedTuple):
    """A search result with name, file path, and line number."""
    name: str
    path: Path
    line: int


# Find repository root (look for .git directory)
def find_repo_root() -> Path:
    """Find the repository root by looking for .git directory."""
    current = Path(__file__).resolve()
    for parent in [current] + list(current.parents):
        if (parent / '.git').exists():
            return parent
    # Fallback: assume we're in tools/qtcreator-plugin/locators/
    return Path(__file__).resolve().parent.parent.parent.parent


REPO_ROOT = find_repo_root()


def find_facts(query: str) -> Generator[SearchResult, None, None]:
    """Find Fact member variables matching query."""
    pattern = re.compile(rf'Fact\s+_(\w*{re.escape(query)}\w*)Fact\s*=', re.IGNORECASE)

    for path in REPO_ROOT.glob('src/**/*.h'):
        if 'build' in str(path) or 'libs' in str(path):
            continue
        try:
            content = path.read_text(encoding='utf-8', errors='replace')
            for i, line in enumerate(content.split('\n'), 1):
                if match := pattern.search(line):
                    yield SearchResult(match.group(1), path.relative_to(REPO_ROOT), i)
        except Exception:
            continue


def find_factgroups(query: str) -> Generator[SearchResult, None, None]:
    """Find FactGroup class definitions matching query."""
    pattern = re.compile(rf'class\s+(\w*{re.escape(query)}\w*FactGroup)\s*:', re.IGNORECASE)

    for path in REPO_ROOT.glob('src/**/*.h'):
        if 'build' in str(path) or 'libs' in str(path):
            continue
        try:
            content = path.read_text(encoding='utf-8', errors='replace')
            for i, line in enumerate(content.split('\n'), 1):
                if match := pattern.search(line):
                    yield SearchResult(match.group(1), path.relative_to(REPO_ROOT), i)
        except Exception:
            continue


def find_mavlink(query: str) -> Generator[SearchResult, None, None]:
    """Find MAVLink message ID usage matching query."""
    pattern = re.compile(rf'MAVLINK_MSG_ID_(\w*{re.escape(query)}\w*)', re.IGNORECASE)
    seen = set()  # Deduplicate same message ID in same file

    for path in REPO_ROOT.glob('src/**/*.cc'):
        if 'build' in str(path) or 'libs' in str(path):
            continue
        try:
            content = path.read_text(encoding='utf-8', errors='replace')
            for i, line in enumerate(content.split('\n'), 1):
                if match := pattern.search(line):
                    msg_id = match.group(1)
                    key = (msg_id, path)
                    if key not in seen:
                        seen.add(key)
                        yield SearchResult(msg_id, path.relative_to(REPO_ROOT), i)
        except Exception:
            continue


def find_params(query: str) -> Generator[SearchResult, None, None]:
    """Find parameter names in FactMetaData JSON files."""
    pattern = re.compile(rf'"name"\s*:\s*"(\w*{re.escape(query)}\w*)"', re.IGNORECASE)

    for path in REPO_ROOT.glob('src/**/*Fact.json'):
        if 'build' in str(path) or 'libs' in str(path):
            continue
        try:
            content = path.read_text(encoding='utf-8', errors='replace')
            for i, line in enumerate(content.split('\n'), 1):
                if match := pattern.search(line):
                    yield SearchResult(match.group(1), path.relative_to(REPO_ROOT), i)
        except Exception:
            continue


def print_results(results: Generator[SearchResult, None, None], max_results: int = 50) -> int:
    """Print search results in editor-friendly format."""
    count = 0
    for result in results:
        if count >= max_results:
            print(f"... (showing first {max_results} results)", file=sys.stderr)
            break
        print(f"{result.name}\t{result.path}:{result.line}")
        count += 1
    return count


def main() -> int:
    """Main entry point."""
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    command = sys.argv[1].lower()
    query = sys.argv[2]

    commands = {
        'fact': find_facts,
        'facts': find_facts,
        'f': find_facts,
        'factgroup': find_factgroups,
        'factgroups': find_factgroups,
        'fg': find_factgroups,
        'mavlink': find_mavlink,
        'mav': find_mavlink,
        'm': find_mavlink,
        'param': find_params,
        'params': find_params,
        'p': find_params,
    }

    if command not in commands:
        print(f"Unknown command: {command}", file=sys.stderr)
        print(f"Available commands: {', '.join(sorted(set(commands.keys())))}", file=sys.stderr)
        return 1

    count = print_results(commands[command](query))

    if count == 0:
        print(f"No results found for '{query}'", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
