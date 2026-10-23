#!/usr/bin/env python3
"""
Run code quality tools on QGroundControl source code.

Usage:
    ./tools/analyze.py                    # Analyze changed files (vs master)
    ./tools/analyze.py --all              # Analyze all source files
    ./tools/analyze.py src/Vehicle/       # Analyze specific directory
    ./tools/analyze.py --tool clang-tidy  # Use specific tool
    ./tools/analyze.py --tool clang-format --fix  # Apply formatting
    ./tools/analyze.py --tool clang-tidy --fix    # Apply clang-tidy fixes (local only)

Run with --help for the list of tools.
"""

from __future__ import annotations

import argparse
import importlib
import os
from pathlib import Path
from typing import TYPE_CHECKING, ClassVar, NamedTuple

import _bootstrap as _bootstrap
from common.file_traversal import find_repo_root
from common.gh_actions import is_ci
from common.git import pr_diff_refs, run_git
from common.logging import log_error, log_ok

if TYPE_CHECKING:
    from analyzers.base import AnalyzerBase


class Tool(NamedTuple):
    module: str
    class_name: str
    help: str
    compiler: bool = False


ANALYZERS: dict[str, Tool] = {
    "clang-format": Tool(
        "analyzers.clang_format",
        "ClangFormatAnalyzer",
        "Changed-line formatting; whole files with --all or paths (--fix applies)",
    ),
    "clang-tidy": Tool(
        "analyzers.clang_tidy",
        "ClangTidyAnalyzer",
        "Clang static analyzer (requires compile_commands.json; --fix applies, local only)",
        compiler=True,
    ),
    "clazy": Tool(
        "analyzers.clazy",
        "ClazyAnalyzer",
        "Qt-specific static analyzer (requires compile_commands.json)",
        compiler=True,
    ),
    "qmllint": Tool("analyzers.qmllint", "QmlLintAnalyzer", "QML file linter"),
    "qmlformat": Tool(
        "analyzers.qmlformat",
        "QmlFormatAnalyzer",
        "Report QML files that qmlformat would change (check only)",
    ),
    "vehicle-null-check": Tool(
        "analyzers.vehicle_null_check",
        "VehicleNullCheckAnalyzer",
        "Detect unsafe activeVehicle() patterns",
    ),
    "qt-translate-noop-check": Tool(
        "analyzers.qt_translate_noop_check",
        "QtTranslateNoopAnalyzer",
        "Detect always-wrong runtime use of QT_TRANSLATE_NOOP",
    ),
}
COMPILER_TOOLS = frozenset(name for name, tool in ANALYZERS.items() if tool.compiler)


class FileCollector:
    """Collect files to analyze based on mode."""

    CPP_EXTENSIONS: ClassVar[tuple[str, ...]] = (".cc", ".cpp", ".h", ".hpp")
    QML_EXTENSIONS: ClassVar[tuple[str, ...]] = (".qml",)

    def __init__(self, repo_root: Path) -> None:
        self.repo_root = repo_root
        # Merge base of the changed-file selection; None when whole files were selected.
        self.diff_base: str | None = None

    def get_compare_ref(self) -> str | None:
        """Return the ref to select changed files against, or None to select every file."""
        refs = pr_diff_refs(
            os.environ.get("PR_BASE_SHA"), os.environ.get("PR_HEAD_SHA"), cwd=self.repo_root
        )
        return refs[0] if refs else None

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
        return self._find_files(self.repo_root / "src", extensions)

    def _find_files(self, search_path: Path, extensions: tuple[str, ...]) -> list[Path]:
        """Find all files with given extensions under search_path."""
        if not search_path.exists():
            return []

        if search_path.is_file():
            return [search_path] if search_path.suffix in extensions else []

        files: list[Path] = []
        for ext in extensions:
            files.extend(search_path.rglob(f"*{ext}"))
        return sorted(files)

    def _get_changed_files(self, extensions: tuple[str, ...], compare_ref: str) -> list[Path]:
        """Get committed, uncommitted, and untracked files changed since the merge base."""
        patterns = [f"*{ext}" for ext in extensions]
        merge_base = run_git("merge-base", compare_ref, "HEAD", cwd=self.repo_root)
        if merge_base.returncode != 0:
            raise RuntimeError(f"Unable to determine changed files: {merge_base.stderr}")
        self.diff_base = merge_base.stdout.strip()

        names: set[str] = set()
        for git_args in (
            ("diff", "--name-only", self.diff_base),
            ("ls-files", "--others", "--exclude-standard"),
        ):
            result = run_git(*git_args, "--", *patterns, cwd=self.repo_root)
            if result.returncode != 0:
                raise RuntimeError(f"Unable to determine changed files: {result.stderr}")
            names.update(line for line in result.stdout.splitlines() if line)

        return sorted(full_path for name in names if (full_path := self.repo_root / name).is_file())


