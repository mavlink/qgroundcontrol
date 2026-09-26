#!/usr/bin/env python3
"""
Coroutine Short-Circuit Analyzer for QGroundControl

GCC 13 and 16 miscompile a co_await nested in an operand of a short-circuit or
conditional operator: in `(flag && !co_await t()) || x` the awaited task runs
even when `flag` is false. Clang evaluates it correctly. The GPS protocol
runtime therefore forbids co_await inside any operand of &&, ||, and, or, or ?:.
Await in its own statement and combine the results afterwards:

    if (!co_await first()) {          // instead of: if (!co_await first() || !co_await second())
        co_return false;
    }
    co_return co_await second();

An operator inside the awaited expression itself (`co_await f(a && b)`) is fine.

Usage:
    python3 coroutine_short_circuit_check.py [files or directories...]
    python3 coroutine_short_circuit_check.py          # Read the file list from stdin

Exit codes:
    0 - No issues found
    1 - Issues found
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}

_SHORT_CIRCUIT = {"&&", "||", "and", "or", "?"}
_ASSIGNMENTS = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="}
_EXPRESSION_STARTS = {"return", "co_return", "co_yield", "throw"}
_CONDITION_KEYWORDS = {"if", "while", "switch", "for", "catch", "constexpr"}
_OPEN = {"(": ")", "[": "]", "{": "}"}
_CLOSE = {value: key for key, value in _OPEN.items()}

_TOKEN = re.compile(
    r"[A-Za-z_]\w*"
    r"|\d[\w.']*"
    r"|<<=|>>=|->\*|\.\.\.|<=>"
    r"|::|->|&&|\|\||<<|>>|<=|>=|==|!=|\+\+|--|\+=|-=|\*=|/=|%=|&=|\|=|\^=|\.\*"
    r"|\S"
)
_RAW_STRING = re.compile(r'(?:u8|u|U|L)?R"([^()\\\s]{0,16})\(')


@dataclass
class Token:
    text: str
    line: int


@dataclass
class Violation:
    file: str
    line: int
    operator: str
    code: str


def _blank(text: str) -> str:
    """Keeps line breaks and replaces everything else with spaces."""
    return re.sub(r"[^\n]", " ", text)


def strip_code(source: str) -> str:
    """Replaces comments, string and character literals and preprocessor lines with blanks, keeping line numbers."""
    out: list[str] = []
    index = 0
    length = len(source)
    line_start = True
    while index < length:
        char = source[index]
        if line_start and char == "#":
            end = source.find("\n", index)
            while end > 0 and source[end - 1] == "\\":
                end = source.find("\n", end + 1)
            end = length if end < 0 else end
            out.append(_blank(source[index:end]))
            index = end
            continue
        if char == "\n":
            line_start = True
            out.append(char)
            index += 1
            continue
        if not char.isspace():
            line_start = False
        raw = _RAW_STRING.match(source, index)
        if raw and (index == 0 or not (source[index - 1].isalnum() or source[index - 1] == "_")):
            terminator = ")" + raw.group(1) + '"'
            end = source.find(terminator, raw.end())
            end = length if end < 0 else end + len(terminator)
            out.append(_blank(source[index:end]))
            index = end
            continue
        if source.startswith("//", index):
            end = source.find("\n", index)
            end = length if end < 0 else end
            out.append(_blank(source[index:end]))
            index = end
            continue
        if source.startswith("/*", index):
            end = source.find("*/", index + 2)
            end = length if end < 0 else end + 2
            out.append(_blank(source[index:end]))
            index = end
            continue
        if char in "\"'" and not (char == "'" and index > 0 and source[index - 1].isalnum()):
            end = index + 1
            while end < length and source[end] != char and source[end] != "\n":
                end += 2 if source[end] == "\\" else 1
            end = min(end + 1, length)
            out.append(_blank(source[index:end]))
            index = end
            continue
        out.append(char)
        index += 1
    return "".join(out)


def tokenize(code: str) -> list[Token]:
    tokens: list[Token] = []
    line = 1
    position = 0
    for match in _TOKEN.finditer(code):
        line += code.count("\n", position, match.start())
        position = match.start()
        tokens.append(Token(match.group(), line))
    return tokens


def match_brackets(tokens: list[Token]) -> dict[int, int]:
    matches: dict[int, int] = {}
    stack: list[int] = []
    for index, token in enumerate(tokens):
        if token.text in _OPEN:
            stack.append(index)
        elif token.text in _CLOSE:
            while stack and tokens[stack[-1]].text != _CLOSE[token.text]:
                stack.pop()
            if stack:
                opening = stack.pop()
                matches[opening] = index
                matches[index] = opening
    return matches


def _enclosing_open(tokens: list[Token], matches: dict[int, int], index: int) -> int | None:
    position = index - 1
    while position >= 0:
        text = tokens[position].text
        if text in _CLOSE and position in matches:
            position = matches[position] - 1
        elif text in _OPEN:
            return position
        else:
            position -= 1
    return None


def _operator_left(tokens: list[Token], matches: dict[int, int], index: int) -> str | None:
    position = index - 1
    while position >= 0:
        text = tokens[position].text
        if text in _CLOSE and position in matches:
            position = matches[position] - 1
            continue
        if (
            text in _OPEN
            or text in _ASSIGNMENTS
            or text in _EXPRESSION_STARTS
            or text in {",", ";"}
        ):
            return None
        if text in _SHORT_CIRCUIT:
            return text
        position -= 1
    return None


def _operator_right(tokens: list[Token], matches: dict[int, int], index: int) -> str | None:
    position = index + 1
    while position < len(tokens):
        text = tokens[position].text
        if text in _OPEN and position in matches:
            position = matches[position] + 1
            continue
        if text in _CLOSE or text in _ASSIGNMENTS or text in {",", ";", ":"}:
            return None
        if text in _SHORT_CIRCUIT:
            return text
        position += 1
    return None


def short_circuit_operator(tokens: list[Token], matches: dict[int, int], index: int) -> str | None:
    """The operator whose operand contains the co_await at @a index, looking outward through enclosing groups."""
    first = last = index
    while True:
        operator = _operator_left(tokens, matches, first) or _operator_right(tokens, matches, last)
        if operator:
            return operator
        opening = _enclosing_open(tokens, matches, first)
        if opening is None or tokens[opening].text == "{" or opening not in matches:
            return None
        if (
            tokens[opening].text == "("
            and opening > 0
            and tokens[opening - 1].text in _CONDITION_KEYWORDS
        ):
            return None
        first, last = opening, matches[opening]


def analyze_source(source: str, file: str = "<source>") -> list[Violation]:
    code = strip_code(source)
    lines = source.splitlines()
    tokens = tokenize(code)
    matches = match_brackets(tokens)
    violations: list[Violation] = []
    for index, token in enumerate(tokens):
        if token.text != "co_await" or (index > 0 and tokens[index - 1].text == "operator"):
            continue
        operator = short_circuit_operator(tokens, matches, index)
        if operator:
            text = lines[token.line - 1].strip() if token.line <= len(lines) else ""
            violations.append(Violation(file, token.line, operator, text))
    return violations


def analyze_file(path: Path) -> list[Violation]:
    try:
        source = path.read_text(encoding="utf-8", errors="replace")
    except OSError as error:
        print(f"Warning: Could not read {path}: {error}", file=sys.stderr)
        return []
    return analyze_source(source, str(path))


def _files(arguments: list[str]) -> list[Path]:
    if not arguments:
        return [Path(line.strip()) for line in sys.stdin if line.strip()]
    files: list[Path] = []
    for argument in arguments:
        path = Path(argument)
        if path.is_dir():
            files.extend(
                sorted(p for p in path.rglob("*") if p.suffix in CPP_SUFFIXES and p.is_file())
            )
        elif path.is_file():
            files.append(path)
    return files


def main() -> int:
    violations = [violation for path in _files(sys.argv[1:]) for violation in analyze_file(path)]
    for violation in violations:
        print(
            f"{violation.file}:{violation.line}: error: co_await inside an operand of '{violation.operator}'"
        )
        print(f"  code: {violation.code}")
        print("  fix:  await in its own statement, then combine the results")
        print()
    if violations:
        print(
            f"{len(violations)} co_await(s) inside a short-circuit or conditional operand.\n"
            "GCC evaluates such a co_await even when the operator short-circuits."
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
