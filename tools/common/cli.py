"""Shared argparse fragments for QGC dev-tool CLIs.

Centralizes the small set of flags that recur across ``clean.py``, ``configure.py``,
``coverage.py``, ``run_tests.py`` and friends so option names + help strings stay
consistent.

Each helper mutates the parser in-place and returns it for chaining::

    parser = argparse.ArgumentParser(...)
    add_build_dir(parser)
    add_jobs(parser)
    add_dry_run(parser)
"""

from __future__ import annotations

import os
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    import argparse

__all__ = [
    "add_build_dir",
    "add_ci_flag",
    "add_dry_run",
    "add_jobs",
    "add_json_output",
    "resolve_jobs",
]


def add_dry_run(parser: argparse.ArgumentParser, *, help: str | None = None) -> argparse.ArgumentParser:
    """Add ``-n`` / ``--dry-run`` to *parser*."""
    parser.add_argument(
        "-n", "--dry-run",
        action="store_true",
        help=help or "Show what would happen without making changes",
    )
    return parser


def add_ci_flag(parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    """Add ``--ci`` toggle (writes GITHUB_OUTPUT + step summary)."""
    parser.add_argument(
        "--ci",
        action="store_true",
        help="Enable GitHub Actions outputs (GITHUB_OUTPUT + step summary)",
    )
    return parser


def add_build_dir(
    parser: argparse.ArgumentParser,
    *,
    default: str = "build",
    flag: str = "-B",
    long_flag: str = "--build-dir",
) -> argparse.ArgumentParser:
    """Add ``-B`` / ``--build-dir`` with a default of ``build``."""
    parser.add_argument(
        flag, long_flag,
        default=default,
        help=f"Build directory (default: {default})",
    )
    return parser


def add_jobs(
    parser: argparse.ArgumentParser,
    *,
    default: int = 0,
    help: str | None = None,
) -> argparse.ArgumentParser:
    """Add ``-j`` / ``--jobs``. ``0`` means "auto" (cpu_count)."""
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=default,
        metavar="N",
        help=help or f"Parallel jobs (default: {default}; 0 = cpu_count)",
    )
    return parser


def add_json_output(parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    """Add ``--json`` for machine-readable output."""
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit machine-readable JSON",
    )
    return parser


def resolve_jobs(value: int) -> int:
    """Translate ``--jobs`` value: ``0`` → ``os.cpu_count() or 1``."""
    if value <= 0:
        return os.cpu_count() or 1
    return value
