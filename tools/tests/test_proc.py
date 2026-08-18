#!/usr/bin/env python3
"""Tests for tools/common/proc.py."""

from __future__ import annotations

import subprocess
import sys
from typing import TYPE_CHECKING

import pytest
from common.proc import run_captured, run_tee, run_text

if TYPE_CHECKING:
    from pathlib import Path


def test_run_captured_returns_completed_process() -> None:
    result = run_captured(["echo", "hello"])
    assert result.returncode == 0
    assert result.stdout.strip() == "hello"
    assert isinstance(result.stdout, str)


def test_run_captured_check_raises() -> None:
    with pytest.raises(subprocess.CalledProcessError):
        run_captured(["false"], check=True)


def test_run_captured_failing_command_does_not_raise_by_default() -> None:
    result = run_captured(["false"])
    assert result.returncode != 0


def test_run_captured_input() -> None:
    result = run_captured(["cat"], input_text="payload\n")
    assert result.stdout == "payload\n"


def test_run_text_returns_stdout() -> None:
    assert run_text(["echo", "hi"]) == "hi"


def test_run_text_default_on_missing_binary() -> None:
    assert run_text(["this-binary-does-not-exist"], default="fallback") == "fallback"


def test_run_text_default_on_nonzero_exit() -> None:
    assert run_text(["false"], default="fb") == "fb"


def test_run_tee_streams_output_and_returns_exit_code(tmp_path: Path) -> None:
    output = tmp_path / "command.log"
    assert run_tee([sys.executable, "-c", "print('streamed')"], output) == 0
    assert output.read_text(encoding="utf-8").strip() == "streamed"


def test_run_tee_falls_back_without_bash(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output = tmp_path / "fallback.log"
    monkeypatch.setattr("common.proc.shutil.which", lambda _name: None)
    assert run_tee([sys.executable, "-c", "print('fallback')"], output) == 0
    assert output.read_text(encoding="utf-8").strip() == "fallback"
