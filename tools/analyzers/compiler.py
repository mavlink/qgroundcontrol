"""Shared execution and diagnostics for compilation-database analyzers."""

from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Any, ClassVar, NamedTuple, TypedDict

from common.git import changed_ranges, diff_base, pr_diff_refs, run_git
from common.io import read_json, write_json
from common.proc import require_tools, run_captured

from analyzers.base import AnalysisResult, AnalyzerBase

from .dependencies import DependencyScanError, scan_dependencies
from .result_cache import CachedResult, ResultCache


class DiagnosticDeduplicator:
    """Suppress identical diagnostic blocks, retaining notes and unfamiliar output."""

    def __init__(self) -> None:
        self.seen: set[bytes] = set()
        self.suppressed = 0

    def filter(self, output: str) -> str:
        blocks = re.split(
            r"(?=^[^\n]+:\d+:\d+: (?:warning|error|fatal error): )", output, flags=re.MULTILINE
        )
        kept = [blocks[0]]
        for block in blocks[1:]:
            digest = hashlib.sha256(block.encode()).digest()
            if digest in self.seen:
                self.suppressed += 1
            else:
                self.seen.add(digest)
                kept.append(block)
        return "".join(kept)


# A header's diagnostics repeat in every translation unit that includes it.
_DIAGNOSTIC = re.compile(
    r"^(?P<path>.+?):(?P<line>\d+):(?P<column>\d+): (?P<severity>warning|error): "
    r".*?(?: \[(?P<check>[^\]\n]+)\])?$",
    re.MULTILINE,
)

Diagnostic = tuple[str, int, int, str, str]
"""One diagnostic location: file, line, column, severity, and check name."""


class FileTiming(TypedDict):
    file: str
    seconds: float
    status: str
    cached: bool


class FileResult(NamedTuple):
    file: str
    output: str
    findings: bool
    error: bool
    error_findings: bool
    cached: bool = False


