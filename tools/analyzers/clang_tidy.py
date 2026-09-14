"""Clang-Tidy analysis using the configured compilation database."""

import hashlib
import json
from pathlib import Path
from typing import ClassVar

from .compiler import CompilerAnalyzer


class ClangTidyAnalyzer(CompilerAnalyzer):
    name: ClassVar[str] = "clang-tidy"
    executable: ClassVar[str] = "clang-tidy"
    install_hint: ClassVar[str] = "Install clang-tidy matching the analysis compiler."
    profile_checks: bool = False

    def _tool_arguments(self) -> tuple[str, ...]:
        # Cached dependencies can live outside the checkout and inherit another config.
        return (*super()._tool_arguments(), f"--config-file={self.repo_root / '.clang-tidy'}")

    def _file_arguments(self, file: Path) -> tuple[str, ...]:
        if not self.profile_checks:
            return ()
        identity = hashlib.sha256(str(file.resolve()).encode()).hexdigest()[:16]
        profile_dir = self.build_dir / "clang-tidy-profiles" / identity
        profile_dir.mkdir(parents=True, exist_ok=True)
        for previous in profile_dir.glob("*.json"):
            previous.unlink()
        return ("--enable-check-profile", f"--store-check-profile={profile_dir}/")

    def _check_timings(self, files: list[Path]) -> dict[str, float]:
        if not self.profile_checks:
            return {}
        totals: dict[str, float] = {}
        for file in files:
            identity = hashlib.sha256(str(file.resolve()).encode()).hexdigest()[:16]
            for path in (self.build_dir / "clang-tidy-profiles" / identity).glob("*.json"):
                try:
                    profile = json.loads(path.read_text(encoding="utf-8"))["profile"]
                    for name, seconds in profile.items():
                        if name.startswith("time.clang-tidy.") and name.endswith(".wall"):
                            check = name.removeprefix("time.clang-tidy.").removesuffix(".wall")
                            totals[check] = totals.get(check, 0.0) + float(seconds)
                except (OSError, ValueError, KeyError, TypeError, AttributeError):
                    print(f"Could not read check timings: {path}", flush=True)
        return {name: round(seconds, 6) for name, seconds in totals.items()}
