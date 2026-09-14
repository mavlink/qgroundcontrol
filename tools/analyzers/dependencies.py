"""Select header dependents using a fresh Clang preprocessing scan."""

from __future__ import annotations

import json
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from common.proc import run_captured


def header_dependents(
    entries: list[dict[str, Any]], headers: set[Path], jobs: int
) -> set[Path] | None:
    """Return dependent sources, or None when a complete scan is unavailable."""
    scanner = shutil.which("clang-scan-deps")
    if scanner is None:
        print("clang-scan-deps unavailable; checking all active project sources", flush=True)
        return None
    expected = {(Path(entry["directory"]) / entry["file"]).resolve() for entry in entries}
    try:
        with tempfile.TemporaryDirectory(prefix="qgc-analysis-deps-") as temp:
            database = Path(temp) / "compile_commands.json"
            database.write_text(json.dumps(entries), encoding="utf-8")
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
        seen: set[Path] = set()
        affected: set[Path] = set()
        for unit in result["translation-units"]:
            for command in unit["commands"]:
                source = Path(command["input-file"]).resolve()
                dependencies = {Path(path).resolve() for path in command["file-deps"]}
                seen.add(source)
                if dependencies & headers:
                    affected.add(source)
        if seen != expected:
            raise ValueError("dependency scan did not cover every compilation unit")
        return affected
    except (OSError, ValueError, KeyError, TypeError, subprocess.TimeoutExpired) as exc:
        print(f"Dependency scan incomplete; checking all active project sources: {exc}", flush=True)
        return None