class CompilerAnalyzer(AnalyzerBase):
    executable: ClassVar[str]
    arguments: ClassVar[tuple[str, ...]] = ()

    def __init__(
        self,
        repo_root: Path,
        build_dir: Path,
        jobs: int = 1,
        *,
        shard: int = 1,
        shard_count: int = 1,
    ) -> None:
        super().__init__(repo_root, build_dir)
        if not 1 <= shard <= shard_count:
            raise ValueError("Shard must be between 1 and shard count")
        self.jobs = jobs
        self.shard = shard
        self.shard_count = shard_count
        self.result_cache: ResultCache | None = None
        # Analyzers sharing one compilation database can share a single dependency scan.
        self.dependencies: dict[Path, frozenset[Path]] = {}
        self._entries: dict[Path, list[dict[str, Any]]] = {}

    def _translation_units(self, files: list[Path]) -> list[Path]:
        """Use active compile commands; headers require a conservative project scan."""
        database = read_json(self.compile_commands)
        if not isinstance(database, list):
            raise ValueError("Compilation database must contain a list")
        selected = {file.resolve() for file in files}
        sources = {".c", ".cc", ".cpp", ".cxx"}
        headers_changed = any(file.suffix not in sources for file in selected)
        project_dirs = [(self.repo_root / name).resolve() for name in ("src", "test")]
        units: set[Path] = set()
        active_entries = []
        self._entries = {}
        for entry in database:
            if (
                not isinstance(entry, dict)
                or not isinstance(entry.get("file"), str)
                or not isinstance(entry.get("directory"), str)
            ):
                raise ValueError("Compilation database entry requires file and directory strings")
            source = (Path(entry["directory"]) / entry["file"]).resolve()
            if source.suffix not in sources:
                continue
            if source in selected or (
                headers_changed and any(source.is_relative_to(path) for path in project_dirs)
            ):
                units.add(source)
                active_entries.append(entry)
                self._entries.setdefault(source, []).append(entry)
        if headers_changed:
            if not units <= selected:
                print(
                    f"Scanning header dependencies for {len(units)} compilation units", flush=True
                )
                changed_headers = {file for file in selected if file.suffix not in sources}
                try:
                    dependencies = self._scan_dependencies(active_entries)
                except DependencyScanError as exc:
                    print(f"{exc}; checking all active project sources", flush=True)
                else:
                    units &= selected | {
                        source for source, paths in dependencies.items() if paths & changed_headers
                    }
            print(
                f"Header selection: analyzing {len(units)} active project translation units",
                flush=True,
            )
        return sorted(units)

    def _scan_dependencies(self, entries: list[dict[str, Any]]) -> dict[Path, frozenset[Path]]:
        """Scan entries whose sources lack a dependency list, reusing earlier scans."""
        pending = [
            entry
            for entry in entries
            if (Path(entry["directory"]) / entry["file"]).resolve() not in self.dependencies
        ]
        if pending:
            self.dependencies.update(scan_dependencies(pending, self.jobs))
        return {
            source: self.dependencies[source]
            for source in {
                (Path(entry["directory"]) / entry["file"]).resolve() for entry in entries
            }
        }

    def _cache_identity(self) -> tuple[str, ...]:
        """Identify the executable, its arguments and any configuration it reads."""
        executable = require_tools(self.executable)[self.executable]
        version = run_captured([executable, "--version"], timeout=60)
        if version.returncode:
            raise OSError(f"{self.executable} --version failed")
        binary = hashlib.sha256(Path(executable).resolve().read_bytes()).hexdigest()
        return (self.name, version.stdout, binary, *self._tool_arguments(), *self._config_inputs())

    def _config_inputs(self) -> tuple[str, ...]:
        return ()

    def _cacheable(self) -> bool:
        return True

    def _cache_keys(self, files: list[Path]) -> dict[Path, str]:
        cache = self.result_cache
        if cache is None or not self._cacheable():
            return {}
        try:
            dependencies = self._scan_dependencies(
                [entry for file in files for entry in self._entries.get(file, [])]
            )
            identity = self._cache_identity()
        except (DependencyScanError, OSError, subprocess.TimeoutExpired) as exc:
            print(f"Result cache disabled: {exc}", flush=True)
            return {}
        keys = {}
        for file in files:
            key = cache.key(
                identity, self._entries.get(file, []), dependencies.get(file, set()) | {file}
            )
            if key is not None:
                keys[file] = key
        return keys

    def _classify(
        self, file: Path, output: str, returncode: int, cached: bool = False
    ) -> FileResult:
        findings = bool(re.search(r"\b(?:warning|error):", output))
        # A failed compiler invocation is an analysis failure, not a clean scan.
        compiler_error = bool(
            re.search(
                r"fatal error:|\[clang-diagnostic-error\]|^(?:[^\s:]+:\s*)?error:|"
                r":\d+:\d+: error: (?!.*\[[^]]+\]$)",
                output,
                re.MULTILINE,
            )
        )
        error = compiler_error or (returncode != 0 and not findings)
        error_findings = bool(re.search(r"\berror:.*\[[^\]]+\]", output))
        return FileResult(self.relative_path(file), output, findings, error, error_findings, cached)

    def _analyze_file(self, file: Path, key: str | None = None) -> FileResult:
        if self.result_cache is not None and key is not None:
            replay = self.result_cache.load(key)
            if replay is not None:
                output = replay.stdout + replay.stderr
                return self._classify(file, output, replay.returncode, cached=True)
        try:
            result = run_captured(
                [
                    self.executable,
                    "-p",
                    str(self.build_dir),
                    *self._tool_arguments(),
                    *self._file_arguments(file),
                    str(file),
                ],
                timeout=300,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            return FileResult(self.relative_path(file), str(exc), False, True, False)
        analysis = self._classify(file, result.stdout + result.stderr, result.returncode)
        if self.result_cache is not None and key is not None and not analysis.error:
            self.result_cache.store(
                key, CachedResult(result.stdout, result.stderr, result.returncode)
            )
        return analysis

    def _finish_cache(self, keys: dict[Path, str], timings: list[FileTiming]) -> dict[str, int]:
        if self.result_cache is None or not keys:
            return {}
        hits = sum(row["cached"] for row in timings)
        report = {"hits": hits, "misses": len(keys) - hits, "uncacheable": len(timings) - len(keys)}
        report["pruned"] = self.result_cache.prune()
        print(
            f"Result cache: {report['hits']} hits, {report['misses']} misses, "
            f"{report['uncacheable']} without a key ({self.result_cache.directory})",
            flush=True,
        )
        return report

    def _tool_arguments(self) -> tuple[str, ...]:
        return self.arguments

    def _diagnostic_file(self, path: str) -> str:
        """Name a diagnostic's file relative to the checkout, or to the build tree as build/..."""
        resolved = Path(os.path.normpath(path))
        for base, prefix in ((self.repo_root, ""), (self.build_dir, "build/")):
            try:
                return prefix + resolved.relative_to(base).as_posix()
            except ValueError:
                continue
        return resolved.as_posix()

    def _diagnostics(self, output: str) -> set[Diagnostic]:
        """Return the output's diagnostics, identified by file, location, severity, and check."""
        return {
            (
                self._diagnostic_file(match["path"]),
                int(match["line"]),
                int(match["column"]),
                match["severity"],
                match["check"] or "",
            )
            for match in _DIAGNOSTIC.finditer(output)
        }

    def _changed_lines(self, files: set[str]) -> dict[str, list[tuple[int, int]] | None] | None:
        """Map each of *files* that the pull request changed to its changed line ranges.

        A None range list means the whole file is new. Returns None outside a pull request
        (PR_HEAD_SHA unset) or without a usable diff base.
        """
        head = os.environ.get("PR_HEAD_SHA")
        if not head:
            return None
        refs = pr_diff_refs(os.environ.get("PR_BASE_SHA"), head, cwd=self.repo_root)
        if refs is None:
            return None
        try:
            base = diff_base(*refs, cwd=self.repo_root)
            if base is None:
                return None
            names = run_git(
                "diff", "--name-only", "--no-renames", base, cwd=self.repo_root, check=True
            ).stdout.splitlines()
            return {
                name: changed_ranges(name, base, cwd=self.repo_root)
                for name in sorted(files.intersection(names))
            }
        except subprocess.CalledProcessError as exc:
            print(f"Cannot diff {refs[0]}..{refs[1]}; changed lines are not reported: {exc}")
            return None

    def _analyzed_commit(self) -> str:
        """Return the checked-out commit whose line numbers the diagnostics use, or ''.

        On a pull request that is GitHub's merge commit (GITHUB_SHA), not the PR head.
        """
        if sha := os.environ.get("GITHUB_SHA", "").strip():
            return sha
        try:
            head = run_git("rev-parse", "--verify", "--quiet", "HEAD", cwd=self.repo_root)
        except OSError:
            return ""
        return head.stdout.strip()

    def _write_diagnostics(self, diagnostics: set[Diagnostic]) -> None:
        """Write each unique diagnostic once; reports merge shards by the same identity.

        ``commit`` names the analyzed checkout. In a pull request, ``changed`` repeats the
        diagnostics on the lines it changed.
        """
        by_file: dict[str, list[list[int | str]]] = {}
        for file, line, column, severity, check in sorted(diagnostics):
            by_file.setdefault(file, []).append([line, column, severity, check])
        report: dict[str, Any] = {"tool": self.name, "shard": self.shard, "diagnostics": by_file}
        if commit := self._analyzed_commit():
            report["commit"] = commit
        changed_lines = self._changed_lines(set(by_file))
        if changed_lines is not None:
            changed = {
                file: [
                    row
                    for row in by_file[file]
                    if ranges is None or any(start <= int(row[0]) <= end for start, end in ranges)
                ]
                for file, ranges in changed_lines.items()
            }
            report["changed"] = {file: rows for file, rows in changed.items() if rows}
        write_json(self.build_dir / f"{self.name}-diagnostics.json", report, indent=None)

    def _file_arguments(self, file: Path) -> tuple[str, ...]:
        return ()

    def _check_timings(self, files: list[Path]) -> dict[str, float]:
        return {}

    def _timed_analysis(self, file: Path, key: str | None = None) -> tuple[FileResult, float]:
        start = time.perf_counter()
        result = self._analyze_file(file, key)
        return result, time.perf_counter() - start

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not files:
            return AnalysisResult(tool=self.name, passed=True, skipped=True)
        if not self.require_compile_commands() or not self.require_tool(self.executable):
            return AnalysisResult(tool=self.name, passed=False, execution_error=True)
        selection_start = time.perf_counter()
        try:
            files = self._translation_units(files)
        except (OSError, ValueError) as exc:
            message = f"Unable to select compilation units: {exc}"
            print(message)
            return AnalysisResult(
                tool=self.name, passed=False, execution_error=True, output=message
            )
        total_files = len(files)
        # Partition after dependency expansion so changed headers keep every dependent.
        files = files[self.shard - 1 :: self.shard_count]
        if self.shard_count > 1:
            print(
                f"Shard {self.shard}/{self.shard_count}: {len(files)} of {total_files} files",
                flush=True,
            )
        if not files:
            print("No selected translation units are active in this build")
            return AnalysisResult(tool=self.name, passed=True, skipped=True)
        keys = self._cache_keys(files)

        outputs: list[str] = []
        diagnostics = DiagnosticDeduplicator()
        unique_diagnostics: set[Diagnostic] = set()
        raw_path = self.build_dir / f"{self.name}-raw.txt"
        affected: list[str] = []
        errors = False
        error_findings = False
        selection_seconds = time.perf_counter() - selection_start
        timings: list[FileTiming] = []
        analysis_start = time.perf_counter()
        print(f"Running {self.name} on {len(files)} files with {self.jobs} workers", flush=True)
        with (
            raw_path.open("w", encoding="utf-8") as raw_log,
            (self.build_dir / f"{self.name}-progress.jsonl").open(
                "w", encoding="utf-8"
            ) as progress,
            ThreadPoolExecutor(max_workers=self.jobs) as pool,
        ):
            futures = [pool.submit(self._timed_analysis, file, keys.get(file)) for file in files]
            for future in as_completed(futures):
                (name, output, findings, error, fatal_findings, cached), seconds = future.result()
                status = "error" if error else "findings" if findings else "passed"
                timings.append(
                    {"file": name, "seconds": round(seconds, 3), "status": status, "cached": cached}
                )
                progress.write(json.dumps(timings[-1]) + "\n")
                progress.flush()
                source = "cached" if cached else f"{seconds:.2f}s"
                print(f"[{len(timings)}/{len(files)}] {name}: {status} ({source})", flush=True)
                if output:
                    raw_log.write(f"=== {name} ===\n{output}\n")
                    raw_log.flush()
                    unique_diagnostics |= self._diagnostics(output)
                    unique_output = diagnostics.filter(output)
                    if unique_output:
                        print(unique_output, end="" if unique_output.endswith("\n") else "\n")
                        outputs.append(unique_output)
                if findings or error:
                    affected.append(name)
                errors |= error
                error_findings |= fatal_findings
        timings.sort(key=lambda row: row["seconds"], reverse=True)
        wall_seconds = time.perf_counter() - analysis_start
        check_timings = self._check_timings(files)
        cache_report = self._finish_cache(keys, timings)
        self._write_diagnostics(unique_diagnostics)
        write_json(
            self.build_dir / f"{self.name}-timings.json",
            {
                "tool": self.name,
                "selection_seconds": round(selection_seconds, 3),
                "wall_seconds": round(wall_seconds, 3),
                "worker_seconds": round(sum(row["seconds"] for row in timings), 3),
                "workers": self.jobs,
                "shard": self.shard,
                "shard_count": self.shard_count,
                "selected_files": total_files,
                "checks": check_timings,
                "result_cache": cache_report,
                "files": timings,
            },
        )
        print(
            f"Suppressed {diagnostics.suppressed} repeated diagnostic blocks "
            f"({len(unique_diagnostics)} unique diagnostics); raw output: {raw_path}",
            flush=True,
        )
        print("Slowest compilation units:", flush=True)
        for row in timings[:10]:
            print(f"  {row['seconds']:.2f}s {row['file']}", flush=True)
        print(f"Analysis wall time: {wall_seconds:.2f}s with {self.jobs} workers", flush=True)
        if check_timings:
            print(
                "Most expensive checks (summed across compilation units; durations overlap):",
                flush=True,
            )
            for name, seconds in sorted(
                check_timings.items(), key=lambda item: item[1], reverse=True
            )[:10]:
                print(f"  {seconds:.2f}s {name}", flush=True)
        return AnalysisResult(
            tool=self.name,
            passed=not affected,
            issues=len(affected),
            files_checked=len(files),
            files_with_issues=affected,
            output="\n".join(outputs),
            execution_error=errors,
            error_findings=error_findings,
        )
