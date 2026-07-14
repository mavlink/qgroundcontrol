"""Environment + CI runtime detection.

Centralizes ``CI`` and ``GITHUB_ACTIONS`` checks so setup code does not
re-implement them with subtly different truth-value semantics.
"""

from __future__ import annotations

import os

__all__ = ["is_ci"]


def is_ci() -> bool:
    """True when running in any CI environment (GitHub, GitLab, generic)."""
    return _truthy(os.environ.get("CI")) or _truthy(os.environ.get("GITHUB_ACTIONS"))


def _truthy(value: str | None) -> bool:
    return value is not None and value.lower() in {"1", "true", "yes", "on"}
