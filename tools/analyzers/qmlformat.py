"""qmlformat check for QML files.

Usage:
    python3 tools/analyzers/qmlformat.py FILE...

Each file is formatted to stdout with its nearest .qmlformat.ini and compared with the file on disk.
Differences are printed as unified diffs; files are never rewritten (apply with ``qmlformat -i``).
"""

from __future__ import annotations

import argparse
import difflib
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import ClassVar

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.logging import log_error, log_info, log_ok, log_warn
from common.proc import run_captured

from analyzers.base import AnalysisResult, AnalyzerBase


class QmlFormatAnalyzer(AnalyzerBase):
    """Reports QML files whose qmlformat output differs from their contents."""

    name: ClassVar[str] = "qmlformat"
    install_hint: ClassVar[str] = "Install the Qt SDK and put its bin directory on PATH"
    show_diff: bool = False

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_tool("qmlformat"):
            return AnalysisResult(
                tool=self.name, passed=False, execution_error=True, output="qmlformat not found"
            )
        if fix:
            log_warn("qmlformat only checks; apply its formatting with: qmlformat -i FILE")
        if not files:
            log_info("No QML files to check")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Checking {len(files)} QML files with qmlformat...")
        with ThreadPoolExecutor() as pool:
            checks = list(pool.map(self._check, files))

        needs_format: list[str] = []
        failed: list[str] = []
        output: list[str] = []
        for file, (ran, message) in zip(files, checks, strict=True):
            if not ran:
                failed.append(self.relative_path(file))
                log_error(f"qmlformat failed on {self.relative_path(file)}: {message.strip()}")
            elif message:
                needs_format.append(self.relative_path(file))
                output.append(message)
                if self.show_diff:
                    print(message, end="")

        # log_error writes to stderr; keep it after the buffered stdout diffs.
        sys.stdout.flush()
        if needs_format:
            log_error("qmlformat would change:")
            for name in needs_format:
                print(f"  {name}")
        elif not failed:
            log_ok("All QML files match qmlformat")

        return AnalysisResult(
            tool=self.name,
            passed=not (needs_format or failed),
            error_findings=bool(failed),
            issues=len(needs_format) + len(failed),
            output="".join(output),
            files_checked=len(files),
            files_with_issues=sorted(needs_format + failed),
        )

    def _check(self, file: Path) -> tuple[bool, str]:
        """Return (True, diff), or (False, error) when qmlformat fails."""
        result = run_captured(["qmlformat", str(file)])
        if result.returncode != 0:
            return False, result.stderr or result.stdout or f"exit code {result.returncode}"
        # Bytes keep CRLF line endings, which .qmlformat.ini's NewlineType=unix rewrites.
        original = file.read_bytes().decode("utf-8", errors="replace")
        name = self.relative_path(file)
        diff = difflib.unified_diff(
            original.splitlines(keepends=True),
            result.stdout.splitlines(keepends=True),
            fromfile=name,
            tofile=f"{name} (qmlformat)",
        )
        return True, "".join(diff)


def main(argv: list[str] | None = None) -> int:
    """Pre-commit entry point."""
    parser = argparse.ArgumentParser(
        description="Report QML files that qmlformat would change, without modifying them."
    )
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args(argv)
    analyzer = QmlFormatAnalyzer(Path.cwd(), Path.cwd())
    analyzer.show_diff = True
    return 0 if analyzer.run(args.files).passed else 1


if __name__ == "__main__":
    sys.exit(main())
