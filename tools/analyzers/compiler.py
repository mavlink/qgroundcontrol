"""Shared execution and diagnostics for compilation-database analyzers."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import ClassVar, TypedDict

from common.analyzer import AnalysisResult, AnalyzerBase
from common.proc import run_captured

from .dependencies import header_dependents


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


class FileTiming(TypedDict):
    file: str
    seconds: float
    status: str


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

    def _translation_units(self, files: list[Path]) -> list[Path]:
        """Use active compile commands; headers require a conservative project scan."""
        database = json.loads(self.compile_commands.read_text(encoding="utf-8"))
        if not isinstance(database, list):
            raise ValueError("Compilation database must contain a list")
        selected = {file.resolve() for file in files}
        sources = {".c", ".cc", ".cpp", ".cxx"}
        headers_changed = any(file.suffix not in sources for file in selected)
        project_dirs = [(self.repo_root / name).resolve() for name in ("src", "test")]
        units: set[Path] = set()
        active_entries = []
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
        if headers_changed:
            if not units <= selected:
                print(
                    f"Scanning header dependencies for {len(units)} compilation units", flush=True
                )
                affected = header_dependents(
                    active_entries,
                    {file for file in selected if file.suffix not in sources},
                    self.jobs,
                )
                if affected is not None:
                    units &= selected | affected
            print(
                f"Header selection: analyzing {len(units)} active project translation units",
                flush=True,
            )
        return sorted(units)

    def _analyze_file(self, file: Path) -> tuple[str, str, bool, bool, bool]:
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
            return self.relative_path(file), str(exc), False, True, False
        output = result.stdout + result.stderr
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
        error = compiler_error or (result.returncode != 0 and not findings)
        error_findings = bool(re.search(r"\berror:.*\[[^\]]+\]", output))
        return self.relative_path(file), output, findings, error, error_findings

    def _tool_arguments(self) -> tuple[str, ...]:
        return self.arguments

    def _file_arguments(self, file: Path) -> tuple[str, ...]:
        return ()

    def _check_timings(self, files: list[Path]) -> dict[str, float]:
        return {}

    def _timed_analysis(self, file: Path) -> tuple[tuple[str, str, bool, bool, bool], float]:
        start = time.perf_counter()
        result = self._analyze_file(file)
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

        outputs: list[str] = []
        diagnostics = DiagnosticDeduplicator()
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
            futures = [pool.submit(self._timed_analysis, file) for file in files]
            for future in as_completed(futures):
                (name, output, findings, error, fatal_findings), seconds = future.result()
                status = "error" if error else "findings" if findings else "passed"
                timings.append({"file": name, "seconds": round(seconds, 3), "status": status})
                progress.write(json.dumps(timings[-1]) + "\n")
                progress.flush()
                print(
                    f"[{len(timings)}/{len(files)}] {name}: {status} ({seconds:.2f}s)", flush=True
                )
                if output:
                    raw_log.write(f"=== {name} ===\n{output}\n")
                    raw_log.flush()
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
        (self.build_dir / f"{self.name}-timings.json").write_text(
            json.dumps(
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
                    "files": timings,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        print(
            f"Suppressed {diagnostics.suppressed} repeated diagnostic blocks; raw output: {raw_path}",
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
