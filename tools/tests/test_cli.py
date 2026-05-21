#!/usr/bin/env python3
"""Tests for tools/common/cli.py."""

from __future__ import annotations

import argparse
from unittest.mock import patch

from common.cli import (
    add_build_dir,
    add_ci_flag,
    add_dry_run,
    add_jobs,
    add_json_output,
    resolve_jobs,
)


def _parser() -> argparse.ArgumentParser:
    return argparse.ArgumentParser()


def test_add_dry_run_default_false() -> None:
    args = add_dry_run(_parser()).parse_args([])
    assert args.dry_run is False


def test_add_dry_run_short_flag() -> None:
    args = add_dry_run(_parser()).parse_args(["-n"])
    assert args.dry_run is True


def test_add_dry_run_long_flag() -> None:
    args = add_dry_run(_parser()).parse_args(["--dry-run"])
    assert args.dry_run is True


def test_add_ci_flag() -> None:
    args = add_ci_flag(_parser()).parse_args(["--ci"])
    assert args.ci is True


def test_add_build_dir_default() -> None:
    args = add_build_dir(_parser()).parse_args([])
    assert args.build_dir == "build"


def test_add_build_dir_override() -> None:
    args = add_build_dir(_parser()).parse_args(["-B", "out"])
    assert args.build_dir == "out"


def test_add_build_dir_custom_default() -> None:
    args = add_build_dir(_parser(), default="build-debug").parse_args([])
    assert args.build_dir == "build-debug"


def test_add_jobs_default_zero() -> None:
    args = add_jobs(_parser()).parse_args([])
    assert args.jobs == 0


def test_add_jobs_explicit() -> None:
    args = add_jobs(_parser()).parse_args(["-j", "4"])
    assert args.jobs == 4


def test_add_json_output() -> None:
    args = add_json_output(_parser()).parse_args(["--json"])
    assert args.json is True


def test_resolve_jobs_explicit() -> None:
    assert resolve_jobs(8) == 8


def test_resolve_jobs_auto_uses_cpu_count() -> None:
    with patch("common.cli.os.cpu_count", return_value=12):
        assert resolve_jobs(0) == 12


def test_resolve_jobs_auto_fallback_when_cpu_count_none() -> None:
    with patch("common.cli.os.cpu_count", return_value=None):
        assert resolve_jobs(0) == 1


def test_helpers_chainable() -> None:
    parser = add_jobs(add_dry_run(add_build_dir(_parser())))
    args = parser.parse_args(["-B", "out", "-n", "-j", "2"])
    assert args.build_dir == "out"
    assert args.dry_run is True
    assert args.jobs == 2
