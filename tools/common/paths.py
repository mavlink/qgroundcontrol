"""Locations of the tools/ directory and the repository that contains it."""

from __future__ import annotations

from pathlib import Path

__all__ = ["REPO_ROOT", "TOOLS_DIR"]

TOOLS_DIR = Path(__file__).resolve().parents[1]
REPO_ROOT = TOOLS_DIR.parent
