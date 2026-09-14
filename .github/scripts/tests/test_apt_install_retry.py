"""Exercise bounded apt retries and cache cleanup without installing packages."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import apt_install_retry
import pytest
from _helpers import REPO_ROOT

SCRIPT = REPO_ROOT / ".github" / "scripts" / "apt_install_retry.py"


def _write_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(0o755)


def test_update_retries_finish_before_progressive_install_retries(
    tmp_path: Path, monkeypatch
) -> None:
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
    monkeypatch.setenv("APT_INSTALL_COUNT", str(install_count))
    monkeypatch.setenv("APT_RETRY_CALL_LOG", str(call_log))
    monkeypatch.setenv("APT_UPDATE_COUNT", str(update_count))
    monkeypatch.setenv("PATH", f"{fake_bin}{os.pathsep}{os.environ['PATH']}")
    monkeypatch.setattr(Path, "home", lambda: tmp_path / "home")
    delays = []
    monkeypatch.setattr(apt_install_retry.time, "sleep", delays.append)

    assert apt_install_retry.main(["--update", "--autoclean", "fake-package"]) == 0
    assert delays == [15, 15, 30]
    calls = call_log.read_text(encoding="utf-8").splitlines()
    operations = [
        operation
        for call in calls
        for operation in ("update", "install")
        if f" {operation} " in f" {call} "
    ]
    assert operations == ["update", "update", "install", "install", "install"]


@pytest.mark.parametrize("failure", [None, "update", "install", "autoclean"])
def test_failures_preserve_timeout_policy_and_cache_ownership(tmp_path, monkeypatch, failure):
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    calls = []
    delays = []
    monkeypatch.setattr(apt_install_retry.time, "sleep", delays.append)

    def run(command, *, check, **kwargs):
        calls.append(command)
        code = 100 if failure in command else 0
        if check and code:
            raise subprocess.CalledProcessError(code, command)
        return subprocess.CompletedProcess(command, code)

    monkeypatch.setattr(apt_install_retry.subprocess, "run", run)
    result = apt_install_retry.main(
        ["--update", "--autoclean", "--no-install-recommends", "--install-timeout", "42", "pkg"]
    )
    assert result == (1 if failure in ("update", "install") else 0)
    attempts = [command for command in calls if command[:2] == ["sudo", "timeout"]]
    updates = [command for command in attempts if "update" in command]
    installs = [command for command in attempts if "install" in command]
    assert len(updates) == (3 if failure == "update" else 1)
    assert len(installs) == (0 if failure == "update" else 3 if failure == "install" else 1)
    assert delays == ([15, 30] if failure in ("update", "install") else [])
    for command in attempts:
        assert command[:6] == [
            "sudo",
            "timeout",
            "-k",
            "30",
            "300" if "update" in command else "42",
            "apt-get",
        ]
        assert "Acquire::Retries=3" in command
        assert "Acquire::http::Timeout=30" in command
        assert "Acquire::https::Timeout=30" in command
        assert "DPkg::Lock::Timeout=300" in command
        assert f"Dir::Cache::Archives={tmp_path}/.cache/qgc-apt-archives" in command
        assert "APT::Keep-Downloaded-Packages=true" in command
    for command in installs:
        assert command[-4:] == ["install", "-y", "--no-install-recommends", "pkg"]
    assert "autoclean" in calls[-3]
    assert calls[-2] == ["sudo", "rm", "-rf", str(tmp_path / ".cache/qgc-apt-archives/partial")]
    assert calls[-1] == [
        "sudo",
        "chown",
        "-R",
        f"{os.getuid()}:{os.getgid()}",
        str(tmp_path / ".cache/qgc-apt-archives"),
    ]


@pytest.mark.parametrize("args", [[], ["--install-timeout", "0", "pkg"], ["--unknown", "pkg"]])
def test_invalid_arguments_fail_before_running_commands_without_site_packages(args):
    result = subprocess.run(
        [sys.executable, "-S", str(SCRIPT), *args],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 2
    assert "error:" in result.stderr
    assert "ModuleNotFoundError" not in result.stderr
