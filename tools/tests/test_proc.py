#!/usr/bin/env python3
"""Contracts for subprocess wrappers."""

from __future__ import annotations

import subprocess
import sys

import pytest
from common.proc import run_captured, run_tee, run_text


def test_run_captured_handles_text_input_and_failure_policy() -> None:
    result = run_captured(["echo", "hello"])
    assert (result.returncode, result.stdout.strip()) == (0, "hello")
    assert isinstance(result.stdout, str)
    assert run_captured(["cat"], input_text="payload\n").stdout == "payload\n"
    assert run_captured(["false"]).returncode != 0
    with pytest.raises(subprocess.CalledProcessError):
        run_captured(["false"], check=True)


def test_run_text_returns_stdout_or_default() -> None:
    assert run_text(["echo", "hi"]) == "hi"
    assert run_text(["this-binary-does-not-exist"], default="fallback") == "fallback"
    assert run_text(["false"], default="fb") == "fb"


def test_run_tee_streams_output_and_returns_exit_code(tmp_path) -> None:
    output = tmp_path / "command.log"
    assert run_tee([sys.executable, "-c", "print('streamed')"], output) == 0
    assert output.read_text().strip() == "streamed"
