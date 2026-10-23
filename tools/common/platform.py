"""OS checks and CPU architecture normalization."""

from __future__ import annotations

import os
import platform
import sys
from typing import Literal

__all__ = [
    "host_arch",
    "is_linux",
    "is_macos",
    "is_windows",
    "normalize_arch",
]

Architecture = Literal["x86_64", "aarch64"]

_ARCH_ALIASES: dict[str, Architecture] = {
    "x86_64": "x86_64",
    "amd64": "x86_64",
    "x64": "x86_64",
    "aarch64": "aarch64",
    "arm64": "aarch64",
}


def normalize_arch(value: str) -> Architecture:
    """Normalize common local and GitHub runner architecture spellings."""
    normalized = value.strip().lower()
    try:
        return _ARCH_ALIASES[normalized]
    except KeyError as exc:
        raise ValueError(f"Unsupported architecture: {value or '(empty)'}") from exc


def host_arch() -> Architecture:
    """Return the normalized CI runner or local host architecture."""
    return normalize_arch(os.environ.get("RUNNER_ARCH") or platform.machine())


def is_windows() -> bool:
    """True on Windows (CPython sets ``sys.platform == "win32"``)."""
    return sys.platform == "win32"


def is_macos() -> bool:
    """True on macOS."""
    return sys.platform == "darwin"


def is_linux() -> bool:
    """True on any Linux variant."""
    return sys.platform.startswith("linux")