def validate_path(path_str: str, repo_root: Path) -> Path:
    """Return a repository-relative or absolute path relative to the root; reject outside paths."""
    root = repo_root.resolve()
    full_path = (root / path_str).resolve()
    if not full_path.is_relative_to(root):
        raise ValueError(f"Invalid path: {path_str} resolves outside the repository")
    return full_path.relative_to(root)


def get_analyzer(
    tool: str,
    repo_root: Path,
    build_dir: Path,
    jobs: int = 1,
    *,
    shard: int = 1,
    shard_count: int = 1,
) -> AnalyzerBase:
    """Get the appropriate analyzer for the given tool."""
    entry = ANALYZERS.get(tool)
    if entry is None:
        raise ValueError(f"Unknown tool: {tool}. Available tools: {', '.join(ANALYZERS)}")
    analyzer_cls = getattr(importlib.import_module(entry.module), entry.class_name)
    if entry.compiler:
        return analyzer_cls(repo_root, build_dir, jobs=jobs, shard=shard, shard_count=shard_count)
    return analyzer_cls(repo_root, build_dir)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments."""
    tools = "\n".join(f"  {name:<24} {tool.help}" for name, tool in ANALYZERS.items())
    parser = argparse.ArgumentParser(
        description="Run code quality tools on QGroundControl source code",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
Tools:
{tools}

Examples:
  %(prog)s                           Analyze changed files (vs master)
  %(prog)s --all                     Analyze all source files
  %(prog)s src/Vehicle/              Analyze specific directory
  %(prog)s --tool clang-tidy         Use specific tool
  %(prog)s --tool clang-format --fix Apply formatting
  %(prog)s --tool clang-tidy --fix   Apply clang-tidy fixes (never in CI)
""",
    )

    parser.add_argument(
        "path",
        nargs="*",
        help="Files or directories to analyze (relative to the repository root, or absolute)",
    )
    parser.add_argument(
        "-t",
        "--tool",
        dest="tools",
        action="append",
        choices=ANALYZERS,
        help="Analysis tool to use (default: clang-tidy); repeat to run clang-tidy and clazy "
        "over one selection and dependency scan",
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
        help="Apply fixes: clang-format formats; clang-tidy applies its exported fixes with "
        "clang-apply-replacements, bypassing --cache-dir (local only, refused in CI)",
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
        "--shard", type=int, default=1, help="Compiler-analysis shard number (1-based)"
    )
    parser.add_argument(
        "--shard-count", type=int, default=1, help="Number of compiler-analysis shards"
    )
    parser.add_argument(
        "--profile-checks",
        action="store_true",
        help="Record clang-tidy per-check profiles (adds significant runtime overhead)",
    )
    parser.add_argument(
        "--cache-dir",
        type=Path,
        help="Replay clang-tidy/clazy results whose tool, configuration, compile command and "
        "preprocessed inputs are unchanged, storing new completed results here",
    )

    parser.add_argument(
        "--qml-build", action="store_true", help="Use generated QML imports and enforce type errors"
    )

    parser.add_argument(
        "--advisory",
        action="store_true",
        help="Report warnings without failing; errors still fail",
    )

    args = parser.parse_args(argv)
    args.tools = list(dict.fromkeys(args.tools or ["clang-tidy"]))
    args.tool = args.tools[0]
    compiler_tools = set(args.tools) <= COMPILER_TOOLS
    if len(args.tools) > 1 and not compiler_tools:
        parser.error("Only clang-tidy and clazy can run together")
    if not 1 <= args.shard <= args.shard_count:
        parser.error("--shard must be between 1 and --shard-count")
    if args.shard_count > 1 and not compiler_tools:
        parser.error("Sharding requires --tool clang-tidy or clazy")
    if args.profile_checks and "clang-tidy" not in args.tools:
        parser.error("--profile-checks requires --tool clang-tidy")
    if args.cache_dir is not None and not compiler_tools:
        parser.error("--cache-dir requires --tool clang-tidy or clazy")
    if args.fix and not set(args.tools) <= {"clang-format", "clang-tidy"}:
        parser.error("--fix requires --tool clang-format or clang-tidy")
    if args.fix and "clang-tidy" in args.tools and is_ci():
        parser.error("clang-tidy --fix rewrites sources and runs only locally, never in CI")
    return args


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    args = parse_args(argv)

    try:
        repo_root = find_repo_root()
    except RuntimeError as e:
        log_error(str(e))
        return 1

    build_dir = repo_root / args.build_dir

    try:
        targets = [validate_path(path, repo_root) for path in args.path]
    except ValueError as e:
        log_error(str(e))
        return 1

    try:
        jobs = args.jobs if args.jobs > 0 else os.cpu_count() or 1
        analyzers = [
            get_analyzer(
                tool,
                repo_root,
                build_dir,
                jobs=jobs,
                shard=args.shard,
                shard_count=args.shard_count,
            )
            for tool in args.tools
        ]
    except ValueError as e:
        log_error(str(e))
        return 1
    analyzer = analyzers[0]

    if args.tool in COMPILER_TOOLS:
        from analyzers.clang_tidy import ClangTidyAnalyzer
        from analyzers.compiler import CompilerAnalyzer
        from analyzers.result_cache import ResultCache

        dependencies: dict[Path, frozenset[Path]] = {}
        cache = ResultCache(repo_root / args.cache_dir) if args.cache_dir else None
        for compiler in analyzers:
            if isinstance(compiler, CompilerAnalyzer):
                compiler.dependencies = dependencies
                compiler.result_cache = cache
            if args.profile_checks and isinstance(compiler, ClangTidyAnalyzer):
                compiler.profile_checks = True

    if args.qml_build:
        from analyzers.qmllint import QmlLintAnalyzer

        if not isinstance(analyzer, QmlLintAnalyzer):
            log_error("--qml-build requires --tool qmllint")
            return 2
        analyzer.build_aware = True

    collector = FileCollector(repo_root)

    qml_tool = args.tool in {"qmllint", "qmlformat"}
    collect = collector.get_qml_files if qml_tool else collector.get_cpp_files
    try:
        files = (
            sorted({file for target in targets for file in collect(target)})
            if targets
            else collect(analyze_all=args.all)
        )
    except RuntimeError as e:
        log_error(str(e))
        return 2

    if args.tool == "clang-format":
        from analyzers.clang_format import ClangFormatAnalyzer

        if isinstance(analyzer, ClangFormatAnalyzer):
            analyzer.diff_base = collector.diff_base

    status = 0
    for selected in analyzers:
        result = selected.run(files, fix=args.fix)
        reason = f": {result.output}" if result.execution_error and result.output else ""
        print(f"{result.tool}: {result.status} ({result.files_checked} files){reason}")
        if result.execution_error or result.error_findings:
            status = 2
        elif not (result.passed or args.advisory):
            status = max(status, 1)
    if status == 0:
        log_ok("Analysis complete")
    return status


if __name__ == "__main__":
    raise SystemExit(main())
