"""Bootstrap helper for GitHub CI scripts that import from tools/.

Compat shim: the implementation lives in ``tools/_bootstrap.py``. This module
adds the ``tools/`` directory to ``sys.path`` (so ``_bootstrap`` is importable),
then re-exports its ``ensure_tools_dir`` for callers that still do
``from ci_bootstrap import ensure_tools_dir``.
"""

from __future__ import annotations

import sys
from pathlib import Path

_TOOLS_DIR = Path(__file__).resolve().parents[2] / "tools"
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

from _bootstrap import (
    ensure_tools_dir as ensure_tools_dir,
)
