"""
MAVLink message metadata for LSP completion.

Loads message definitions from the project's MAVLink XML at runtime. When no
project root has been resolved yet, returns an empty list — the LSP surfaces
"build the project once to enable MAVLink completions" rather than serving a
stale hardcoded subset.
"""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path  # noqa: TC003

logger = logging.getLogger(__name__)


@dataclass
class MAVLinkField:
    name: str
    type: str
    description: str = ""
    units: str = ""
    enum: str = ""


@dataclass
class MAVLinkMessage:
    name: str
    id: int
    description: str
    fields: list[MAVLinkField] = field(default_factory=list)
    category: str = ""


_dynamic_messages: list[MAVLinkMessage] | None = None
_dynamic_loaded = False


def load_messages_from_xml(project_root: Path) -> list[MAVLinkMessage]:
    """Load MAVLink messages from XML; return [] when XML or parser is unavailable."""
    try:
        from .mavlink_parser import load_all_messages

        return load_all_messages(project_root)
    except ImportError:
        logger.debug("mavlink_parser unavailable (missing optional dependency)")
        return []
    except Exception as e:
        logger.warning(f"Failed to load MAVLink XML: {e}")
        return []


def get_all_messages(project_root: Path | None = None) -> list[MAVLinkMessage]:
    """Return cached XML-loaded messages, or load once if a project root is provided."""
    global _dynamic_messages, _dynamic_loaded

    if project_root is not None and not _dynamic_loaded:
        _dynamic_loaded = True
        _dynamic_messages = load_messages_from_xml(project_root)
        if _dynamic_messages:
            logger.info(f"Using {len(_dynamic_messages)} messages from MAVLink XML")
        else:
            logger.info(
                "MAVLink completions unavailable: build the project once to populate "
                "build/cpm_modules/mavlink/.../message_definitions/"
            )

    return _dynamic_messages or []


def reset_dynamic_messages() -> None:
    """Reset dynamic message cache (for testing)."""
    global _dynamic_messages, _dynamic_loaded
    _dynamic_messages = None
    _dynamic_loaded = False


def get_message_by_name(name: str, project_root: Path | None = None) -> MAVLinkMessage | None:
    """Get a message by name, or None if no XML messages are loaded."""
    target = name.upper()
    for msg in get_all_messages(project_root):
        if msg.name == target:
            return msg
    return None
