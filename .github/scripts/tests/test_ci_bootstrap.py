"""Tests for ci_bootstrap.py (compat shim for tools/_bootstrap.py)."""

from __future__ import annotations

import subprocess
import sys
import textwrap

import pytest
from _helpers import REPO_ROOT
from ci_bootstrap import ensure_tools_dir


class TestEnsureToolsDir:
    def test_resolves_tools_as_sibling_of_ancestor(self, tmp_path):
        (tmp_path / "tools").mkdir()
        deep = tmp_path / ".github" / "scripts" / "x.py"
        deep.parent.mkdir(parents=True)
        deep.touch()
        result = ensure_tools_dir(str(deep))
        assert result == tmp_path / "tools"

    def test_resolves_tools_as_ancestor(self, tmp_path):
        tools = tmp_path / "tools"
        sub = tools / "sub"
        sub.mkdir(parents=True)
        result = ensure_tools_dir(str(sub / "x.py"))
        assert result == tools

    def test_adds_to_sys_path(self, tmp_path, monkeypatch):
        (tmp_path / "tools").mkdir()
        deep = tmp_path / "pkg" / "x.py"
        deep.parent.mkdir(parents=True)
        deep.touch()
        tools_str = str(tmp_path / "tools")
        monkeypatch.setattr(sys, "path", [p for p in sys.path if p != tools_str])
        ensure_tools_dir(str(deep))
        assert tools_str in sys.path

    def test_idempotent(self, tmp_path, monkeypatch):
        (tmp_path / "tools").mkdir()
        deep = tmp_path / "pkg" / "x.py"
        deep.parent.mkdir(parents=True)
        deep.touch()
        tools_str = str(tmp_path / "tools")
        monkeypatch.setattr(sys, "path", [p for p in sys.path if p != tools_str])
        ensure_tools_dir(str(deep))
        ensure_tools_dir(str(deep))
        assert sys.path.count(tools_str) == 1

    def test_raises_when_tools_not_found(self, tmp_path):
        deep = tmp_path / "a" / "b" / "x.py"
        deep.parent.mkdir(parents=True)
        deep.touch()
        with pytest.raises(RuntimeError, match="Could not locate tools"):
            ensure_tools_dir(str(deep))

def _spawn_bootstrap(env: dict[str, str], body: str) -> subprocess.CompletedProcess:
    """Run a snippet in a subprocess with tools/ on sys.path so _bootstrap imports."""
    code = textwrap.dedent(body)
    full_env = {**env, "PYTHONDONTWRITEBYTECODE": "1"}
    return subprocess.run(
        [sys.executable, "-c", code],
        cwd=REPO_ROOT,
        env=full_env,
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    )

class TestDebugConfigure:
    """`_configure_debug()` runs at import time when QGC_CI_DEBUG is set."""

    BOOTSTRAP_IMPORT = "import sys; sys.path.insert(0, 'tools'); import _bootstrap"

    def test_disabled_by_default_leaves_faulthandler_off(self):
        result = _spawn_bootstrap(
            env={"PATH": ""},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == "disabled"

    @pytest.mark.parametrize("value", ["1", "true", "TRUE", "yes", "on"])
    def test_truthy_value_enables_faulthandler(self, value):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": value},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == "enabled"

    @pytest.mark.parametrize("value", ["", "0", "false", "no", "off", "random"])
    def test_non_truthy_value_keeps_faulthandler_off(self, value):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": value},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import faulthandler
            print('enabled' if faulthandler.is_enabled() else 'disabled')
            """,
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.strip() == "disabled"

    def test_gha_excepthook_emits_error_annotation(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert "::error::RuntimeError: boom" in result.stderr
        assert "RuntimeError: boom" in result.stderr  # plus the normal traceback

    def test_non_gha_skips_error_annotation(self):
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert "::error::" not in result.stderr
        assert "RuntimeError: boom" in result.stderr

    def test_idempotent_when_imported_twice(self):
        # Second import should not double-wrap the excepthook; only one ::error:: line.
        result = _spawn_bootstrap(
            env={"PATH": "", "QGC_CI_DEBUG": "1", "GITHUB_ACTIONS": "true"},
            body=f"""
            {self.BOOTSTRAP_IMPORT}
            import importlib, _bootstrap
            importlib.reload(_bootstrap)
            raise RuntimeError('boom')
            """,
        )
        assert result.returncode != 0
        assert result.stderr.count("::error::RuntimeError: boom") == 1
