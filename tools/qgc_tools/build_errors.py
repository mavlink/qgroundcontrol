"""Find the errors that explain a failed CMake configure or build in its captured output.

Shared by ``ci/compile/cmake_helper.py`` and ``ci/android/android_lint.py``. It recognizes CMake
``CMake Error`` blocks, Ninja ``FAILED:`` edges and ``ninja: error:``, Xcode's failed build
commands, GCC, Clang, javac, and MSVC compiler errors, GNU, LLVM, mold, and MSVC linker errors, and
Gradle's ``* What went wrong:`` blocks.
"""

from __future__ import annotations

import os
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Sequence

__all__ = ["BuildFailure", "Diagnostic", "cmake_errors", "parse_build_failure", "repo_path"]

MAX_ERRORS = 10
MAX_LINE_CHARS = 500
MAX_BLOCK_LINES = 15

_ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
_CMAKE_AT = re.compile(
    r"^CMake (?:Deprecation )?Error at (?P<file>.+):(?P<line>\d+) \([^)]*\):\s*(?P<message>.*)$"
)
_CMAKE_IN = re.compile(r"^CMake (?:Deprecation )?Error in (?P<file>.+):\s*$")
_CMAKE_PLAIN = re.compile(r"^CMake (?:Deprecation )?Error: (?P<message>.+)$")
_CMAKE_CALL_STACK = "Call Stack (most recent call first):"
_GRADLE_WRONG = "* What went wrong:"
# GCC, Clang, and javac: path:line[:col]: [fatal ]error: message.
_COMPILER = re.compile(
    r"^(?P<file>(?:[A-Za-z]:)?[^:\s][^:]*?):(?P<line>\d+):(?:(?P<col>\d+):)?"
    r" (?:fatal )?error: (?P<message>.+)$"
)
# MSVC and clang-cl: path(line[,col]): [fatal ]error [C1234]: message.
_MSVC = re.compile(
    r"^\s*(?P<file>(?:[A-Za-z]:)?[^:]+?)\((?P<line>\d+)(?:,(?P<col>\d+))?\)\s?:"
    r" (?:fatal )?error(?: (?P<code>C\d+))?: (?P<message>.+)$"
)
_MSVC_LINK = re.compile(r"^\s*(?P<input>.+?) : (?:fatal )?error (?P<code>LNK\d+): (?P<message>.+)$")
_UNDEFINED_REFERENCE = re.compile(
    r"^(?P<input>[^:\s][^:]*):\([^)]*\): (?P<message>undefined reference to .+)$"
)
_NINJA_FAILED = re.compile(r"^FAILED: (?:\[code=\d+\] )?(?P<outputs>.+?)\s*$")
_NINJA_ERROR = re.compile(r"^ninja: error: (?P<message>.+)$")
_NINJA_EDGE_END = re.compile(r"^(?:\[\d+/\d+\] |ninja: |FAILED: )")
# A tool's own error: `ld.lld: error:`, `mold: error:`, `clang: error:`, or a bare `error:`.
_TOOL = re.compile(r"^(?:(?:\S*[/\\])?(?P<tool>[\w.+-]+): )?(?:fatal )?error: (?P<message>.+)$")
_LINKERS = frozenset(
    {"collect2", "ld", "ld.bfd", "ld.gold", "ld.lld", "ld64.lld", "lld-link", "mold"}
)
# Exit-status summaries that only repeat that an earlier error happened.
_SUMMARY_ERROR = re.compile(r"ld returned \d+ exit status|linker command failed with exit code")
_XCODE_FAILED = "The following build commands failed:"


@dataclass(frozen=True)
class Diagnostic:
    """One error: annotation title and message, its source location, and the log excerpt."""

    title: str
    message: str
    file: str = ""
    line: int | None = None
    col: int | None = None
    excerpt: str = ""

    @property
    def location(self) -> str:
        """``file:line:col`` as far as it is known, or ``""`` without a file."""
        numbers = [str(number) for number in (self.line, self.col) if number is not None]
        return ":".join([self.file, *numbers]) if self.file else ""


