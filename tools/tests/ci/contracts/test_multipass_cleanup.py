"""Exercise run_multipass.py with a fake client: VM ownership, launch timeout, and artifact copy."""

from __future__ import annotations

import os
import subprocess
import sys
from typing import TYPE_CHECKING

import pytest

from ..._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

SCRIPT = REPO_ROOT / "deploy/multipass/run_multipass.py"


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
        [sys.executable, str(SCRIPT)],
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


def test_launch_waits_longer_than_multipass_default(tmp_path: Path) -> None:
    mock = tmp_path / "multipass"
    mock.write_text(
        '#!/bin/sh\nprintf "%s\\n" "$*" >> "$CALL_LOG"\n'
        'case "$1" in\ninfo) exit 1;;\nlaunch) exit 7;;\nesac\n',
        encoding="utf-8",
    )
    mock.chmod(0o755)
    log = tmp_path / "calls"
    subprocess.run(
        [sys.executable, str(SCRIPT)],
        env={
            **os.environ,
            "PATH": f"{tmp_path}:{os.environ['PATH']}",
            "CALL_LOG": str(log),
            "OUTPUT_DIR": str(tmp_path / "output"),
            "QGC_SOURCE_DIR": "",
        },
        capture_output=True,
        check=False,
    )
    launch = next(line for line in log.read_text().splitlines() if line.startswith("launch"))
    assert "--timeout 900" in launch


def test_artifact_is_staged_under_home_then_moved_to_output(tmp_path: Path) -> None:
    # The root snap client may write only under the caller's home, not the runner's workspace.
    home = tmp_path / "home"
    home.mkdir()
    mock = tmp_path / "multipass"
    mock.write_text(
        '#!/bin/sh\nprintf "%s\\n" "$*" >> "$CALL_LOG"\n'
        'case "$1" in\n'
        "info) exit 1;;\n"
        'exec) [ "$5" = qgc-appimage-path ] && echo /home/ubuntu/QGC.AppImage; exit 0;;\n'
        'transfer) printf appimage > "${3%/}/QGC.AppImage";;\n'
        "esac\n",
        encoding="utf-8",
    )
    mock.chmod(0o755)
    log = tmp_path / "calls"
    output = tmp_path / "output"
    result = subprocess.run(
        [sys.executable, str(SCRIPT)],
        env={
            **os.environ,
            "PATH": f"{tmp_path}:{os.environ['PATH']}",
            "CALL_LOG": str(log),
            "HOME": str(home),
            "OUTPUT_DIR": str(output),
            "QGC_SOURCE_DIR": "",
        },
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    (transfer,) = (line for line in log.read_text().splitlines() if line.startswith("transfer"))
    assert transfer.split()[-1].startswith(f"{home}/qgc-out-")
    assert (output / "QGC.AppImage").read_text() == "appimage"
    assert list(home.iterdir()) == []
