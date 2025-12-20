#!/usr/bin/env python3
"""
Vehicle Null-Check Analyzer for QGroundControl

Detects unsafe patterns:
1. activeVehicle()->method() without null check
2. getParameter() result used without null validation

Usage:
    python3 vehicle_null_check.py [files...]
    python3 vehicle_null_check.py src/          # Analyze directory
    python3 vehicle_null_check.py --json src/   # JSON output
    python3 vehicle_null_check.py               # Analyze stdin (for pre-commit)

Exit codes:
    0 - No issues found
    1 - Issues found
"""

import json
import re
import sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Generator

# Add tools to path for imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from common.patterns import (
    ACTIVE_VEHICLE_DIRECT_PATTERN,
    ACTIVE_VEHICLE_ASSIGN_PATTERN,
    GET_PARAMETER_DIRECT_PATTERN,
    NULL_CHECK_PATTERNS,
)
from common.file_traversal import find_cpp_files


@dataclass
class Violation:
    """A detected null-safety violation."""
    file: str
    line: int
    column: int
    pattern: str
    code: str
    suggestion: str


def has_null_check_before(lines: list[str], current_line: int, var_name: str = None) -> bool:
    """Check if there's a null check in the preceding lines (within same function scope)."""
    start = max(0, current_line - 10)
    context = '\n'.join(lines[start:current_line])

    for pattern in NULL_CHECK_PATTERNS:
        if var_name:
            specific_pattern = pattern.replace(r'\w+', re.escape(var_name))
            if re.search(specific_pattern, context):
                return True
        if re.search(pattern, context):
            return True

    if var_name and re.search(rf'if\s*\([^)]*{re.escape(var_name)}[^)]*\)\s*\{{', context):
        return True

    return False


def analyze_file(file_path: Path) -> Generator[Violation, None, None]:
    """Analyze a single C++ file for null-check violations."""
    try:
        content = file_path.read_text(encoding='utf-8', errors='replace')
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return

    lines = content.split('\n')

    for line_num, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*'):
            continue

        # Check for direct activeVehicle()->method() calls
        for match in ACTIVE_VEHICLE_DIRECT_PATTERN.finditer(line):
            if not has_null_check_before(lines, line_num):
                yield Violation(
                    file=str(file_path),
                    line=line_num + 1,
                    column=match.start() + 1,
                    pattern='unsafe_active_vehicle_direct',
                    code=stripped,
                    suggestion=(
                        'Add null check before using activeVehicle():\n'
                        '  Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();\n'
                        '  if (!vehicle) return;'
                    )
                )

        # Check for activeVehicle() assignment followed by use without check
        assign_match = ACTIVE_VEHICLE_ASSIGN_PATTERN.search(line)
        if assign_match:
            var_name = assign_match.group(1)
            for future_line_num in range(line_num + 1, min(line_num + 5, len(lines))):
                future_line = lines[future_line_num]
                if re.search(rf'\b{re.escape(var_name)}\s*->', future_line):
                    if not has_null_check_before(lines, future_line_num, var_name):
                        yield Violation(
                            file=str(file_path),
                            line=future_line_num + 1,
                            column=1,
                            pattern='unsafe_active_vehicle_use',
                            code=future_line.strip(),
                            suggestion=(
                                f'Add null check after assigning {var_name}:\n'
                                f'  if (!{var_name}) return;'
                            )
                        )
                    break

        # Check for getParameter() result used directly
        for match in GET_PARAMETER_DIRECT_PATTERN.finditer(line):
            yield Violation(
                file=str(file_path),
                line=line_num + 1,
                column=match.start() + 1,
                pattern='unsafe_get_parameter',
                code=stripped,
                suggestion=(
                    'getParameter() can return nullptr. Store result and check:\n'
                    '  Fact *param = vehicle->parameterManager()->getParameter(...);\n'
                    '  if (!param) { qCWarning(...) << "Parameter not found"; return; }'
                )
            )


def format_violation(v: Violation) -> str:
    """Format a violation for human-readable output."""
    return (
        f"{v.file}:{v.line}:{v.column}: warning: {v.pattern}\n"
        f"  {v.code}\n"
        f"  Suggestion: {v.suggestion}\n"
    )


def main() -> int:
    """Main entry point."""
    json_output = False
    args = sys.argv[1:]

    if '--json' in args:
        json_output = True
        args.remove('--json')

    if '-h' in args or '--help' in args:
        print(__doc__)
        return 0

    if args:
        paths = [Path(arg) for arg in args]
    else:
        paths = [Path(line.strip()) for line in sys.stdin if line.strip()]

    if not paths:
        print("Usage: vehicle_null_check.py [--json] [files or directories...]", file=sys.stderr)
        return 0

    violations = list(v for file_path in find_cpp_files(paths) for v in analyze_file(file_path))

    if json_output:
        print(json.dumps([asdict(v) for v in violations], indent=2))
    else:
        for v in violations:
            print(format_violation(v))
        if violations:
            print(f"\nFound {len(violations)} potential null-safety issue(s).", file=sys.stderr)

    return 1 if violations else 0


if __name__ == '__main__':
    sys.exit(main())
