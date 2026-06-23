#!/usr/bin/env python3
"""
Run code quality tools on QGroundControl source code.

Usage:
    ./tools/analyze.py                    # Analyze changed files (vs master)
    ./tools/analyze.py --all              # Analyze all source files
    ./tools/analyze.py src/Vehicle/       # Analyze specific directory
    ./tools/analyze.py --tool clang-tidy  # Use specific tool
    ./tools/analyze.py --tool clang-format --fix  # Apply formatting

Tools:
    clang-format - Code formatting (use --fix to apply changes)
    clang-tidy   - Clang static analyzer (requires compile_commands.json)
    cppcheck     - Cppcheck static analyzer
    clazy        - Qt-specific static analyzer (requires compile_commands.json)
    qmllint      - QML file linter
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import TYPE_CHECKING, ClassVar

from common import find_repo_root, get_default_branch_ref, log_error, log_ok, log_warn

if TYPE_CHECKING:
    from common.analyzer import AnalyzerBase
from common.git import run_git


class FileCollector:
    """Collect files to analyze based on mode."""

    CPP_EXTENSIONS: ClassVar[tuple[str, ...]] = (".cc", ".cpp", ".h", ".hpp")
    QML_EXTENSIONS: ClassVar[tuple[str, ...]] = (".qml",)

    def __init__(self, repo_root: Path) -> None:
        self.repo_root = repo_root

    def get_compare_ref(self) -> str | None:
        """Get the best available ref to compare against."""
        return get_default_branch_ref(self.repo_root)

    def get_cpp_files(
        self,
        target_path: Path | None = None,
        analyze_all: bool = False,
    ) -> list[Path]:
        """Get C++ files to analyze."""
        return self._get_files(self.CPP_EXTENSIONS, target_path, analyze_all)

    def get_qml_files(
        self,
        target_path: Path | None = None,
        analyze_all: bool = False,
    ) -> list[Path]:
        """Get QML files to analyze."""
        return self._get_files(self.QML_EXTENSIONS, target_path, analyze_all)

    def _get_files(
        self,
        extensions: tuple[str, ...],
        target_path: Path | None = None,
        analyze_all: bool = False,
    ) -> list[Path]:
        if target_path is not None:
            search_path = self.repo_root / target_path
            return self._find_files(search_path, extensions)

        if analyze_all:
            return self._find_files(self.repo_root / "src", extensions)

        compare_ref = self.get_compare_ref()
        if compare_ref:
            return self._get_changed_files(extensions, compare_ref)

        log_warn("Default branch not available, analyzing all files")
        return self._find_files(self.repo_root / "src", extensions)

    def _find_files(self, search_path: Path, extensions: tuple[str, ...]) -> list[Path]:
        """Find all files with given extensions under search_path."""
        if not search_path.exists():
            return []

        files: list[Path] = []
        for ext in extensions:
            files.extend(search_path.rglob(f"*{ext}"))
        return sorted(files)

    def _get_changed_files(self, extensions: tuple[str, ...], compare_ref: str) -> list[Path]:
        """Get files changed compared to an available upstream ref."""
        patterns = [f"*{ext}" for ext in extensions]
        result = run_git(
            "diff", "--name-only", f"{compare_ref}...", "--", *patterns, cwd=self.repo_root
        )

        if result.returncode != 0:
            return []

        files: list[Path] = []
        for line in result.stdout.strip().splitlines():
            if not line:
                continue
            full_path = self.repo_root / line
            if full_path.is_file():
                files.append(full_path)
        return sorted(files)


def validate_path(path_str: str, repo_root: Path) -> Path:
    """Validate and sanitize a user-provided path."""
    if ".." in path_str:
        raise ValueError(f"Invalid path: {path_str} (must not contain '..')")

    if path_str.startswith("/"):
        raise ValueError(f"Invalid path: {path_str} (must be relative)")

    target = Path(path_str)
    full_path = (repo_root / target).resolve()
    repo_resolved = repo_root.resolve()

    if not full_path.is_relative_to(repo_resolved):
        raise ValueError("Invalid path: resolves outside repository")

    return target


def get_analyzer(
    tool: str,
    repo_root: Path,
    build_dir: Path,
    jobs: int = 1,
) -> AnalyzerBase:
    """Get the appropriate analyzer for the given tool."""
    match tool:
        case "clang-tidy":
            from analyzers.clang_tidy import ClangTidyAnalyzer

            return ClangTidyAnalyzer(repo_root, build_dir, jobs=jobs)
        case "clazy":
            from analyzers.clazy import ClazyAnalyzer

            return ClazyAnalyzer(repo_root, build_dir, jobs=jobs)
        case "clang-format":
            from analyzers.clang_format import ClangFormatAnalyzer

            return ClangFormatAnalyzer(repo_root, build_dir)
        case "cppcheck":
            from analyzers.cppcheck import CppcheckAnalyzer

            return CppcheckAnalyzer(repo_root, build_dir)
        case "qmllint":
            from analyzers.qmllint import QmlLintAnalyzer

            return QmlLintAnalyzer(repo_root, build_dir)
        case "vehicle-null-check":
            from analyzers.vehicle_null_check import VehicleNullCheckAnalyzer

            return VehicleNullCheckAnalyzer(repo_root, build_dir)
        case "qt-translate-noop-check":
            from analyzers.qt_translate_noop_check import QtTranslateNoopAnalyzer

            return QtTranslateNoopAnalyzer(repo_root, build_dir)
        case _:
            available = "clang-format, clang-tidy, cppcheck, clazy, qmllint, vehicle-null-check, qt-translate-noop-check"
            raise ValueError(f"Unknown tool: {tool}. Available tools: {available}")


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Run code quality tools on QGroundControl source code",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Tools:
  clang-format  Code formatting (use --fix to apply changes)
  clang-tidy    Clang static analyzer (requires compile_commands.json)
  cppcheck      Cppcheck static analyzer
  clazy         Qt-specific static analyzer (requires compile_commands.json)
  qmllint       QML file linter
  vehicle-null-check       Detect unsafe activeVehicle()/getParameter() patterns
  qt-translate-noop-check  Detect always-wrong runtime use of QT_TRANSLATE_NOOP

Examples:
  %(prog)s                           Analyze changed files (vs master)
  %(prog)s --all                     Analyze all source files
  %(prog)s src/Vehicle/              Analyze specific directory
  %(prog)s --tool clang-tidy         Use specific tool
  %(prog)s --tool clang-format --fix Apply formatting
""",
    )

    parser.add_argument(
        "path",
        nargs="?",
        help="Path to analyze (relative to repo root)",
    )
    parser.add_argument(
        "-t",
        "--tool",
        default="clang-tidy",
        choices=[
            "clang-format",
            "clang-tidy",
            "cppcheck",
            "clazy",
            "qmllint",
            "vehicle-null-check",
            "qt-translate-noop-check",
        ],
        help="Analysis tool to use (default: clang-tidy)",
    )
    parser.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="Analyze all source files",
    )
    parser.add_argument(
        "-f",
        "--fix",
        action="store_true",
        help="Apply fixes (only for clang-format)",
    )
    parser.add_argument(
        "-b",
        "--build-dir",
        default="build",
        help="Build directory containing compile_commands.json (default: build)",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=0,
        metavar="N",
        help="Parallel jobs for clang-tidy/clazy (default: cpu count, 0=auto)",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output",
    )

    parser.add_argument(
        "--check-deps",
        action="store_true",
        help="Check that required external tools are available, then exit",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    if args.check_deps:
        from common.deps import check_and_report

        tool_map = {
            "clang-format": ["clang-format"],
            "clang-tidy": ["clang-tidy"],
            "cppcheck": ["cppcheck"],
            "clazy": ["clazy-standalone"],
            "qmllint": ["qmllint"],
        }
        check_and_report(tool_map.get(args.tool, [args.tool]))
        return 0

    try:
        repo_root = find_repo_root()
    except RuntimeError as e:
        log_error(str(e))
        return 1

    build_dir = repo_root / args.build_dir

    target_path: Path | None = None
    if args.path:
        try:
            target_path = validate_path(args.path, repo_root)
        except ValueError as e:
            log_error(str(e))
            return 1

    try:
        jobs = args.jobs if args.jobs > 0 else os.cpu_count() or 1
        analyzer = get_analyzer(args.tool, repo_root, build_dir, jobs=jobs)
    except ValueError as e:
        log_error(str(e))
        return 1

    collector = FileCollector(repo_root)

    if args.tool == "qmllint":
        files = collector.get_qml_files(target_path, args.all)
    else:
        files = collector.get_cpp_files(target_path, args.all)

    result = analyzer.run(files, fix=args.fix)

    if result.passed:
        log_ok("Analysis complete")
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
