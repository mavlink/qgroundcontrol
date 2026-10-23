"""
Vehicle Null-Check Analyzer for QGroundControl

Detects unsafe patterns:
1. activeVehicle()->method() without null check
2. A variable assigned from activeVehicle() dereferenced without null check

ParameterManager::getParameter() is not checked: it never returns null and
returns a default Fact (after reporting the missing parameter) instead.

Usage:
    python3 vehicle_null_check.py FILE...
    python3 vehicle_null_check.py src/          # Analyze directory
    python3 vehicle_null_check.py --json src/   # JSON output

Exit codes:
    0 - No issues found
    1 - Issues found
"""

import re
import sys
from collections.abc import Generator
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import ClassVar

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap

from analyzers.base import AnalysisResult, AnalyzerBase, run_line_check_cli

# Captures the method called on activeVehicle()
ACTIVE_VEHICLE_DIRECT_PATTERN = re.compile(r"activeVehicle\(\)\s*->\s*(\w+)\s*\(")

# Captures the variable assigned from activeVehicle()
ACTIVE_VEHICLE_ASSIGN_PATTERN = re.compile(
    r"(\w+)\s*=\s*(?:MultiVehicleManager::instance\(\)->)?activeVehicle\(\)"
)

# \w+ is replaced by the variable name when checking a specific variable
NULL_CHECK_PATTERNS = [
    r"if\s*\(\s*!\s*\w+\s*\)",  # if (!var)
    r"if\s*\(\s*\w+\s*==\s*nullptr\s*\)",  # if (var == nullptr)
    r"if\s*\(\s*\w+\s*!=\s*nullptr\s*\)",  # if (var != nullptr)
    r"if\s*\(\s*\w+\s*\)",  # if (var)
    r"\?\s*:",  # ternary operator
]


@dataclass
class Violation:
    """A detected null-safety violation."""

    file: str
    line: int
    column: int
    pattern: str
    code: str
    suggestion: str


def has_null_check_before(lines: list[str], current_line: int, var_name: str | None = None) -> bool:
    """Check for a null check in the preceding lines or, for var_name, earlier on its use line."""
    start = max(0, current_line - 10)
    context = "\n".join(lines[start:current_line])

    if not var_name:
        return any(re.search(pattern, context) for pattern in NULL_CHECK_PATTERNS)

    # Generic patterns would accept a check of any unrelated variable nearby
    var = re.escape(var_name)
    use = re.search(rf"\b{var}\s*->", lines[current_line])
    guard = rf"\b{var}\s*(?:\?|&&)|!\s*{var}\s*\|\|"
    if use and re.search(guard, lines[current_line][: use.start()]):
        return True
    specific_patterns = [p.replace(r"\w+", var) for p in NULL_CHECK_PATTERNS if r"\w+" in p]
    specific_patterns += [
        rf"if\s*\([^)]*\b{var}\b[^)]*\)\s*\{{",
        rf"QVERIFY2?\s*\(\s*{var}\s*[,)]",
    ]
    return any(re.search(pattern, context) for pattern in specific_patterns)


def analyze_file(file_path: Path) -> Generator[Violation, None, None]:
    """Analyze a single C++ file for null-check violations."""
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except OSError as e:
        print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return

    lines = content.split("\n")

    for line_num, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("/*"):
            continue

        # Check for direct activeVehicle()->method() calls
        for match in ACTIVE_VEHICLE_DIRECT_PATTERN.finditer(line):
            if not has_null_check_before(lines, line_num):
                yield Violation(
                    file=str(file_path),
                    line=line_num + 1,
                    column=match.start() + 1,
                    pattern="unsafe_active_vehicle_direct",
                    code=stripped,
                    suggestion=(
                        "Add null check before using activeVehicle():\n"
                        "  Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();\n"
                        "  if (!vehicle) return;"
                    ),
                )

        # Check for activeVehicle() assignment followed by use without check
        assign_match = ACTIVE_VEHICLE_ASSIGN_PATTERN.search(line)
        if assign_match:
            var_name = assign_match.group(1)
            for future_line_num in range(line_num + 1, min(line_num + 5, len(lines))):
                future_line = lines[future_line_num]
                if re.search(rf"\b{re.escape(var_name)}\s*->", future_line):
                    if not has_null_check_before(lines, future_line_num, var_name):
                        yield Violation(
                            file=str(file_path),
                            line=future_line_num + 1,
                            column=1,
                            pattern="unsafe_active_vehicle_use",
                            code=future_line.strip(),
                            suggestion=(
                                f"Add null check after assigning {var_name}:\n"
                                f"  if (!{var_name}) return;"
                            ),
                        )
                    break


class VehicleNullCheckAnalyzer(AnalyzerBase):
    """Detect unsafe activeVehicle() patterns."""

    name: ClassVar[str] = "vehicle-null-check"
    install_hint: ClassVar[str] = ""

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        del fix
        violations = [v for f in files for v in analyze_file(f)]
        return AnalysisResult(
            tool=self.name,
            passed=not violations,
            issues=len(violations),
            output="\n".join(format_violation(v) for v in violations),
            files_checked=len(files),
            files_with_issues=sorted({v.file for v in violations}),
            findings=[asdict(v) for v in violations],
        )


def format_violation(v: Violation) -> str:
    """Format a violation for human-readable output."""
    return (
        f"{v.file}:{v.line}:{v.column}: warning: {v.pattern}\n"
        f"  {v.code}\n"
        f"  Suggestion: {v.suggestion}\n"
    )


def main(argv: list[str] | None = None) -> int:
    return run_line_check_cli(VehicleNullCheckAnalyzer, argv)


if __name__ == "__main__":
    raise SystemExit(main())