@dataclass
class BuildFailure:
    """Ninja's (or Xcode's) failed targets and the first distinct errors, in log order."""

    failed_targets: list[str] = field(default_factory=list)
    errors: list[Diagnostic] = field(default_factory=list)
    error_count: int = 0


def _clip(text: str) -> str:
    return text if len(text) <= MAX_LINE_CHARS else f"{text[: MAX_LINE_CHARS - 1]}…"


def _number(value: str | None) -> int | None:
    return int(value) if value else None


def _lines(text: str) -> list[str]:
    return [_ANSI.sub("", line).rstrip() for line in text.splitlines()]


def _block(lines: list[str]) -> str:
    """Join *lines* without blank edges or runs, at most MAX_BLOCK_LINES of them."""
    kept: list[str] = []
    for line in lines:
        if line.strip() or (kept and kept[-1]):
            kept.append(_clip(line))
    while kept and not kept[-1]:
        kept.pop()
    if len(kept) > MAX_BLOCK_LINES:
        kept = [*kept[:MAX_BLOCK_LINES], f"… {len(kept) - MAX_BLOCK_LINES} more lines"]
    return "\n".join(kept)


def _cmake_error(lines: list[str], start: int) -> tuple[Diagnostic, int] | None:
    """Parse the ``CMake Error`` block at *start*; return it and the index after it."""
    header = lines[start]
    match = _CMAKE_AT.match(header) or _CMAKE_IN.match(header) or _CMAKE_PLAIN.match(header)
    if not match:
        return None
    groups = match.groupdict()
    end = start + 1
    while end < len(lines) and (
        not lines[end] or lines[end][0].isspace() or lines[end] == _CMAKE_CALL_STACK
    ):
        end += 1
    body = lines[start + 1 : end]
    stack = body.index(_CMAKE_CALL_STACK) if _CMAKE_CALL_STACK in body else len(body)
    message = _block([groups.get("message") or "", *(line[2:] for line in body[:stack])])
    diagnostic = Diagnostic(
        title="CMake Error",
        message=message or header,
        file=groups.get("file") or "",
        line=_number(groups.get("line")),
        excerpt=_block([header, *body]),
    )
    return diagnostic, end


def cmake_errors(text: str) -> list[Diagnostic]:
    """Return every ``CMake Error`` block in *text*, in order."""
    lines = _lines(text)
    errors = []
    index = 0
    while index < len(lines):
        if lines[index].startswith("CMake ") and (parsed := _cmake_error(lines, index)):
            errors.append(parsed[0])
            index = parsed[1]
        else:
            index += 1
    return errors


def _gradle_error(lines: list[str], start: int) -> tuple[Diagnostic, int]:
    """Parse Gradle's ``* What went wrong:`` block at *start*; return it and the index after it."""
    end = start + 1
    while end < len(lines) and not lines[end].startswith(("* ", "BUILD FAILED", "=====")):
        end += 1
    message = _block(lines[start + 1 : end])
    return Diagnostic(
        title="Gradle build failed",
        message=message,
        excerpt=_block(lines[start:end]),
    ), end


