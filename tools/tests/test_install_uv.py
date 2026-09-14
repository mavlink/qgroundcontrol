"""Verify uv bootstrapping before the project environment exists."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

import pytest
from setup import install_uv


def test_existing_uv_skips_download_and_install():
    with (
        patch.object(install_uv.shutil, "which", return_value="/usr/bin/uv"),
        patch.object(install_uv.subprocess, "run") as run,
    ):
        assert install_uv.main() == 0
    run.assert_not_called()


@pytest.mark.parametrize("failure", [None, "curl", "sh"])
def test_bootstrap_preserves_security_environment_and_cleanup(monkeypatch, failure, capsys):
    monkeypatch.setenv("UV_INSTALL_DIR", "/custom/bin")
    monkeypatch.setenv("UV_NO_MODIFY_PATH", "0")
    installer = None

    def run(command, *, check, env=None):
        nonlocal installer
        assert check is True
        if command[0] == "curl":
            for flag, value in (
                ("--proto", "=https"),
                ("--proto-redir", "=https"),
                ("--retry", "3"),
            ):
                assert command[command.index(flag) + 1] == value
            assert {"--fail", "--location", "--tlsv1.2"}.issubset(command)
            assert "https://astral.sh/uv/0.11.12/install.sh" in command
            installer = Path(command[command.index("-o") + 1])
            installer.write_text("installer fixture")
        else:
            assert installer is not None
            assert command == ["sh", str(installer)]
            assert installer.read_text() == "installer fixture"
            assert env is not None
            assert env["UV_INSTALL_DIR"] == "/custom/bin"
            assert env["UV_NO_MODIFY_PATH"] == "1"
        if command[0] == failure:
            raise subprocess.CalledProcessError(22, command)

    with (
        patch.object(install_uv.shutil, "which", return_value=None),
        patch.object(install_uv.subprocess, "run", side_effect=run) as invoke,
    ):
        assert install_uv.main() == (1 if failure else 0)
    assert invoke.call_count == (1 if failure == "curl" else 2)
    assert installer is not None
    assert not installer.parent.exists()
    assert os.environ["UV_NO_MODIFY_PATH"] == "0"
    assert bool(capsys.readouterr().err) == bool(failure)


def test_missing_download_tool_reports_failure(capsys):
    with (
        patch.object(install_uv.shutil, "which", return_value=None),
        patch.object(install_uv.subprocess, "run", side_effect=FileNotFoundError("curl")),
    ):
        assert install_uv.main() == 1
    assert "uv installation failed: curl" in capsys.readouterr().err


def test_bootstrap_starts_without_site_packages(tmp_path):
    result = subprocess.run(
        [sys.executable, "-S", str(Path(install_uv.__file__).resolve())],
        env={**os.environ, "PATH": str(tmp_path)},
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 1
    assert "uv installation failed:" in result.stderr
    assert "curl" in result.stderr
    assert "ModuleNotFoundError" not in result.stderr
