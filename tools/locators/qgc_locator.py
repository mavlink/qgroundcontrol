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

Options:
    --json    Output results as JSON array
    --limit N Maximum results (default: 50)

Output format: name<TAB>path:line (for editor integration)

Examples:
    qgc_locator.py fact lat           # Find all Facts containing 'lat'
    qgc_locator.py factgroup gps      # Find FactGroup classes with 'gps'
    qgc_locator.py mavlink HEARTBEAT  # Find MAVLINK_MSG_ID_HEARTBEAT usage
    qgc_locator.py --json fact lat    # JSON output
"""

import json
import sys
from pathlib import Path
from typing import Generator, NamedTuple

# Add tools to path for imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

from common.patterns import (
    FACT_MEMBER_PATTERN,
    FACTGROUP_CLASS_PATTERN,
    MAVLINK_MSG_ID_PATTERN,
    PARAM_NAME_PATTERN,
    make_query_pattern,
)
from common.file_traversal import (
    find_repo_root,
    find_header_files,
    find_source_files,
    find_json_files,
)


class SearchResult(NamedTuple):
    """A search result with name, file path, and line number."""
    name: str
    path: str
    line: int


REPO_ROOT = find_repo_root(Path(__file__))


def search_with_pattern(
    pattern,
    files: Generator[Path, None, None],
    query: str,
) -> Generator[SearchResult, None, None]:
    """Generic search using a pattern across files."""
    query_pattern = make_query_pattern(pattern, query)

    for path in files:
        try:
            content = path.read_text(encoding='utf-8', errors='replace')
            for i, line in enumerate(content.split('\n'), 1):
                if match := query_pattern.search(line):
                    yield SearchResult(
                        match.group(1),
                        str(path.relative_to(REPO_ROOT)),
                        i
                    )
        except Exception:
            continue


def find_facts(query: str) -> Generator[SearchResult, None, None]:
    """Find Fact member variables matching query."""
    return search_with_pattern(
        FACT_MEMBER_PATTERN,
        find_header_files(REPO_ROOT / 'src'),
        query
    )


def find_factgroups(query: str) -> Generator[SearchResult, None, None]:
    """Find FactGroup class definitions matching query."""
    return search_with_pattern(
        FACTGROUP_CLASS_PATTERN,
        find_header_files(REPO_ROOT / 'src'),
        query
    )


def find_mavlink(query: str) -> Generator[SearchResult, None, None]:
    """Find MAVLink message ID usage matching query."""
    seen = set()
    for result in search_with_pattern(
        MAVLINK_MSG_ID_PATTERN,
        find_source_files(REPO_ROOT / 'src'),
        query
    ):
        key = (result.name, result.path)
        if key not in seen:
            seen.add(key)
            yield result


def find_params(query: str) -> Generator[SearchResult, None, None]:
    """Find parameter names in FactMetaData JSON files."""
    return search_with_pattern(
        PARAM_NAME_PATTERN,
        find_json_files(REPO_ROOT / 'src'),
        query
    )


COMMANDS = {
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


def main() -> int:
    """Main entry point."""
    args = sys.argv[1:]
    json_output = False
    max_results = 50

    # Parse options
    if '--json' in args:
        json_output = True
        args.remove('--json')

    if '--limit' in args:
        idx = args.index('--limit')
        max_results = int(args[idx + 1])
        args = args[:idx] + args[idx + 2:]

    if '-h' in args or '--help' in args or len(args) < 2:
        print(__doc__)
        return 0 if '-h' in args or '--help' in args else 1

    command = args[0].lower()
    query = args[1]

    if command not in COMMANDS:
        print(f"Unknown command: {command}", file=sys.stderr)
        print(f"Available: {', '.join(sorted(set(COMMANDS.keys())))}", file=sys.stderr)
        return 1

    results = []
    for i, result in enumerate(COMMANDS[command](query)):
        if i >= max_results:
            break
        results.append(result)

    if json_output:
        print(json.dumps([r._asdict() for r in results], indent=2))
    else:
        for result in results:
            print(f"{result.name}\t{result.path}:{result.line}")

        if not results:
            print(f"No results found for '{query}'", file=sys.stderr)
            return 1

        if len(results) == max_results:
            print(f"... (showing first {max_results} results)", file=sys.stderr)

    return 0


if __name__ == '__main__':
    sys.exit(main())
