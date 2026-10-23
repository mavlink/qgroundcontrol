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
    python3 qt_translate_noop_check.py FILE...
    python3 qt_translate_noop_check.py src/          # Analyze directory
    python3 qt_translate_noop_check.py --json src/   # JSON output

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
        re.compile(r"\breturn\s+QT_TRANSLATE_NOOP\s*\("),
        "QT_TRANSLATE_NOOP result returned directly — string will not be translated",
        'return QCoreApplication::translate("context", "source")',
    ),
    (
        re.compile(r"\breturn\s+QT_TR_NOOP\s*\("),
        "QT_TR_NOOP result returned directly — string will not be translated",
        'return tr("source")',
    ),
    (
        re.compile(r"\.arg\s*\(\s*QT_TRANSLATE_NOOP\s*\("),
        "QT_TRANSLATE_NOOP result passed to .arg() — string will not be translated",
        '.arg(QCoreApplication::translate("context", "source"))',
    ),
    (
        re.compile(r"\.arg\s*\(\s*QT_TR_NOOP\s*\("),
        "QT_TR_NOOP result passed to .arg() — string will not be translated",
        '.arg(tr("source"))',
    ),
    (
        re.compile(r"\bQString\s*\(\s*QT_TRANSLATE_NOOP\s*\("),
        "QT_TRANSLATE_NOOP result wrapped in QString() — string will not be translated",
        'QCoreApplication::translate("context", "source")',
    ),
    (
        re.compile(r"\bQString\s*\(\s*QT_TR_NOOP\s*\("),
        "QT_TR_NOOP result wrapped in QString() — string will not be translated",
        'tr("source")',
    ),
]

GUIDANCE = (
    "QT_TRANSLATE_NOOP only marks strings for extraction by lupdate.\n"
    "Translation must be done at the call site with QCoreApplication::translate().\n"
    "Also check: QString(_member).arg() where _member holds a QT_TRANSLATE_NOOP string.\n"
)


def analyze_file(file_path: Path) -> Generator[Violation, None, None]:
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except OSError as e:
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


class QtTranslateNoopAnalyzer(AnalyzerBase):
    """Detect always-wrong runtime uses of QT_TRANSLATE_NOOP / QT_TR_NOOP."""

    name: ClassVar[str] = "qt-translate-noop-check"
    install_hint: ClassVar[str] = ""

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        del fix
        violations = [v for f in files for v in analyze_file(f)]
        output = "".join(
            f"{v.file}:{v.line}: error: {v.message}\n  code: {v.code}\n  fix:  {v.suggestion}\n\n"
            for v in violations
        )
        return AnalysisResult(
            tool=self.name,
            passed=not violations,
            issues=len(violations),
            output=(output + GUIDANCE) if violations else "",
            files_checked=len(files),
            files_with_issues=sorted({v.file for v in violations}),
            findings=[asdict(v) for v in violations],
        )


def main(argv: list[str] | None = None) -> int:
    return run_line_check_cli(QtTranslateNoopAnalyzer, argv)


if __name__ == "__main__":
    raise SystemExit(main())
