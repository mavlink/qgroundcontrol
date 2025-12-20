#!/usr/bin/env python3
"""
Vehicle Null-Check Analyzer for QGroundControl

Detects unsafe patterns:
1. activeVehicle()->method() without null check
2. getParameter() result used without null validation

Usage:
    python3 vehicle_null_check.py [files...]
    python3 vehicle_null_check.py src/          # Analyze directory
    python3 vehicle_null_check.py               # Analyze stdin (for pre-commit)

Exit codes:
    0 - No issues found
    1 - Issues found
"""

import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Generator

@dataclass
class Violation:
    """A detected null-safety violation."""
    file: Path
    line: int
    column: int
    pattern: str
    code: str
    suggestion: str


# Pattern: activeVehicle() followed by -> without intervening null check
# Captures: method call immediately after activeVehicle()
UNSAFE_ACTIVE_VEHICLE_DIRECT = re.compile(
    r'activeVehicle\(\)\s*->\s*(\w+)\s*\('
)

# Pattern: Variable assigned from activeVehicle(), then used with -> on same/next lines
ACTIVE_VEHICLE_ASSIGN = re.compile(
    r'(\w+)\s*=\s*(?:MultiVehicleManager::instance\(\)->)?activeVehicle\(\)'
)

# Pattern: getParameter() result used directly without check
UNSAFE_GET_PARAMETER = re.compile(
    r'getParameter\s*\([^)]*\)\s*->\s*(\w+)\s*\('
)

# Null check patterns that make the code safe
NULL_CHECK_PATTERNS = [
    r'if\s*\(\s*!\s*\w+\s*\)',           # if (!var)
    r'if\s*\(\s*\w+\s*==\s*nullptr\s*\)', # if (var == nullptr)
    r'if\s*\(\s*\w+\s*!=\s*nullptr\s*\)', # if (var != nullptr)
    r'if\s*\(\s*\w+\s*\)',                # if (var)
    r'\?\s*:',                            # ternary operator
]


def has_null_check_before(lines: list[str], current_line: int, var_name: str = None) -> bool:
    """Check if there's a null check in the preceding lines (within same function scope)."""
    # Look back up to 10 lines for a null check
    start = max(0, current_line - 10)
    context = '\n'.join(lines[start:current_line])

    for pattern in NULL_CHECK_PATTERNS:
        if var_name:
            # Check for specific variable
            specific_pattern = pattern.replace(r'\w+', re.escape(var_name))
            if re.search(specific_pattern, context):
                return True
        if re.search(pattern, context):
            return True

    # Also check if we're inside an if block that checked the variable
    # Simple heuristic: look for "if (!vehicle)" or similar before this line
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
        # Skip comments
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*'):
            continue

        # Check for direct activeVehicle()->method() calls
        for match in UNSAFE_ACTIVE_VEHICLE_DIRECT.finditer(line):
            method_name = match.group(1)
            if not has_null_check_before(lines, line_num):
                yield Violation(
                    file=file_path,
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
        assign_match = ACTIVE_VEHICLE_ASSIGN.search(line)
        if assign_match:
            var_name = assign_match.group(1)
            # Look ahead for usage without null check
            for future_line_num in range(line_num + 1, min(line_num + 5, len(lines))):
                future_line = lines[future_line_num]
                if re.search(rf'\b{re.escape(var_name)}\s*->', future_line):
                    if not has_null_check_before(lines, future_line_num, var_name):
                        yield Violation(
                            file=file_path,
                            line=future_line_num + 1,
                            column=1,
                            pattern='unsafe_active_vehicle_use',
                            code=future_line.strip(),
                            suggestion=(
                                f'Add null check after assigning {var_name}:\n'
                                f'  if (!{var_name}) return;'
                            )
                        )
                    break  # Only report first use

        # Check for getParameter() result used directly
        for match in UNSAFE_GET_PARAMETER.finditer(line):
            method_name = match.group(1)
            yield Violation(
                file=file_path,
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


def find_cpp_files(paths: list[Path]) -> Generator[Path, None, None]:
    """Find all C++ files in the given paths."""
    for path in paths:
        if path.is_file():
            if path.suffix in ('.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx'):
                yield path
        elif path.is_dir():
            for pattern in ('**/*.cc', '**/*.cpp', '**/*.h', '**/*.hpp'):
                for file_path in path.glob(pattern):
                    # Skip build directories and libs
                    if any(skip in str(file_path) for skip in ('build/', 'libs/', 'test/', '.cache/')):
                        continue
                    yield file_path


def format_violation(v: Violation) -> str:
    """Format a violation for output."""
    return (
        f"{v.file}:{v.line}:{v.column}: warning: {v.pattern}\n"
        f"  {v.code}\n"
        f"  Suggestion: {v.suggestion}\n"
    )


def main() -> int:
    """Main entry point."""
    # Parse arguments
    if len(sys.argv) > 1:
        paths = [Path(arg) for arg in sys.argv[1:]]
    else:
        # Read file list from stdin (for pre-commit)
        paths = [Path(line.strip()) for line in sys.stdin if line.strip()]

    if not paths:
        print("Usage: vehicle_null_check.py [files or directories...]", file=sys.stderr)
        return 0

    violations = []
    for file_path in find_cpp_files(paths):
        for violation in analyze_file(file_path):
            violations.append(violation)

    # Output violations
    for v in violations:
        print(format_violation(v))

    if violations:
        print(f"\nFound {len(violations)} potential null-safety issue(s).", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
