"""Exercise VM ownership without launching or deleting real instances."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest

SCRIPT = Path(__file__).resolve().parents[3] / "deploy/multipass/run-multipass.sh"


@pytest.mark.parametrize(
    ("exists", "launch_rc", "expected_rc", "deleted"),
    [(True, 0, 1, False), (False, 7, 7, False), (False, 0, 8, True)],
)
def test_cleanup_only_owns_successfully_created_vm(
    tmp_path: Path, exists: bool, launch_rc: int, expected_rc: int, deleted: bool
) -> None:
    mock = tmp_path / "multipass"
    mock.write_text(
        '#!/bin/sh\nprintf "%s\\n" "$*" >> "$CALL_LOG"\n'
        'case "$1" in\n'
        f"info) exit {0 if exists else 1};;\n"
        f"launch) exit {launch_rc};;\n"
        "exec) exit 8;;\n"
        "esac\n",
        encoding="utf-8",
    )
    mock.chmod(0o755)
    log = tmp_path / "calls"
    result = subprocess.run(
        ["bash", str(SCRIPT)],
        env={
            **os.environ,
            "PATH": f"{tmp_path}:{os.environ['PATH']}",
            "CALL_LOG": str(log),
            "MP_NAME": "existing-user-vm",
            "OUTPUT_DIR": str(tmp_path / "output"),
            "QGC_SOURCE_DIR": "",
        },
        capture_output=True,
        check=False,
    )
    assert result.returncode == expected_rc
    assert ("delete --purge existing-user-vm" in log.read_text()) is deleted
