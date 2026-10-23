"""Script entry: `python3 tools/setup/install_dependencies` runs ._cli.main.

When invoked via path-to-directory (Python treats __main__.py as a top-level
module with no parent), relative imports fail. Inject the package's parent
directory onto sys.path so absolute imports resolve in both contexts:
    python3 tools/setup/install_dependencies         (path invocation)
    python3 -m setup.install_dependencies            (module invocation)

It needs only the standard library, so it runs with the host Python before tools/.venv exists.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from setup.install_dependencies._cli import main

if __name__ == "__main__":
    raise SystemExit(main())
