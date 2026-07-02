"""Shared pytest setup for CI script tests."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

# Allow direct imports of scripts from ".github/scripts".
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))


@pytest.fixture
def gh_output(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point GITHUB_OUTPUT at a fresh temp file and return its path."""
    path = tmp_path / "gh_output"
    path.write_text("")
    monkeypatch.setenv("GITHUB_OUTPUT", str(path))
    return path
