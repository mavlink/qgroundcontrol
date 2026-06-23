#!/usr/bin/env python3
"""Tests for tools/common/opener.py."""

from __future__ import annotations

from typing import TYPE_CHECKING
from unittest.mock import MagicMock

from common.opener import open_in_default_app

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_open_macos_uses_open(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "darwin")
    called: list[list[str]] = []
    monkeypatch.setattr("common.opener.subprocess.run", lambda cmd, check: called.append(cmd))
    target = tmp_path / "file.html"
    assert open_in_default_app(target) is True
    assert called == [["open", str(target)]]


def test_open_linux_uses_xdg_open(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "linux")
    monkeypatch.setattr("common.opener.shutil.which", lambda name: f"/usr/bin/{name}")
    called: list[list[str]] = []
    monkeypatch.setattr("common.opener.subprocess.run", lambda cmd, check: called.append(cmd))
    target = tmp_path / "file.html"
    assert open_in_default_app(target) is True
    assert called == [["/usr/bin/xdg-open", str(target)]]


def test_open_linux_no_opener_returns_false(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "linux")
    monkeypatch.setattr("common.opener.shutil.which", lambda name: None)
    assert open_in_default_app(tmp_path / "x") is False


def test_open_accepts_string(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "linux")
    monkeypatch.setattr("common.opener.shutil.which", lambda name: "/bin/xdg-open")
    called: list[list[str]] = []
    monkeypatch.setattr("common.opener.subprocess.run", lambda cmd, check: called.append(cmd))
    assert open_in_default_app("https://example.com") is True
    assert called == [["/bin/xdg-open", "https://example.com"]]


def test_open_windows_uses_startfile(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "win32")
    mock_startfile = MagicMock()
    import os

    monkeypatch.setattr(os, "startfile", mock_startfile, raising=False)
    target = tmp_path / "file.html"
    assert open_in_default_app(target) is True
    mock_startfile.assert_called_once_with(str(target))
