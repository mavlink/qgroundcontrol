"""Bootstrap helper for GitHub CI scripts that import from tools/."""

from __future__ import annotations

import sys
from pathlib import Path


def ensure_tools_dir(start: str | Path) -> Path:
    """Ensure the repository's tools directory is importable."""
    path = Path(start).resolve()
    repo_root = path.parents[2]
    tools_dir = repo_root / "tools"
    if str(tools_dir) not in sys.path:
        sys.path.insert(0, str(tools_dir))
    return tools_dir
