"""Shared JSON schema validation helpers for the QML generators."""

from __future__ import annotations

import reprlib

# Clamp offending-value reprs so a huge JSON fragment can't balloon the error message
_repr = reprlib.Repr()
_repr.maxstring = 120
_repr.maxlist = 4
_repr.maxdict = 4
_repr.maxother = 120


def clamped_repr(value: object) -> str:
    """repr() clamped to a bounded size, for use in schema error messages."""
    return _repr.repr(value)


def reject_unknown_keys(data: object, allowed: frozenset[str], context: str, source: object) -> None:
    """Fail the build loudly on unknown keys so typos can't silently drop generated UI.

    `source` identifies the offending JSON file in the error (path or filename).
    """
    if not isinstance(data, dict):
        raise ValueError(
            f"{source}: {context} must be a JSON object, "
            f"got {type(data).__name__}: {_repr.repr(data)}"
        )
    unknown = set(data) - allowed
    if unknown:
        raise ValueError(f"{source}: unknown {context} key(s): {', '.join(sorted(unknown))}")


def require_list(value: object, context: str, source: object) -> list:
    """Require a JSON array; a clear error beats iterating an int (TypeError) or a
    string (silent per-character iteration)."""
    if not isinstance(value, list):
        raise ValueError(
            f"{source}: {context} must be a JSON array, "
            f"got {type(value).__name__}: {_repr.repr(value)}"
        )
    return value


def require_dict(value: object, context: str, source: object) -> dict:
    """Require a JSON object; fail at load time instead of deep inside the emitter."""
    if not isinstance(value, dict):
        raise ValueError(
            f"{source}: {context} must be a JSON object, "
            f"got {type(value).__name__}: {_repr.repr(value)}"
        )
    return value
