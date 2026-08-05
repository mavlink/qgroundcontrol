"""Shared helpers for tools tests."""

from __future__ import annotations

import importlib.util
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from types import ModuleType

REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_DIR = REPO_ROOT / "tools"


def completed(
    stdout: str = "", returncode: int = 0, stderr: str = ""
) -> subprocess.CompletedProcess:
    """Fake subprocess.CompletedProcess for stubbing external commands."""
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr=stderr)


def load_script_module(relpath: str, name: str) -> ModuleType:
    """Import a tools/ script that isn't reachable as a package module (hyphenated or path-loaded)."""
    spec = importlib.util.spec_from_file_location(name, TOOLS_DIR / relpath)
    assert spec and spec.loader
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod
