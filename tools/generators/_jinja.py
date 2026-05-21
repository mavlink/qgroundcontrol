"""Shared Jinja2 Environment factory for QML generators."""

from __future__ import annotations

from pathlib import Path  # noqa: TC003

from jinja2 import Environment, FileSystemLoader, select_autoescape


def make_env(templates_dir: Path) -> Environment:
    """Return a configured Jinja2 Environment for QML template rendering."""
    return Environment(
        loader=FileSystemLoader(templates_dir),
        autoescape=select_autoescape(),  # CodeQL py/jinja2/autoescape-false; no-op for non-html/xml templates.
        trim_blocks=True,
        lstrip_blocks=True,
    )
