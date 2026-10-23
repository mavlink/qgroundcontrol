"""Content-addressed replay of completed compiler-analysis results.

A key covers the analyzer identity and configuration, every compile command for
the translation unit except output-only options, and the contents of every file
its preprocessing read. Only completed analyses are stored: tool failures,
timeouts and crashes always run again.
"""

from __future__ import annotations

import contextlib
import hashlib
import json
import os
import shlex
import threading
import time
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from common.io import read_json

if TYPE_CHECKING:
    from collections.abc import Iterable, Mapping, Sequence
    from pathlib import Path

FORMAT_VERSION = 1
DEFAULT_MAX_BYTES = 256 * 1024 * 1024
DEFAULT_MAX_AGE_SECONDS = 14 * 24 * 60 * 60
_TEMPORARY_AGE_SECONDS = 60 * 60
_OUTPUT_OPTIONS = frozenset({"-o", "-MF", "-MT", "-MQ", "-MJ", "--serialize-diagnostics"})
_OUTPUT_FLAGS = frozenset({"-MD", "-MMD"})
_CRASH_MARKERS = ("Stack dump:", "PLEASE submit a bug report")


@dataclass(frozen=True)
class CachedResult:
    stdout: str
    stderr: str
    returncode: int


def compile_arguments(entry: Mapping[str, Any]) -> list[str]:
    """Return a compilation-database entry's arguments without output-only options."""
    arguments = entry.get("arguments")
    if not isinstance(arguments, list):
        arguments = shlex.split(str(entry.get("command", "")))
    kept: list[str] = []
    skip_value = False
    for argument in arguments:
        if skip_value:
            skip_value = False
        elif argument in _OUTPUT_OPTIONS:
            skip_value = True
        elif argument not in _OUTPUT_FLAGS:
            kept.append(str(argument))
    return kept


def completed_normally(result: CachedResult) -> bool:
    """Return whether a process result may be replayed (no signal exit or crash report)."""
    output = result.stdout + result.stderr
    return result.returncode >= 0 and not any(marker in output for marker in _CRASH_MARKERS)


class ResultCache:
    """Store analyzer output by key, refreshing an entry's age whenever it is replayed."""

    def __init__(
        self,
        directory: Path,
        *,
        max_bytes: int = DEFAULT_MAX_BYTES,
        max_age_seconds: float = DEFAULT_MAX_AGE_SECONDS,
    ) -> None:
        self.directory = directory
        self.max_bytes = max_bytes
        self.max_age_seconds = max_age_seconds
        self._digests: dict[Path, str | None] = {}

    def file_digest(self, path: Path) -> str | None:
        if path not in self._digests:
            try:
                self._digests[path] = hashlib.sha256(path.read_bytes()).hexdigest()
            except OSError:
                self._digests[path] = None
        return self._digests[path]

    def key(
        self,
        identity: Sequence[str],
        entries: Sequence[Mapping[str, Any]],
        dependencies: Iterable[Path],
    ) -> str | None:
        """Return the entry key, or None when a dependency cannot be read."""
        digest = hashlib.sha256()

        def add(value: str) -> None:
            encoded = value.encode("utf-8", "surrogateescape")
            digest.update(len(encoded).to_bytes(8, "little"))
            digest.update(encoded)

        add(f"qgc-analysis-result-v{FORMAT_VERSION}")
        for part in identity:
            add(part)
        for command in sorted(
            json.dumps([entry.get("directory"), entry.get("file"), compile_arguments(entry)])
            for entry in entries
        ):
            add(command)
        for path in sorted(set(dependencies)):
            content = self.file_digest(path)
            if content is None:
                return None
            add(str(path))
            add(content)
        return digest.hexdigest()

    def _path(self, key: str) -> Path:
        return self.directory / key[:2] / f"{key}.json"

    def load(self, key: str) -> CachedResult | None:
        path = self._path(key)
        try:
            data = read_json(path)
            if data["version"] != FORMAT_VERSION:
                return None
            result = CachedResult(str(data["stdout"]), str(data["stderr"]), int(data["returncode"]))
        except (OSError, ValueError, KeyError, TypeError):
            return None
        with contextlib.suppress(OSError):
            os.utime(path)
        return result

    def store(self, key: str, result: CachedResult) -> bool:
        if not completed_normally(result):
            return False
        path = self._path(key)
        temporary = path.with_name(f"{path.name}.{os.getpid()}.{threading.get_ident()}.tmp")
        data = {
            "version": FORMAT_VERSION,
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            temporary.write_text(json.dumps(data), encoding="utf-8")
            os.replace(temporary, path)
        except OSError as exc:
            print(f"Could not store analysis result {key}: {exc}", flush=True)
            with contextlib.suppress(OSError):
                temporary.unlink()
            return False
        return True

    def prune(self, now: float | None = None) -> int:
        """Drop expired entries, then the least recently used beyond the size limit."""
        now = time.time() if now is None else now
        entries: list[tuple[float, int, Path]] = []
        removed = 0
        for path in self.directory.glob("*/*"):
            try:
                stat = path.stat()
            except OSError:
                continue
            # Leave other writers' recent temporary files alone.
            expired = now - stat.st_mtime > (
                self.max_age_seconds if path.suffix == ".json" else _TEMPORARY_AGE_SECONDS
            )
            if expired:
                with contextlib.suppress(OSError):
                    path.unlink()
                    removed += 1
            if path.suffix == ".json" and not expired:
                entries.append((stat.st_mtime, stat.st_size, path))
        total = sum(size for _, size, _ in entries)
        for _, size, path in sorted(entries):
            if total <= self.max_bytes:
                break
            with contextlib.suppress(OSError):
                path.unlink()
                removed += 1
            total -= size
        return removed
