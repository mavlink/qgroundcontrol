"""Environment + CI runtime detection.

Centralizes ``os.environ.get("CI")`` / ``GITHUB_ACTIONS`` / ``GITHUB_EVENT_NAME``
checks so each tool doesn't re-implement them with subtly different semantics.
"""

from __future__ import annotations

import os
import platform

__all__ = [
    "is_ci",
    "is_debug",
    "is_github_actions",
    "is_pr_event",
    "is_verbose",
    "runner_arch",
]


def is_ci() -> bool:
    """True when running in any CI environment (GitHub, GitLab, generic)."""
    return _truthy(os.environ.get("CI")) or is_github_actions()


def is_github_actions() -> bool:
    """True when running inside a GitHub Actions runner."""
    return _truthy(os.environ.get("GITHUB_ACTIONS"))


def is_pr_event() -> bool:
    """True when the current GitHub Actions event is a pull_request."""
    return os.environ.get("GITHUB_EVENT_NAME") == "pull_request"


def runner_arch() -> str:
    """Return the CI runner architecture (``RUNNER_ARCH`` if set, else local machine)."""
    return os.environ.get("RUNNER_ARCH") or platform.machine()


def is_debug() -> bool:
    """True when ``DEBUG`` env var is set to a truthy value."""
    return _truthy(os.environ.get("DEBUG"))


def is_verbose() -> bool:
    """True when ``VERBOSE`` env var is set to a truthy value."""
    return _truthy(os.environ.get("VERBOSE"))


def _truthy(value: str | None) -> bool:
    return value is not None and value.lower() in {"1", "true", "yes", "on"}
