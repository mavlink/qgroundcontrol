#!/usr/bin/env python3
"""Tests for tools/setup/read_config.py."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path


TOOLS_DIR = Path(__file__).parent.parent
SCRIPT = TOOLS_DIR / "setup" / "read_config.py"


def _run_read_config(*args: str, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    run_env = os.environ.copy()
    if env:
        run_env.update(env)
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        capture_output=True,
        text=True,
        env=run_env,
        check=False,
    )


def _write_config(path: Path) -> None:
    config = {
        "qt_version": "6.10.2",
        "qt_minimum_version": "6.8.0",
        "qt_modules": "qtpositioning qtserialport qtscxml",
        "gstreamer_windows_version": "1.26.6",
        "android_platform": "35",
    }
    path.write_text(json.dumps(config), encoding="utf-8")


def test_get_single_value(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--get", "qt_version", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    assert result.stdout.strip() == "6.10.2"


def test_missing_key_returns_error(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--get", "not_a_real_key", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 1
    assert "not found" in result.stderr


def test_export_bash_format(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--export", "bash", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    assert 'export QT_VERSION="6.10.2"' in result.stdout
    assert 'export QT_MODULES="qtpositioning qtserialport qtscxml"' in result.stdout


def test_export_bash_preserves_bang_character(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    config.write_text(json.dumps({"qt_version": "6.10.2!beta"}), encoding="utf-8")

    result = _run_read_config("--export", "bash", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    assert 'export QT_VERSION="6.10.2!beta"' in result.stdout
    assert "\\!" not in result.stdout


def test_github_output_includes_ios_modules(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    github_output = tmp_path / "github_output.txt"
    github_env = tmp_path / "github_env.txt"

    result = _run_read_config(
        "--github-output",
        env={
            "CONFIG_FILE": str(config),
            "GITHUB_OUTPUT": str(github_output),
            "GITHUB_ENV": str(github_env),
        },
    )

    assert result.returncode == 0

    output_text = github_output.read_text(encoding="utf-8")
    assert "qt_version=6.10.2" in output_text
    assert "qt_minimum_version=6.8.0" in output_text
    assert "gstreamer_windows_version=1.26.6" in output_text
    # Derived value excludes qtserialport and qtscxml and normalizes spacing.
    assert "qt_modules_ios=qtpositioning" in output_text

    env_text = github_env.read_text(encoding="utf-8")
    assert "QT_VERSION=6.10.2" in env_text
