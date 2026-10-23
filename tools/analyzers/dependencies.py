"""Clang preprocessing scans for header selection and analysis result keys."""

from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from common.io import write_json
from common.proc import require_tools, run_captured


class DependencyScanError(RuntimeError):
    """The scanner is unavailable or did not cover every compilation unit."""


def scan_dependencies(entries: list[dict[str, Any]], jobs: int) -> dict[Path, frozenset[Path]]:
    """Return every file each source's preprocessing reads, merged across its compile commands."""
    expected = {(Path(entry["directory"]) / entry["file"]).resolve() for entry in entries}
    try:
        scanner = require_tools("clang-scan-deps")["clang-scan-deps"]
        with tempfile.TemporaryDirectory(prefix="qgc-analysis-deps-") as temp:
            database = Path(temp) / "compile_commands.json"
            write_json(database, entries, indent=None)
            scan = run_captured(
                [
                    scanner,
                    f"-compilation-database={database}",
                    "-format=experimental-full",
                    "-j",
                    str(jobs),
                ],
                timeout=300,
            )
        if scan.returncode:
            raise ValueError(scan.stderr[-2000:] or "dependency scanner failed")
        result = json.loads(scan.stdout)
        if not isinstance(result, dict):
            raise ValueError("expected an object from dependency scanner")
        if result.get("modules"):
            raise ValueError("module dependencies require a full scan")
        # Units share most headers; resolve each reported path once.
        resolved: dict[str, Path] = {}
        dependencies: dict[Path, set[Path]] = {}
        for unit in result["translation-units"]:
            for command in unit["commands"]:
                source = Path(command["input-file"]).resolve()
                files = dependencies.setdefault(source, set())
                for path in command["file-deps"]:
                    if path not in resolved:
                        resolved[path] = Path(path).resolve()
                    files.add(resolved[path])
        if dependencies.keys() != expected:
            raise ValueError("dependency scan did not cover every compilation unit")
        return {source: frozenset(files) for source, files in dependencies.items()}
    except (OSError, ValueError, KeyError, TypeError, subprocess.TimeoutExpired) as exc:
        raise DependencyScanError(f"Dependency scan incomplete: {exc}") from exc
