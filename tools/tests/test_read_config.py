"""Contract tests for build-config helpers and their CLI."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from typing import TYPE_CHECKING

import pytest
from common.build_config import export_build_config_values, get_build_config_value, lookup_dotted

from ._helpers import TOOLS_DIR

if TYPE_CHECKING:
    from pathlib import Path

SCRIPT = TOOLS_DIR / "setup" / "read_config.py"


def _config() -> dict[str, object]:
    return {
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
            }
        },
        "android": {"platform": "35", "min_sdk": "28"},
    }


def _run(
    config: Path, *arguments: str, env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    run_env = {**os.environ, "CONFIG_FILE": str(config), **(env or {})}
    return subprocess.run(
        [sys.executable, str(SCRIPT), *arguments],
        capture_output=True,
        text=True,
        env=run_env,
        check=False,
    )


def test_build_config_lookup_distinguishes_values_null_and_missing(tmp_path: Path) -> None:
    data = {"qt": {"version": 611, "minimum_version": None}}
    assert lookup_dotted(data, "qt.version") == 611
    assert lookup_dotted(data, "qt.minimum_version", "fallback") is None
    assert lookup_dotted(data, "qt.missing", "fallback") == "fallback"
    with pytest.raises(KeyError, match=r"qt\.missing"):
        lookup_dotted(data, "qt.missing")

    path = tmp_path / "build-config.json"
    path.write_text(json.dumps(data))
    assert get_build_config_value("qt.version", config_file=path) == "611"
    assert get_build_config_value("qt.minimum_version", "fallback", config_file=path) == "fallback"
    assert export_build_config_values(data, keys=["qt.version", "qt.minimum_version"]) == {
        "QT_VERSION": "611"
    }


def test_cli_get_handles_value_null_alias_and_missing(tmp_path: Path) -> None:
    path = tmp_path / "build-config.json"
    path.write_text(json.dumps(_config()))
    for key, expected in (
        ("qt.version", "6.10.2"),
        ("gstreamer_version", "1.28.2"),
    ):
        result = _run(path, "--get", key)
        assert result.returncode == 0
        assert result.stdout.strip() == expected

    path.write_text(json.dumps({"qt": {"version": None}}))
    assert _run(path, "--get", "qt.version").stdout.strip() == "null"
    missing = _run(path, "--get", "qt.missing")
    assert missing.returncode == 1
    assert "not found" in missing.stderr


def test_cli_exports_shell_values_without_overescaping(tmp_path: Path) -> None:
    data = _config()
    data["qt"]["version"] = "6.10.2!beta"  # type: ignore[index]
    path = tmp_path / "build-config.json"
    path.write_text(json.dumps(data))

    result = _run(path, "--export", "bash")

    assert result.returncode == 0
    assert 'export QT_VERSION="6.10.2!beta"' in result.stdout
    assert 'export QT_MODULES="qtpositioning qtserialport qtscxml"' in result.stdout
    assert 'export GSTREAMER_VERSION="1.28.2"' in result.stdout
    assert "\\!" not in result.stdout


def test_cli_writes_github_outputs_and_derived_ios_modules(tmp_path: Path) -> None:
    path = tmp_path / "build-config.json"
    path.write_text(json.dumps(_config()))
    output = tmp_path / "github-output"
    environment = tmp_path / "github-env"

    result = _run(
        path,
        "--github-output",
        env={"GITHUB_OUTPUT": str(output), "GITHUB_ENV": str(environment)},
    )

    assert result.returncode == 0
    output_text = output.read_text()
    for value in (
        "qt_version=6.10.2",
        "qt_minimum_version=6.8.0",
        "gstreamer_version=1.28.2",
        "gstreamer_windows_version=1.26.6",
        "gstreamer_minimum_version=1.24.0",
        "gstreamer_android_version=1.28.1",
        "gstreamer_macos_version=1.28.2",
        "gstreamer_ios_version=1.28.2",
        "android_min_sdk=28",
        "qt_modules_ios=qtpositioning qtscxml",
    ):
        assert value in output_text
    assert "QT_VERSION=6.10.2" in environment.read_text()
