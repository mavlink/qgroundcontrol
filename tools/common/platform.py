"""Platform detection helpers.

Replaces ad-hoc ``sys.platform == "win32"`` / ``platform.system().lower()``
checks scattered through ``configure.py``, ``run_tests.py``,
``setup/install_dependencies/`` package, ``setup/install_python.py`` and others.
"""

from __future__ import annotations

import sys
from typing import Literal

__all__ = ["current_platform", "is_linux", "is_macos", "is_windows"]

Platform = Literal["linux", "macos", "windows", "other"]


def is_windows() -> bool:
    """True on Windows (CPython sets ``sys.platform == "win32"``)."""
    return sys.platform == "win32"


def is_macos() -> bool:
    """True on macOS."""
    return sys.platform == "darwin"


def is_linux() -> bool:
    """True on any Linux variant."""
    return sys.platform.startswith("linux")


def current_platform() -> Platform:
    """Return a normalized platform tag: ``linux`` / ``macos`` / ``windows`` / ``other``."""
    if is_windows():
        return "windows"
    if is_macos():
        return "macos"
    if is_linux():
        return "linux"
    return "other"
