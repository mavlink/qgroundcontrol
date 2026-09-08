"""Check the sourceable retry helper under both supported shell families."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

SCRIPT = Path(__file__).resolve().parents[3] / "deploy/docker/lib/retry.sh"


@pytest.mark.parametrize("shell", ["sh", "bash"])
@pytest.mark.parametrize(("succeed_on", "expected"), [(1, 0), (3, 0), (4, 7)])
def test_retry_preserves_status(shell: str, succeed_on: int, expected: int) -> None:
    result = subprocess.run(
        [
            shell,
            "-c",
            '. "$1"; sleep() { :; }; calls=0; '
            'work() { calls=$((calls + 1)); [ "$calls" -ge "$2" ] || return 7; }; '
            'retry work "$1" "$2"; result=$?; echo "$calls"; exit "$result"',
            "test",
            str(SCRIPT),
            str(succeed_on),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == expected
    assert int(result.stdout) == min(succeed_on, 3)
