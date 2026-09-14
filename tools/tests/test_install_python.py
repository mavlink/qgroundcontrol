"""Environment integration tests shared by the Python 3.10/3.12 CI matrix."""

from __future__ import annotations

import subprocess
import sys
from typing import TYPE_CHECKING
from unittest.mock import patch

import pytest
from qgc_tools import python_env
from setup.install_python import main

if TYPE_CHECKING:
    from pathlib import Path


def test_requirements_honor_markers_and_versions() -> None:
    assert python_env.check_requirements(["absent-package; python_version < '2'"]) == 0
    assert python_env.check_requirements(["jinja2>=9999"]) == 1
    assert python_env.check_requirements(["jinja2>=3"]) == 0


def test_unknown_group_rejected() -> None:
    with pytest.raises(ValueError, match="Unknown group"):
        python_env.requirements_for("nope")


def test_nested_profile_contains_generator_and_test_dependencies() -> None:
    packages = python_env.requirements_for("dev")
    assert "pytest" in packages
    assert "packaging>=24" in packages
    assert "aqtinstall" in packages
    assert len(packages) == len(set(packages))


def test_missing_uv_never_falls_back_to_system_install() -> None:
    with (
        patch("qgc_tools.python_env.shutil.which", return_value=None),
        pytest.raises(FileNotFoundError, match="uv is required"),
    ):
        python_env.sync_groups("scripts")


def test_explicit_override_is_isolated() -> None:
    with patch("qgc_tools.python_env.require_uv", return_value="uv"):
        assert python_env.tool_command("aqt", "qt", source="aqtinstall==3.3.0") == [
            "uv",
            "tool",
            "run",
            "--isolated",
            "--from",
            "aqtinstall==3.3.0",
            "aqt",
        ]


def test_tool_uses_resolved_environment_not_path(tmp_path: Path) -> None:
    tool = python_env.executable("aqt", tmp_path)
    tool.parent.mkdir(parents=True)
    tool.touch()
    with patch("qgc_tools.python_env.sync_groups", return_value=tmp_path):
        assert python_env.tool_command("aqt", "qt") == [str(tool)]


def test_fresh_environment_and_repeated_setup_preserve_tools(tmp_path: Path) -> None:
    environment = tmp_path / "venv"
    python_env.sync_groups("scripts,test", environment=environment, python=sys.executable)
    interpreter = python_env.executable("python", environment)
    setup_script = python_env.project_path() / "setup/install_python.py"
    result = subprocess.run(
        [str(interpreter), str(setup_script), "scripts", "--check"], capture_output=True, text=True
    )
    assert result.returncode == 0, result.stderr
    python_env.sync_groups("scripts", environment=environment)
    result = subprocess.run(
        [str(interpreter), "-m", "pytest", "--version"], capture_output=True, text=True
    )
    assert result.returncode == 0, result.stderr
    python_env.sync_groups("scripts", environment=environment, replace=True)
    result = subprocess.run(
        [
            str(interpreter),
            "-c",
            "import importlib.util; assert importlib.util.find_spec('pytest') is None",
        ]
    )
    assert result.returncode == 0


def test_check_does_not_mutate_environment() -> None:
    with patch("setup.install_python.sync_groups") as sync:
        assert main(["scripts", "--check"]) == 0
    sync.assert_not_called()


@pytest.mark.parametrize("flag", ["--help", "--list", "--print-packages", "--dry-run"])
def test_system_setup_queries_do_not_install_site_packages(tmp_path: Path, flag: str) -> None:
    import os

    environment = {
        **os.environ,
        "PATH": os.environ.get("PATH", "") if flag == "--dry-run" else "",
        "QGC_PYTHON_ENV": str(tmp_path / "absent"),
    }
    command = [
        sys.executable,
        "-S",
        str(python_env.project_path() / "setup/install_dependencies"),
        flag,
        "--platform",
        "debian",
    ]
    # Preview Python setup without querying host package managers.
    if flag == "--dry-run":
        command.append("--skip-system-packages")
    result = subprocess.run(command, capture_output=True, text=True, env=environment)
    assert result.returncode == 0, result.stderr
    assert not (tmp_path / "absent").exists()
