"""Contracts for the CI bootstrap compatibility shim."""

from __future__ import annotations

import subprocess
import sys
import textwrap
from typing import TYPE_CHECKING

import pytest
from _helpers import REPO_ROOT
from ci_bootstrap import ensure_tools_dir

if TYPE_CHECKING:
    from pathlib import Path


def test_ensure_tools_dir_locates_and_adds_tools_once(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    tools = tmp_path / "tools"
    tools.mkdir()
    script = tmp_path / ".github" / "scripts" / "x.py"
    script.parent.mkdir(parents=True)
    script.touch()
    monkeypatch.setattr(sys, "path", [entry for entry in sys.path if entry != str(tools)])

    assert ensure_tools_dir(str(script)) == tools
    assert ensure_tools_dir(str(script)) == tools
    assert sys.path.count(str(tools)) == 1

    nested_script = tools / "sub" / "x.py"
    nested_script.parent.mkdir()
    assert ensure_tools_dir(str(nested_script)) == tools


def test_ensure_tools_dir_rejects_unrelated_tree(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("pathlib.Path.is_dir", lambda _path: False)
    with pytest.raises(RuntimeError, match="Could not locate tools"):
        ensure_tools_dir("/unrelated/a/b/x.py")


def _spawn_bootstrap(env: dict[str, str], body: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, "-c", textwrap.dedent(body)],
        cwd=REPO_ROOT,
        env={**env, "PYTHONDONTWRITEBYTECODE": "1"},
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    )


_BOOTSTRAP_IMPORT = "import sys; sys.path.insert(0, 'tools'); import _bootstrap"


def test_debug_flag_controls_faulthandler() -> None:
    cases = [(None, False)] + [(value, True) for value in ("1", "true", "TRUE", "yes", "on")]
    cases += [(value, False) for value in ("", "0", "false", "no", "off", "random")]

    for value, expected in cases:
        env = {"PATH": ""}
        if value is not None:
            env["QGC_CI_DEBUG"] = value
        result = _spawn_bootstrap(
            env,
            f"""
            {_BOOTSTRAP_IMPORT}
            import faulthandler
            print(faulthandler.is_enabled())
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == str(expected)


def test_debug_excepthook_annotates_github_once() -> None:
    cases = [
        ({"QGC_CI_DEBUG": "1"}, False, ""),
        ({"QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"}, True, ""),
        (
            {"QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            True,
            "import importlib, _bootstrap; importlib.reload(_bootstrap)",
        ),
    ]
    for extra_env, annotated, reload_code in cases:
        result = _spawn_bootstrap(
            {"PATH": "", **extra_env},
            f"""
            {_BOOTSTRAP_IMPORT}
            {reload_code}
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert "RuntimeError: boom" in result.stderr
        assert result.stderr.count("::error::RuntimeError: boom") == int(annotated)
