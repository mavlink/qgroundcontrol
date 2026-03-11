"""Shared import bootstrap for Python entrypoints under tools/."""

from __future__ import annotations

import sys
from pathlib import Path


def ensure_tools_dir(start: str | Path) -> Path:
    """Ensure the top-level ``tools`` directory is importable."""
    path = Path(start).resolve()
    current = path if path.is_dir() else path.parent
    for candidate in [current] + list(current.parents):
        if candidate.name == "tools":
            if str(candidate) not in sys.path:
                sys.path.insert(0, str(candidate))
            return candidate
    raise RuntimeError(f"Could not locate tools directory from {start}")
