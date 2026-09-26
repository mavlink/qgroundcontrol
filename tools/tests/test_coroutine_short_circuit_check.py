#!/usr/bin/env python3
"""Tests for coroutine_short_circuit_check.py analyzer."""

from __future__ import annotations

import subprocess
import sys
from typing import TYPE_CHECKING

import pytest

from ._helpers import TOOLS_DIR, load_script_module

if TYPE_CHECKING:
    from pathlib import Path

ANALYZER = TOOLS_DIR / "analyzers" / "coroutine_short_circuit_check.py"
checker = load_script_module(
    "analyzers/coroutine_short_circuit_check.py", "coroutine_short_circuit_check"
)


def operators(source: str) -> list[str]:
    return [violation.operator for violation in checker.analyze_source(source)]


@pytest.mark.parametrize(
    ("statement", "operator"),
    [
        ("if ((flag && !co_await side()) || other) {}", "&&"),
        ("if (!flag || !co_await side()) {}", "||"),
        ("const bool ok = first && co_await second();", "&&"),
        ("co_return flag and co_await side();", "and"),
        ("co_return co_await side() ? 1 : 2;", "?"),
        ("co_return flag ? co_await side() : 2;", "?"),
        ("co_return flag ? 1 : co_await side();", "?"),
        ("if ((co_await side()) || other) {}", "||"),
        ("co_return other && use(co_await side());", "&&"),
        ("result = flag && (value = co_await side());", "&&"),
        ("co_return other(a, b) || wrap(1, (co_await side()).value);", "||"),
    ],
)
def test_flags_awaits_in_short_circuit_operands(statement: str, operator: str) -> None:
    assert operators(f"Task f() {{ {statement} }}") == [operator]


@pytest.mark.parametrize(
    "statement",
    [
        "const bool first = co_await side(); if (!first || other) {}",
        "co_await send(flag && other, flag ? 1 : 2);",
        "if (!co_await side()) { co_return; }",
        "while (co_await side()) {}",
        "for (auto value : co_await values()) { use(flag && value); }",
        "use(flag && other, co_await side());",
        "value = co_await side();",
        "co_return co_await side();",
        "auto task = [&]() -> Task { co_return co_await side(); }; co_return flag && ready;",
        "// co_return flag && co_await side();",
        '/* flag && co_await side() */ const char* text = "a && co_await b";',
        'const auto raw = R"(flag && co_await side())";',
    ],
)
def test_ignores_sequential_awaits(statement: str) -> None:
    assert operators(f"Task f() {{ {statement} }}") == []


def test_flags_every_await_in_the_expression() -> None:
    assert operators("Task f() { co_return co_await first() && co_await second(); }") == [
        "&&",
        "&&",
    ]


def test_ignores_operator_declaration() -> None:
    assert operators("struct Task { auto operator co_await() && noexcept; };") == []


def test_ignores_preprocessor_lines() -> None:
    assert (
        operators(
            "#define AWAIT_BOTH(a, b) (co_await a && \\\n co_await b)\nTask f() { co_return; }"
        )
        == []
    )


def test_reports_the_line() -> None:
    source = "Task f()\n{\n    const bool a = co_await x();\n    co_return a || co_await y();\n}\n"
    [violation] = checker.analyze_source(source, "sample.cc")
    assert (violation.file, violation.line, violation.operator) == ("sample.cc", 4, "||")
    assert violation.code == "co_return a || co_await y();"


def test_exit_codes(tmp_path: Path) -> None:
    bad = tmp_path / "bad.cc"
    bad.write_text("Task f() { co_return flag && co_await g(); }\n")
    good = tmp_path / "good.cc"
    good.write_text("Task f() { const bool value = co_await g(); co_return flag && value; }\n")

    failed = subprocess.run(
        [sys.executable, str(ANALYZER), str(bad)], capture_output=True, text=True, check=False
    )
    assert failed.returncode == 1
    assert f"{bad}:1: error: co_await inside an operand of '&&'" in failed.stdout

    passed = subprocess.run(
        [sys.executable, str(ANALYZER), str(good)], capture_output=True, text=True, check=False
    )
    assert passed.returncode == 0
    assert not passed.stdout


def test_reads_file_list_from_stdin(tmp_path: Path) -> None:
    bad = tmp_path / "bad.h"
    bad.write_text("Task f() { co_return co_await g() || flag; }\n")
    result = subprocess.run(
        [sys.executable, str(ANALYZER)],
        input=f"{bad}\n",
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 1
