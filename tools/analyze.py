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
import shutil
import subprocess
import sys
import tempfile
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from pathlib import Path
from typing import ClassVar

from common import find_repo_root, Logger, log_info, log_ok, log_warn, log_error


@dataclass
class AnalysisResult:
    """Result from running an analyzer."""

    tool: str
    passed: bool
    issues: int = 0
    output: str = ""
    files_checked: int = 0
    files_with_issues: list[str] = field(default_factory=list)




class FileCollector:
    """Collect files to analyze based on mode."""

    CPP_EXTENSIONS: ClassVar[tuple[str, ...]] = (".cc", ".cpp", ".h", ".hpp")
    QML_EXTENSIONS: ClassVar[tuple[str, ...]] = (".qml",)

    def __init__(self, repo_root: Path) -> None:
        self.repo_root = repo_root

    def can_compare_master(self) -> bool:
        """Check if we can compare against master branch."""
        for ref in ("master", "origin/master"):
            result = subprocess.run(
                ["git", "-C", str(self.repo_root), "rev-parse", "--verify", ref],
                capture_output=True,
            )
            if result.returncode == 0:
                return True
        return False

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

        if self.can_compare_master():
            return self._get_changed_files(extensions)

        log_warn("master branch not available, analyzing all files")
        return self._find_files(self.repo_root / "src", extensions)

    def _find_files(self, search_path: Path, extensions: tuple[str, ...]) -> list[Path]:
        """Find all files with given extensions under search_path."""
        if not search_path.exists():
            return []

        files: list[Path] = []
        for ext in extensions:
            files.extend(search_path.rglob(f"*{ext}"))
        return sorted(files)

    def _get_changed_files(self, extensions: tuple[str, ...]) -> list[Path]:
        """Get files changed compared to master."""
        patterns = [f"*{ext}" for ext in extensions]
        result = subprocess.run(
            ["git", "-C", str(self.repo_root), "diff", "--name-only", "master..."]
            + ["--"]
            + patterns,
            capture_output=True,
            text=True,
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


class AnalyzerBase(ABC):
    """Base class for all code analyzers."""

    name: ClassVar[str] = "base"
    install_hint: ClassVar[str] = ""

    def __init__(
        self,
        repo_root: Path,
        build_dir: Path,
    ) -> None:
        self.repo_root = repo_root
        self.build_dir = build_dir
        self.compile_commands = build_dir / "compile_commands.json"

    @abstractmethod
    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        """Run the analyzer on the given files."""

    def check_tool(self, tool_name: str) -> bool:
        """Check if the tool is available."""
        return shutil.which(tool_name) is not None

    def require_tool(self, tool_name: str) -> bool:
        """Require a tool, logging error if not found."""
        if not self.check_tool(tool_name):
            log_error(f"{tool_name} not found. {self.install_hint}")
            return False
        return True

    def require_compile_commands(self) -> bool:
        """Require compile_commands.json, logging error if not found."""
        if not self.compile_commands.exists():
            log_error(
                f"compile_commands.json not found at {self.compile_commands}"
            )
            log_info("Run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
            return False
        return True

    def relative_path(self, path: Path) -> str:
        """Get path relative to repo root."""
        try:
            return str(path.relative_to(self.repo_root))
        except ValueError:
            return str(path)


class ClangFormatAnalyzer(AnalyzerBase):
    """Clang-format code formatter."""

    name: ClassVar[str] = "clang-format"
    install_hint: ClassVar[str] = "Install with: sudo apt install clang-format"

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_tool("clang-format"):
            return AnalysisResult(tool=self.name, passed=False, output="Tool not found")

        version_result = subprocess.run(
            ["clang-format", "--version"],
            capture_output=True,
            text=True,
        )
        version_match = version_result.stdout.split()[2] if version_result.stdout else "unknown"
        log_info(f"Using clang-format version {version_match}")

        if not files:
            log_info("No files to check")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"{'Formatting' if fix else 'Checking'} {len(files)} files...")

        if fix:
            return self._run_fix(files)
        return self._run_check(files)

    def _run_fix(self, files: list[Path]) -> AnalysisResult:
        formatted = 0
        for file in files:
            result = subprocess.run(
                ["clang-format", "-i", str(file)],
                capture_output=True,
            )
            if result.returncode == 0:
                formatted += 1

        log_ok(f"Formatted {formatted} files")

        diff_result = subprocess.run(
            ["git", "-C", str(self.repo_root), "diff", "--quiet"],
            capture_output=True,
        )

        if diff_result.returncode == 0:
            log_info("No formatting changes needed")
        else:
            log_info("Files modified:")
            modified = subprocess.run(
                ["git", "-C", str(self.repo_root), "diff", "--name-only"],
                capture_output=True,
                text=True,
            )
            print(modified.stdout)

        return AnalysisResult(
            tool=self.name,
            passed=True,
            files_checked=len(files),
        )

    def _run_check(self, files: list[Path]) -> AnalysisResult:
        needs_format: list[str] = []

        for file in files:
            result = subprocess.run(
                ["clang-format", "--dry-run", "--Werror", str(file)],
                capture_output=True,
            )
            if result.returncode != 0:
                needs_format.append(self.relative_path(file))

        if needs_format:
            log_error("The following files need formatting:")
            for f in needs_format:
                print(f"  {f}")
            print()
            log_info("Run: ./tools/analyze.py --tool clang-format --fix")
            return AnalysisResult(
                tool=self.name,
                passed=False,
                issues=len(needs_format),
                files_checked=len(files),
                files_with_issues=needs_format,
            )

        log_ok("All files properly formatted")
        return AnalysisResult(
            tool=self.name,
            passed=True,
            files_checked=len(files),
        )


class ClangTidyAnalyzer(AnalyzerBase):
    """Clang-tidy static analyzer."""

    name: ClassVar[str] = "clang-tidy"
    install_hint: ClassVar[str] = "Install with: sudo apt install clang-tidy"

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_compile_commands():
            return AnalysisResult(
                tool=self.name,
                passed=False,
                output="compile_commands.json not found",
            )

        if not self.require_tool("clang-tidy"):
            return AnalysisResult(tool=self.name, passed=False, output="Tool not found")

        if not files:
            log_info("No files to analyze")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running clang-tidy on {len(files)} files...")

        issues_found = False
        files_with_issues: list[str] = []

        for file in files:
            rel_path = self.relative_path(file)
            print(f"  Analyzing: {rel_path}... ", end="", flush=True)

            result = subprocess.run(
                ["clang-tidy", "-p", str(self.build_dir), str(file)],
                capture_output=True,
            )

            if result.returncode == 0:
                print("OK")
            else:
                print("ISSUES")
                issues_found = True
                files_with_issues.append(rel_path)

        return AnalysisResult(
            tool=self.name,
            passed=not issues_found,
            issues=len(files_with_issues),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )


class CppcheckAnalyzer(AnalyzerBase):
    """Cppcheck static analyzer."""

    name: ClassVar[str] = "cppcheck"
    install_hint: ClassVar[str] = "Install with: sudo apt install cppcheck"

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.require_tool("cppcheck"):
            return AnalysisResult(tool=self.name, passed=False, output="Tool not found")

        if not files:
            log_info("No files to analyze")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running cppcheck on {len(files)} files...")

        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
            for file in files:
                f.write(f"{file}\n")
            filelist_path = f.name

        try:
            result = subprocess.run(
                [
                    "cppcheck",
                    "--enable=warning,style,performance,portability",
                    "--std=c++20",
                    "--suppress=missingIncludeSystem",
                    "--suppress=unmatchedSuppression",
                    "--inline-suppr",
                    f"--file-list={filelist_path}",
                    "--error-exitcode=1",
                ],
                capture_output=True,
                text=True,
            )

            output = result.stdout + result.stderr
            if output.strip():
                print(output)

            return AnalysisResult(
                tool=self.name,
                passed=result.returncode == 0,
                output=output,
                files_checked=len(files),
            )
        finally:
            Path(filelist_path).unlink(missing_ok=True)


class ClazyAnalyzer(AnalyzerBase):
    """Clazy Qt-specific static analyzer."""

    name: ClassVar[str] = "clazy"
    install_hint: ClassVar[str] = "Install with: sudo apt install clazy"

    CHECKS: ClassVar[str] = "level1,connect-non-signal,lambda-in-connect,overridden-signal"

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.compile_commands.exists():
            log_warn("compile_commands.json not found - skipping clazy")
            log_info("Run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
            return AnalysisResult(tool=self.name, passed=True, output="Skipped")

        if not self.check_tool("clazy-standalone"):
            log_warn("clazy-standalone not found - skipping")
            log_info(self.install_hint)
            return AnalysisResult(tool=self.name, passed=True, output="Skipped")

        if not files:
            log_info("No files to analyze")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running clazy on {len(files)} files...")

        issues_found = False
        files_with_issues: list[str] = []
        all_output: list[str] = []

        for file in files:
            rel_path = self.relative_path(file)
            result = subprocess.run(
                [
                    "clazy-standalone",
                    "-p",
                    str(self.build_dir),
                    f"--checks={self.CHECKS}",
                    str(file),
                ],
                capture_output=True,
                text=True,
            )

            if result.returncode != 0 and result.stderr.strip():
                log_warn(f"Issues in {rel_path}:")
                print(result.stderr)
                all_output.append(result.stderr)
                issues_found = True
                files_with_issues.append(rel_path)

        if issues_found:
            log_warn("Clazy found Qt-specific issues")

        return AnalysisResult(
            tool=self.name,
            passed=not issues_found,
            issues=len(files_with_issues),
            output="\n".join(all_output),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )


class QmlLintAnalyzer(AnalyzerBase):
    """QML file linter."""

    name: ClassVar[str] = "qmllint"
    install_hint: ClassVar[str] = (
        "Install with: Qt SDK or 'sudo apt install qt6-declarative-dev-tools'"
    )

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not self.check_tool("qmllint"):
            log_warn("qmllint not found - skipping")
            log_info(self.install_hint)
            return AnalysisResult(tool=self.name, passed=True, output="Skipped")

        if not files:
            log_info("No QML files to lint")
            return AnalysisResult(tool=self.name, passed=True)

        log_info(f"Running qmllint on {len(files)} files...")

        issues_found = False
        files_with_issues: list[str] = []
        all_output: list[str] = []

        for file in files:
            result = subprocess.run(
                ["qmllint", str(file)],
                capture_output=True,
                text=True,
            )

            output = result.stdout + result.stderr
            if output.strip():
                print(output)
                all_output.append(output)

            if result.returncode != 0:
                issues_found = True
                files_with_issues.append(self.relative_path(file))

        if issues_found:
            log_warn("QML lint issues found")

        return AnalysisResult(
            tool=self.name,
            passed=not issues_found,
            issues=len(files_with_issues),
            output="\n".join(all_output),
            files_checked=len(files),
            files_with_issues=files_with_issues,
        )


def validate_path(path_str: str, repo_root: Path) -> Path:
    """Validate and sanitize a user-provided path."""
    if ".." in path_str:
        raise ValueError(f"Invalid path: {path_str} (must not contain '..')")

    if path_str.startswith("/"):
        raise ValueError(f"Invalid path: {path_str} (must be relative)")

    target = Path(path_str)
    full_path = repo_root / target

    if full_path.exists():
        resolved = full_path.resolve()
        repo_resolved = repo_root.resolve()
        if not str(resolved).startswith(str(repo_resolved)):
            raise ValueError("Invalid path: resolves outside repository")

    return target


def get_analyzer(
    tool: str,
    repo_root: Path,
    build_dir: Path,
) -> AnalyzerBase:
    """Get the appropriate analyzer for the given tool."""
    analyzers: dict[str, type[AnalyzerBase]] = {
        "clang-format": ClangFormatAnalyzer,
        "clang-tidy": ClangTidyAnalyzer,
        "cppcheck": CppcheckAnalyzer,
        "clazy": ClazyAnalyzer,
        "qmllint": QmlLintAnalyzer,
    }

    if tool not in analyzers:
        available = ", ".join(analyzers.keys())
        raise ValueError(f"Unknown tool: {tool}. Available tools: {available}")

    return analyzers[tool](repo_root, build_dir)


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
        choices=["clang-format", "clang-tidy", "cppcheck", "clazy", "qmllint"],
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
        default=1,
        help="Number of parallel jobs (not yet implemented)",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    logger = Logger(color=not args.no_color)

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
        analyzer = get_analyzer(args.tool, repo_root, build_dir)
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
