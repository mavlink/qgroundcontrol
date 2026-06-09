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

if TYPE_CHECKING:
    from .server import QGCLanguageServer as QGCLanguageServer
    from .server import server as server

__all__ = ["QGCLanguageServer", "server"]


def __getattr__(name: str):
    """Lazy import to avoid requiring pygls until actually used."""
    if name in ("QGCLanguageServer", "server"):
        from .server import QGCLanguageServer, server
        return QGCLanguageServer if name == "QGCLanguageServer" else server
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
