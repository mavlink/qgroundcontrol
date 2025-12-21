"""QGroundControl Language Server.

Provides IDE integration for QGC-specific patterns:
- Vehicle null-check diagnostics
- Parameter access validation
- MAVLink message completion

Usage:
    python -m tools.lsp          # Start server in STDIO mode
    python -m tools.lsp --tcp    # Start server in TCP mode for debugging

Requirements:
    pip install pygls lsprotocol
"""

from typing import TYPE_CHECKING

# Lazy imports - actual loading happens in __getattr__
# These TYPE_CHECKING imports make names visible to static analyzers
if TYPE_CHECKING:
    from .mavlink_data import MAVLINK_MESSAGES as MAVLINK_MESSAGES
    from .server import QGCLanguageServer as QGCLanguageServer
    from .server import server as server

__all__ = ["QGCLanguageServer", "server", "MAVLINK_MESSAGES"]


def __getattr__(name: str):
    """Lazy import to avoid requiring pygls until actually used."""
    if name in ("QGCLanguageServer", "server"):
        from .server import QGCLanguageServer, server
        return QGCLanguageServer if name == "QGCLanguageServer" else server
    if name == "MAVLINK_MESSAGES":
        from .mavlink_data import MAVLINK_MESSAGES
        return MAVLINK_MESSAGES
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
