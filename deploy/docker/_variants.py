"""Compatibility import for the standalone Docker entrypoints."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from qgc_tools.docker_variants import VARIANTS_JSON, DockerVariant, load_variants

__all__ = ["VARIANTS_JSON", "DockerVariant", "load_variants"]
