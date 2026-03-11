"""Bootstrap helper for entrypoints under tools/setup."""

from __future__ import annotations

import sys
from pathlib import Path


def ensure_setup_imports() -> Path:
    """Ensure the parent tools directory is importable."""
    tools_dir = Path(__file__).resolve().parents[1]
    if str(tools_dir) not in sys.path:
        sys.path.insert(0, str(tools_dir))
    return tools_dir
