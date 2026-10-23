"""Qt-specific Clazy analysis using the configured compilation database."""

import os
from typing import ClassVar

from .compiler import CompilerAnalyzer


class ClazyAnalyzer(CompilerAnalyzer):
    name: ClassVar[str] = "clazy"
    executable: ClassVar[str] = "clazy-standalone"
    install_hint: ClassVar[str] = "Install clazy matching the analysis compiler."
    CHECKS: ClassVar[str] = "level1,connect-non-signal,lambda-in-connect,overridden-signal"
    arguments: ClassVar[tuple[str, ...]] = (f"--checks={CHECKS}",)

    def _config_inputs(self) -> tuple[str, ...]:
        # Clazy also reads checks and options from CLAZY_* environment variables.
        return tuple(
            f"{name}={value}"
            for name, value in sorted(os.environ.items())
            if name.startswith("CLAZY_")
        )
