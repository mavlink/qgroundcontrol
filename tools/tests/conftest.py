"""Shared pytest setup for tools tests."""

from __future__ import annotations

import sys
from pathlib import Path

# Ensure the tools directory is importable (for common.*, setup.*, etc.)
tools_dir = str(Path(__file__).resolve().parent.parent)
if tools_dir not in sys.path:
    sys.path.insert(0, tools_dir)
