"""Tests for apt_install_retry.sh."""

from __future__ import annotations

import os
import subprocess
from typing import TYPE_CHECKING

from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

SCRIPT = REPO_ROOT / ".github" / "scripts" / "apt_install_retry.sh"


def _write_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(0o755)


def test_update_retries_finish_before_progressive_install_retries(tmp_path: Path) -> None:
    fake_bin = tmp_path / "bin"
    fake_bin.mkdir()
    call_log = tmp_path / "calls.log"
    update_count = tmp_path / "update-count"
    install_count = tmp_path / "install-count"

    _write_executable(
        fake_bin / "sudo",
        """#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$*" >> "${APT_RETRY_CALL_LOG}"
command_name="$1"
shift
case "${command_name}" in
  mkdir|chmod|rm) command "${command_name}" "$@" ;;
  chown|apt-get) exit 0 ;;
  timeout)
    if [[ " $* " == *" update "* ]]; then
      count=0
      [[ -f "${APT_UPDATE_COUNT}" ]] && read -r count < "${APT_UPDATE_COUNT}"
      count=$((count + 1))
      printf '%s\n' "${count}" > "${APT_UPDATE_COUNT}"
      ((count >= 2))
    elif [[ " $* " == *" install "* ]]; then
      count=0
      [[ -f "${APT_INSTALL_COUNT}" ]] && read -r count < "${APT_INSTALL_COUNT}"
      count=$((count + 1))
      printf '%s\n' "${count}" > "${APT_INSTALL_COUNT}"
      ((count >= 3))
    fi
    ;;
esac
""",
    )
    _write_executable(fake_bin / "sleep", "#!/usr/bin/env bash\nexit 0\n")

    env = os.environ.copy()
    env.update(
        {
            "APT_INSTALL_COUNT": str(install_count),
            "APT_RETRY_CALL_LOG": str(call_log),
            "APT_UPDATE_COUNT": str(update_count),
            "HOME": str(tmp_path / "home"),
            "PATH": f"{fake_bin}{os.pathsep}{env['PATH']}",
        }
    )
    result = subprocess.run(
        [str(SCRIPT), "--update", "--autoclean", "fake-package"],
        check=False,
        capture_output=True,
        env=env,
        text=True,
    )

    assert result.returncode == 0, result.stderr
    calls = call_log.read_text(encoding="utf-8").splitlines()
    operations = [
        operation
        for call in calls
        for operation in ("update", "install")
        if f" {operation} " in f" {call} "
    ]
    assert operations == ["update", "update", "install", "install", "install"]
