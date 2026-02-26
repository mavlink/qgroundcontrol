"""Shared pytest setup for CI script tests."""

from __future__ import annotations

import sys
from pathlib import Path


# Allow direct imports of scripts from ".github/scripts".
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
