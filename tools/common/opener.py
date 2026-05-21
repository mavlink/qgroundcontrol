"""Open a file or URL in the user's default app.

Wraps the ``xdg-open`` (Linux) / ``open`` (macOS) / ``start`` (Windows)
dispatch that was duplicated in ``coverage.py`` and ``generate_docs.py``.
"""

from __future__ import annotations

import shutil
import subprocess
from typing import TYPE_CHECKING

from .platform import is_macos, is_windows

if TYPE_CHECKING:
    from pathlib import Path

__all__ = ["open_in_default_app"]


def open_in_default_app(target: Path | str) -> bool:
    """Open *target* in the user's default app.

    Returns ``True`` on a successful dispatch, ``False`` if no opener was
    found. The subprocess itself is fire-and-forget — no exit-code check.
    """
    path = str(target)

    if is_windows():
        # os.startfile is the canonical Windows opener but isn't on POSIX builds.
        import os

        startfile = getattr(os, "startfile", None)
        if startfile is None:
            return False
        startfile(path)
        return True

    opener = "open" if is_macos() else shutil.which("xdg-open")
    if opener is None:
        return False

    subprocess.run([opener, path], check=False)
    return True
