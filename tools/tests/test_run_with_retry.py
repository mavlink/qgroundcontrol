"""Tests for the command retry wrapper."""

from __future__ import annotations

import subprocess
from unittest.mock import patch

from ._helpers import load_script_module

retry_script = load_script_module("setup/run_with_retry.py", "run_with_retry")


def test_main_forwards_command_and_retry_limits() -> None:
    command = ["python", "-m", "pip", "install", "fastcrc"]
    with patch.object(
        retry_script,
        "run_checked_with_retry",
        return_value=subprocess.CompletedProcess(command, 0),
    ) as run:
        result = retry_script.main(
            [
                "--max-attempts",
                "4",
                "--retry-backoff-seconds",
                "2",
                "--",
                *command,
            ]
        )

    assert result == 0
    run.assert_called_once_with(command, max_attempts=4, retry_backoff_seconds=2.0)


def test_main_returns_final_command_failure() -> None:
    command = ["false"]
    with patch.object(
        retry_script,
        "run_checked_with_retry",
        side_effect=subprocess.CalledProcessError(7, command),
    ):
        assert retry_script.main(["--", *command]) == 7
