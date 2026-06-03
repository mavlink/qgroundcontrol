"""Script entry: `python3 tools/setup/install_dependencies` runs ._cli.main.

When invoked via path-to-directory (Python treats __main__.py as a top-level
module with no parent), relative imports fail. Inject the package's parent
directory onto sys.path so absolute imports resolve in both contexts:
    python3 tools/setup/install_dependencies         (path invocation)
    python3 -m setup.install_dependencies            (module invocation)
"""

from __future__ import annotations

import sys
from pathlib import Path

_pkg_parent = Path(__file__).resolve().parents[1]
if str(_pkg_parent) not in sys.path:
    sys.path.insert(0, str(_pkg_parent))

from install_dependencies._cli import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main())
