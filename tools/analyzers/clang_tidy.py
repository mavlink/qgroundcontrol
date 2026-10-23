"""Clang-Tidy analysis using the configured compilation database."""

from __future__ import annotations

import hashlib
import re
import shutil
import subprocess
from pathlib import Path
from typing import ClassVar

from common.io import read_json
from common.logging import log_error, log_info, log_ok
from common.proc import run_captured

from .base import AnalysisResult
from .compiler import CompilerAnalyzer, FileResult

APPLY_REPLACEMENTS = "clang-apply-replacements"
# The .clang-tidy HeaderFilterRegex directories, anchored to the checkout when fixing: CPM sources
# and generated headers under other src/ directories also match the configured pattern.
FIX_HEADER_DIRS = ("src", "test", "tools")


def _file_identity(file: Path) -> str:
    return hashlib.sha256(str(file.resolve()).encode()).hexdigest()[:16]


class ClangTidyAnalyzer(CompilerAnalyzer):
    name: ClassVar[str] = "clang-tidy"
    executable: ClassVar[str] = "clang-tidy"
    install_hint: ClassVar[str] = "Install clang-tidy matching the analysis compiler."
    profile_checks: bool = False
    fixes_dir: Path | None = None

    def _tool_arguments(self) -> tuple[str, ...]:
        # Cached dependencies can live outside the checkout and inherit another config.
        arguments = (*super()._tool_arguments(), f"--config-file={self.repo_root / '.clang-tidy'}")
        if self.fixes_dir is None:
            return arguments
        root = re.escape(self.repo_root.resolve().as_posix())
        directories = "|".join(FIX_HEADER_DIRS)
        return (*arguments, f"--header-filter=^{root}/({directories})/.*\\.(h|hpp)$")

    def _config_inputs(self) -> tuple[str, ...]:
        return ((self.repo_root / ".clang-tidy").read_text(encoding="utf-8"),)

    def _cacheable(self) -> bool:
        # A replayed result would skip exporting that translation unit's fixes.
        return not self.profile_checks and self.fixes_dir is None

    def _file_arguments(self, file: Path) -> tuple[str, ...]:
        arguments: list[str] = []
        if self.profile_checks:
            profile_dir = self.build_dir / "clang-tidy-profiles" / _file_identity(file)
            profile_dir.mkdir(parents=True, exist_ok=True)
            for previous in profile_dir.glob("*.json"):
                previous.unlink()
            arguments += ["--enable-check-profile", f"--store-check-profile={profile_dir}/"]
        if self.fixes_dir is not None:
            self.fixes_dir.mkdir(parents=True, exist_ok=True)
            arguments.append(f"--export-fixes={self.fixes_dir / f'{_file_identity(file)}.yaml'}")
        return tuple(arguments)

    def _analyze_file(self, file: Path, key: str | None = None) -> FileResult:
        result = super()._analyze_file(file, key)
        if result.error and self.fixes_dir is not None:
            # clang-tidy exports fixes even when compilation failed; its own --fix refuses them.
            (self.fixes_dir / f"{_file_identity(file)}.yaml").unlink(missing_ok=True)
        return result

    def _apply_replacements_executable(self) -> str | None:
        """Prefer the clang-apply-replacements installed beside clang-tidy, then PATH."""
        tidy = shutil.which(self.executable)
        if tidy is not None:
            for directory in dict.fromkeys((Path(tidy).parent, Path(tidy).resolve().parent)):
                if found := shutil.which(APPLY_REPLACEMENTS, path=str(directory)):
                    return found
        return shutil.which(APPLY_REPLACEMENTS)

    def run(self, files: list[Path], fix: bool = False) -> AnalysisResult:
        if not fix or not files:
            return super().run(files)
        apply_replacements = self._apply_replacements_executable()
        if apply_replacements is None:
            log_error(
                f"{APPLY_REPLACEMENTS} not found next to clang-tidy or on PATH. It ships with "
                "LLVM's clang-tidy packages and the clang-tidy PyPI wheel."
            )
            return AnalysisResult(tool=self.name, passed=False, execution_error=True)
        if self.result_cache is not None:
            log_info("Result cache bypassed: --fix analyzes every translation unit")
        fixes_dir = self.build_dir / "clang-tidy-fixes"
        shutil.rmtree(fixes_dir, ignore_errors=True)
        self.fixes_dir = fixes_dir
        try:
            result = super().run(files)
        finally:
            self.fixes_dir = None
        exported = sorted(fixes_dir.glob("*.yaml"))
        if not exported:
            log_info("clang-tidy exported no fixes")
            return result
        # Translation units sharing a header export identical fixes; the tool deduplicates them.
        command = [apply_replacements, "--ignore-insert-conflict", str(fixes_dir)]
        try:
            applied = run_captured(command, timeout=600)
        except (OSError, subprocess.TimeoutExpired) as exc:
            log_error(f"{APPLY_REPLACEMENTS} failed: {exc}")
            result.execution_error = True
            return result
        output = applied.stdout + applied.stderr
        if output:
            print(output, end="" if output.endswith("\n") else "\n")
        if applied.returncode:
            log_error(f"{APPLY_REPLACEMENTS} exited {applied.returncode}; fixes are in {fixes_dir}")
            result.execution_error = True
        else:
            log_ok(
                f"Applied fixes exported from {len(exported)} translation units; review them with "
                "git diff and rerun without --fix for remaining findings"
            )
        return result

    def _check_timings(self, files: list[Path]) -> dict[str, float]:
        if not self.profile_checks:
            return {}
        totals: dict[str, float] = {}
        for file in files:
            identity = _file_identity(file)
            for path in (self.build_dir / "clang-tidy-profiles" / identity).glob("*.json"):
                try:
                    profile = read_json(path)["profile"]
                    for name, seconds in profile.items():
                        if name.startswith("time.clang-tidy.") and name.endswith(".wall"):
                            check = name.removeprefix("time.clang-tidy.").removesuffix(".wall")
                            totals[check] = totals.get(check, 0.0) + float(seconds)
                except (OSError, ValueError, KeyError, TypeError, AttributeError):
                    print(f"Could not read check timings: {path}", flush=True)
        return {name: round(seconds, 6) for name, seconds in totals.items()}
