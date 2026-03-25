"""Tests for ci_bootstrap.py."""

from __future__ import annotations

import sys

from ci_bootstrap import ensure_tools_dir


class TestEnsureToolsDir:
    """Tests for ensure_tools_dir function."""

    def test_returns_tools_dir(self, tmp_path):
        """Returned path ends with 'tools'."""
        # ensure_tools_dir resolves parents[2] as repo root
        # So we need start to be 3 levels deep: repo/a/b/c
        deep = tmp_path / "a" / "b" / "c"
        deep.mkdir(parents=True)
        result = ensure_tools_dir(str(deep))
        assert result.name == "tools"
        assert result == tmp_path / "tools"

    def test_adds_to_sys_path(self, tmp_path, monkeypatch):
        """tools dir appears in sys.path after call."""
        deep = tmp_path / "a" / "b" / "c"
        deep.mkdir(parents=True)
        tools_str = str(tmp_path / "tools")
        # Remove if already present
        monkeypatch.setattr(sys, "path", [p for p in sys.path if p != tools_str])
        ensure_tools_dir(str(deep))
        assert tools_str in sys.path

    def test_idempotent(self, tmp_path, monkeypatch):
        """Calling twice doesn't duplicate sys.path entry."""
        deep = tmp_path / "a" / "b" / "c"
        deep.mkdir(parents=True)
        tools_str = str(tmp_path / "tools")
        monkeypatch.setattr(sys, "path", [p for p in sys.path if p != tools_str])
        ensure_tools_dir(str(deep))
        ensure_tools_dir(str(deep))
        assert sys.path.count(tools_str) == 1
