"""Script entry: `python3 tools/setup/install_dependencies` runs ._cli.main.

When invoked via path-to-directory (Python treats __main__.py as a top-level
module with no parent), relative imports fail. Inject the package's parent
directory onto sys.path so absolute imports resolve in both contexts:
    python3 tools/setup/install_dependencies         (path invocation)
    python3 -m setup.install_dependencies            (module invocation)
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

_tools = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(_tools))

from qgc_tools.python_env import executable, sync_groups  # noqa: E402

if __name__ == "__main__":
    if not any(
        arg in sys.argv[1:]
        for arg in (
            "--help",
            "-h",
            "--list",
            "--dry-run",
            "--print-packages",
            "--print-available-packages",
            "--validate-extra-packages",
        )
    ):
        environment = sync_groups("scripts")
        if Path(sys.prefix).resolve() != environment.resolve():
            raise SystemExit(
                subprocess.run(
                    [str(executable("python", environment)), __file__, *sys.argv[1:]]
                ).returncode
            )
    from setup.install_dependencies._cli import main

    raise SystemExit(main())