def _line_error(line: str) -> Diagnostic | None:
    """Return the compiler, linker, or tool error that *line* reports, if any."""
    if match := _COMPILER.match(line) or _MSVC.match(line):
        groups = match.groupdict()
        code = f"{groups['code']}: " if groups.get("code") else ""
        return Diagnostic(
            title="Compile error",
            message=_clip(code + match["message"]),
            file=match["file"].strip(),
            line=_number(match["line"]),
            col=_number(match["col"]),
            excerpt=_clip(line),
        )
    if match := _MSVC_LINK.match(line):
        message = f"{match['input']}: {match['code']}: {match['message']}"
        return Diagnostic(title="Link error", message=_clip(message), excerpt=_clip(line))
    if match := _UNDEFINED_REFERENCE.match(line):
        message = f"{match['input']}: {match['message']}"
        return Diagnostic(title="Link error", message=_clip(message), excerpt=_clip(line))
    if match := _NINJA_ERROR.match(line):
        return Diagnostic(title="Ninja error", message=_clip(match["message"]), excerpt=line)
    if match := _TOOL.match(line):
        tool = match["tool"] or ""
        title = "Link error" if tool.removesuffix(".exe") in _LINKERS else "Build error"
        message = f"{tool}: {match['message']}" if tool else match["message"]
        return Diagnostic(title=title, message=_clip(message), excerpt=_clip(line))
    return None


def parse_build_failure(text: str) -> BuildFailure:
    """Return the failed targets and the first MAX_ERRORS distinct errors in build *text*.

    Exit-status summaries (``collect2: error: ld returned 1 exit status``) count only when
    nothing else explains the failure; a failed Ninja edge without a recognized error reports
    its own output instead.
    """
    lines = _lines(text)
    failure = BuildFailure()
    seen: set[tuple[object, ...]] = set()
    summaries: list[Diagnostic] = []
    edges: list[tuple[str, list[str]]] = []
    edge_output: list[str] | None = None

    def add(diagnostic: Diagnostic) -> None:
        key = (diagnostic.file, diagnostic.line, diagnostic.col, diagnostic.message)
        if key in seen:
            return
        seen.add(key)
        failure.error_count += 1
        if len(failure.errors) < MAX_ERRORS:
            failure.errors.append(diagnostic)

    index = 0
    while index < len(lines):
        line = lines[index]
        if _NINJA_EDGE_END.match(line):
            edge_output = None
        if match := _NINJA_FAILED.match(line):
            failure.failed_targets.append(_clip(match["outputs"]))
            edge_output = []
            edges.append((match["outputs"], edge_output))
            # Ninja prints the failed command on the next line, then its output.
            index += 2
            continue
        if line == _XCODE_FAILED:
            index += 1
            while index < len(lines) and lines[index][:1].isspace():
                failure.failed_targets.append(_clip(lines[index].strip()))
                index += 1
            continue
        if line.startswith("CMake ") and (parsed := _cmake_error(lines, index)):
            add(parsed[0])
            index = parsed[1]
            continue
        if line == _GRADLE_WRONG:
            diagnostic, index = _gradle_error(lines, index)
            add(diagnostic)
            continue
        if edge_output is not None:
            edge_output.append(line)
        if diagnostic := _line_error(line):
            if _SUMMARY_ERROR.search(diagnostic.message):
                summaries.append(diagnostic)
            else:
                add(diagnostic)
        index += 1

    if not failure.errors:
        for outputs, output in edges:
            if block := _block(output):
                add(
                    Diagnostic(
                        title="Build step failed", message=f"{outputs}\n{block}", excerpt=block
                    )
                )
        for diagnostic in summaries:
            add(diagnostic)
    return failure


def repo_path(path: str, *, bases: Sequence[Path], repo_root: Path) -> str:
    """Return *path* relative to *repo_root* in POSIX form, or ``""`` outside the checkout.

    A relative *path* resolves against the first of *bases* that holds it, else the first base:
    compilers report paths from the build directory, CMake from the source directory.
    """
    candidate = Path(path)
    if not candidate.is_absolute():
        holders = [base for base in bases if (base / candidate).exists()]
        candidate = (holders or list(bases))[0] / candidate
    try:
        relative = os.path.relpath(os.path.normpath(candidate), os.path.normpath(repo_root))
    except ValueError:
        # Another Windows drive.
        return ""
    if relative == os.pardir or relative.startswith(os.pardir + os.sep) or os.path.isabs(relative):
        return ""
    return Path(relative).as_posix()
