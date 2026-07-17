"""Cross-platform contracts for the default-application opener."""

from __future__ import annotations

import os
from typing import TYPE_CHECKING
from unittest.mock import Mock

from common.opener import open_in_default_app

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_posix_openers_and_missing_linux_opener(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    calls: list[list[str]] = []
    monkeypatch.setattr("common.opener.subprocess.run", lambda cmd, check: calls.append(cmd))
    target = tmp_path / "file.html"

    for platform, value, executable, expected in (
        ("darwin", target, None, ["open", str(target)]),
        ("linux", target, "/usr/bin/xdg-open", ["/usr/bin/xdg-open", str(target)]),
        ("linux", "https://example.com", "/bin/xdg-open", ["/bin/xdg-open", "https://example.com"]),
    ):
        monkeypatch.setattr("sys.platform", platform)
        monkeypatch.setattr("common.opener.shutil.which", lambda _name, path=executable: path)
        assert open_in_default_app(value)
        assert calls.pop() == expected

    monkeypatch.setattr("common.opener.shutil.which", lambda _name: None)
    assert not open_in_default_app(target)


def test_windows_uses_startfile(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.platform", "win32")
    startfile = Mock()
    monkeypatch.setattr(os, "startfile", startfile, raising=False)
    target = tmp_path / "file.html"
    assert open_in_default_app(target)
    startfile.assert_called_once_with(str(target))
