#!/usr/bin/env python3
"""
QT_TRANSLATE_NOOP Misuse Analyzer for QGroundControl

QT_TRANSLATE_NOOP is a marker-only macro — it expands to the raw string literal
and does NOT translate at runtime. Translation must be done at the call site via
QCoreApplication::translate() or tr().

Detects always-wrong patterns in C++ files for both QT_TRANSLATE_NOOP and QT_TR_NOOP:

1. return QT_TRANSLATE_NOOP(...)
   The macro result is returned directly from a function. The caller receives an
   untranslated string. Fix: return QCoreApplication::translate("ctx", "str")

2. .arg(QT_TRANSLATE_NOOP(...))  /  QString(QT_TRANSLATE_NOOP(...))
   The macro result is used as a runtime string value without translate().
   Fix: .arg(QCoreApplication::translate("ctx", "str"))

Usage:
    python3 qt_translate_noop_check.py [files...]
    python3 qt_translate_noop_check.py src/          # Analyze directory
    python3 qt_translate_noop_check.py               # Analyze stdin (for pre-commit)

Exit codes:
    0 - No issues found
    1 - Issues found
"""

import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Generator

# Add tools to path for imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from common.file_traversal import find_cpp_files


@dataclass
class Violation:
    file: str
    line: int
    code: str
    message: str
    suggestion: str


# Patterns that are always wrong: QT_TRANSLATE_NOOP / QT_TR_NOOP used as a runtime string value
_WRONG_PATTERNS = [
    (
        re.compile(r'\breturn\s+QT_TRANSLATE_NOOP\s*\('),
        "QT_TRANSLATE_NOOP result returned directly — string will not be translated",
        "return QCoreApplication::translate(\"context\", \"source\")",
    ),
    (
        re.compile(r'\breturn\s+QT_TR_NOOP\s*\('),
        "QT_TR_NOOP result returned directly — string will not be translated",
        "return tr(\"source\")",
    ),
    (
        re.compile(r'\.arg\s*\(\s*QT_TRANSLATE_NOOP\s*\('),
        "QT_TRANSLATE_NOOP result passed to .arg() — string will not be translated",
        ".arg(QCoreApplication::translate(\"context\", \"source\"))",
    ),
    (
        re.compile(r'\.arg\s*\(\s*QT_TR_NOOP\s*\('),
        "QT_TR_NOOP result passed to .arg() — string will not be translated",
        ".arg(tr(\"source\"))",
    ),
    (
        re.compile(r'\bQString\s*\(\s*QT_TRANSLATE_NOOP\s*\('),
        "QT_TRANSLATE_NOOP result wrapped in QString() — string will not be translated",
        "QCoreApplication::translate(\"context\", \"source\")",
    ),
    (
        re.compile(r'\bQString\s*\(\s*QT_TR_NOOP\s*\('),
        "QT_TR_NOOP result wrapped in QString() — string will not be translated",
        "tr(\"source\")",
    ),
]


def analyze_file(file_path: Path) -> Generator[Violation, None, None]:
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return

    for line_num, line in enumerate(content.splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("//"):
            continue
        for pattern, message, suggestion in _WRONG_PATTERNS:
            if pattern.search(line):
                yield Violation(
                    file=str(file_path),
                    line=line_num,
                    code=stripped,
                    message=message,
                    suggestion=suggestion,
                )
                break  # one violation per line is enough


def main() -> int:
    args = sys.argv[1:]

    if not args:
        # Pre-commit mode: read file list from stdin
        files = [Path(f.strip()) for f in sys.stdin if f.strip()]
    else:
        files = []
        for arg in args:
            path = Path(arg)
            if path.is_dir():
                files.extend(find_cpp_files([path]))
            elif path.is_file():
                files.append(path)

    violations: list[Violation] = []
    for f in files:
        violations.extend(analyze_file(f))

    if not violations:
        return 0

    for v in violations:
        print(f"{v.file}:{v.line}: error: {v.message}")
        print(f"  code: {v.code}")
        print(f"  fix:  {v.suggestion}")
        print()

    print(
        f"{len(violations)} QT_TRANSLATE_NOOP misuse(s) found.\n"
        "QT_TRANSLATE_NOOP only marks strings for extraction by lupdate.\n"
        "Translation must be done at the call site with QCoreApplication::translate().\n"
        "Also check: QString(_member).arg() where _member holds a QT_TRANSLATE_NOOP string."
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
