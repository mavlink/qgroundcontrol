"""Tests for tools/setup/read_config.py."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from typing import TYPE_CHECKING

import pytest

from ._helpers import TOOLS_DIR, read_github_file

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows

SCRIPT = TOOLS_DIR / "setup" / "read_config.py"


def _run_read_config(
    *args: str, env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
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
        "qt": {
            "version": "6.10.2",
            "minimum_version": "6.8.0",
            "modules": "qtpositioning qtserialport qtscxml",
        },
        "gstreamer": {
            "version": {
                "default": "1.28.2",
                "minimum": "1.24.0",
                "android": "1.28.1",
                "macos": "1.28.2",
                "ios": "1.28.2",
                "windows": "1.26.6",
            },
        },
        "android": {"platform": "35"},
    }
    path.write_text(json.dumps(config), encoding="utf-8")


def test_get_single_value(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--get", "qt.version", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    assert result.stdout.strip() == "6.10.2"


def test_missing_key_returns_error(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--get", "not_a_real_key", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 1
    assert "not found" in result.stderr


def test_docker_layout_finds_the_config_by_the_normal_repository_walk(tmp_path: Path) -> None:
    dockerfile = (TOOLS_DIR.parent / "deploy/docker/Dockerfile").read_text(encoding="utf-8")
    assert "COPY .github/build-config.json /tmp/.github/\n" in dockerfile
    assert (
        "COPY tools/setup/read_config.py tools/setup/install_qt.py /tmp/tools/setup/\n"
        in dockerfile
    )
    tools = tmp_path / "tools"
    shutil.copytree(TOOLS_DIR / "common", tools / "common")
    shutil.copy2(TOOLS_DIR / "_bootstrap.py", tools)
    (tools / "setup").mkdir()
    shutil.copy2(SCRIPT, tools / "setup")
    (tmp_path / ".github").mkdir()
    _write_config(tmp_path / ".github" / "build-config.json")
    env = {key: value for key, value in os.environ.items() if key != "CONFIG_FILE"}

    result = subprocess.run(
        [sys.executable, str(tools / "setup" / "read_config.py"), "--get", "qt.version"],
        capture_output=True,
        text=True,
        env=env,
        check=False,
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == "6.10.2"


def test_export_bash_format(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--export", "bash", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    assert 'export QT_VERSION="6.10.2"' in result.stdout
    assert 'export QT_MODULES="qtpositioning qtserialport qtscxml"' in result.stdout
    assert 'export GSTREAMER_VERSION="1.28.2"' in result.stdout


def test_export_bash_preserves_bang_character(tmp_path: Path) -> None:
    config = tmp_path / "build-config.json"
    config.write_text(json.dumps({"qt": {"version": "6.10.2!beta"}}), encoding="utf-8")

    result = _run_read_config("--export", "bash", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    assert 'export QT_VERSION="6.10.2!beta"' in result.stdout
    assert "\\!" not in result.stdout


def test_github_output_includes_ios_modules(tmp_path: Path, gh_output: Path, gh_env: Path) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)

    result = _run_read_config("--github-output", env={"CONFIG_FILE": str(config)})

    assert result.returncode == 0
    expected = {
        "qt_version": "6.10.2",
        "qt_minimum_version": "6.8.0",
        "gstreamer_version": "1.28.2",
        "gstreamer_windows_version": "1.26.6",
        "gstreamer_minimum_version": "1.24.0",
        "gstreamer_android_version": "1.28.1",
        "gstreamer_macos_version": "1.28.2",
        "gstreamer_ios_version": "1.28.2",
        # Derived value excludes qtserialport and normalizes spacing.
        "qt_modules_ios": "qtpositioning qtscxml",
    }
    assert expected.items() <= read_github_file(gh_output).items()
    assert read_github_file(gh_env)["QT_VERSION"] == "6.10.2"


def test_github_output_writes_only_requested_keys(
    tmp_path: Path, gh_output: Path, gh_env: Path
) -> None:
    config = tmp_path / "build-config.json"
    _write_config(config)
    env = {"CONFIG_FILE": str(config)}

    result = _run_read_config("--github-output", "qt.version", "Android.Platform", env=env)

    assert result.returncode == 0, result.stderr
    assert read_github_file(gh_output) == {"qt_version": "6.10.2", "android_platform": "35"}
    assert gh_env.read_text() == ""

    gh_output.write_text("")
    result = _run_read_config("--github-output", "qt.version", "tools.packer.version", env=env)
    assert result.returncode == 1
    assert "Key 'tools.packer.version' not found" in result.stderr
    assert gh_output.read_text() == ""
