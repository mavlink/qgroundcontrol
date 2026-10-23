"""Environment integration tests shared by the Python 3.10/3.12 CI matrix."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

import pytest
from qgc_tools import python_env
from setup.install_python import add_python3_alias, main

pytestmark = pytest.mark.windows


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
        patch("common.proc.shutil.which", return_value=None),
        pytest.raises(FileNotFoundError, match="uv not found on PATH"),
    ):
        python_env.sync_groups("scripts")


def test_tool_uses_resolved_environment_not_path(tmp_path: Path) -> None:
    tool = python_env.executable("aqt", tmp_path)
    tool.parent.mkdir(parents=True)
    tool.touch()
    with patch("qgc_tools.python_env.sync_groups", return_value=tmp_path):
        assert python_env.tool_command("aqt", "qt") == [str(tool)]


def test_new_environment_interpreter_is_resolved_by_uv(tmp_path: Path) -> None:
    def python_arg() -> str | None:
        command = run.call_args.args[0]
        return command[command.index("--python") + 1] if "--python" in command else None

    with (
        patch("qgc_tools.python_env.require_uv", return_value="uv"),
        patch("qgc_tools.python_env.run_with_retry") as run,
    ):
        python_env.sync_groups("dev", environment=tmp_path)
        assert python_arg() is None
        python_env.sync_groups("dev", environment=tmp_path, python="python3.12")
        assert python_arg() == "python3.12"
        interpreter = python_env.executable("python", tmp_path)
        interpreter.parent.mkdir(parents=True)
        interpreter.touch()
        python_env.sync_groups("dev", environment=tmp_path)
        assert python_arg() == str(interpreter)


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


@pytest.mark.parametrize("github_actions", ["true", ""])
def test_ci_setup_adds_environment_commands_to_path(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, gh_path: Path, github_actions: str
) -> None:
    monkeypatch.setenv("GITHUB_ACTIONS", github_actions)
    environment = tmp_path / "venv"
    interpreter = python_env.executable("python", environment)
    interpreter.parent.mkdir(parents=True)
    interpreter.touch()
    with patch("setup.install_python.sync_groups", return_value=environment):
        assert main(["scripts"]) == 0
    commands = interpreter.parent
    assert gh_path.read_text() == (f"{commands}\n" if github_actions else "")


@pytest.mark.parametrize(
    ("platform", "flags", "aliased"),
    [("win32", [], True), ("win32", ["--dry-run"], False), ("linux", [], False)],
)
def test_windows_setup_adds_python3_alias(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, platform: str, flags: list[str], aliased: bool
) -> None:
    monkeypatch.setattr(sys, "platform", platform)
    scripts = tmp_path / "Scripts"
    scripts.mkdir()
    (scripts / "python.exe").write_bytes(b"launcher")
    with patch("setup.install_python.sync_groups", return_value=tmp_path):
        assert main(["scripts", *flags]) == 0
    assert (scripts / "python3.exe").is_file() == aliased


def test_python3_alias_follows_a_rebuilt_launcher(tmp_path: Path) -> None:
    scripts = tmp_path / "Scripts"
    scripts.mkdir()
    (scripts / "python.exe").write_bytes(b"launcher")
    alias = add_python3_alias(tmp_path)
    with patch("setup.install_python.shutil.copy2") as copy:
        assert add_python3_alias(tmp_path) == alias
    copy.assert_not_called()
    (scripts / "python.exe").write_bytes(b"rebuilt launcher")
    add_python3_alias(tmp_path)
    assert alias.read_bytes() == b"rebuilt launcher"


@pytest.mark.skipif(sys.platform != "win32", reason="runs the Windows venv launcher")
def test_python3_alias_starts_the_environment_from_git_bash(tmp_path: Path) -> None:
    import os

    environment = tmp_path / "venv"
    python_env.sync_groups("scripts", environment=environment, python=sys.executable)
    scripts = add_python3_alias(environment).parent
    probe = "import sys; print(sys.prefix)"
    result = subprocess.run(
        [str(scripts / "python3.exe"), "-c", probe], capture_output=True, text=True
    )
    assert result.returncode == 0, result.stderr
    assert os.path.samefile(result.stdout.strip(), environment)
    git_bash = Path(os.environ.get("PROGRAMFILES", r"C:\Program Files")) / "Git/bin/bash.exe"
    if git_bash.is_file():
        result = subprocess.run(
            [str(git_bash), "-c", f"python3 -c '{probe}'"],
            capture_output=True,
            text=True,
            env={**os.environ, "PATH": f"{scripts}{os.pathsep}{os.environ['PATH']}"},
        )
        assert result.returncode == 0, result.stderr
        assert os.path.samefile(result.stdout.strip(), environment)


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
        str(python_env.project_path() / "setup/install_dependencies"),
        flag,
        "--platform",
        "debian",
    ]
    result = subprocess.run(command, capture_output=True, text=True, env=environment)
    assert result.returncode == 0, result.stderr
    assert not (tmp_path / "absent").exists()
