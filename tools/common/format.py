"""Human-readable formatters for byte counts and deltas.

Replaces the three near-identical helpers that lived in
``.github/scripts/collect_artifact_sizes.py``,
``.github/scripts/generate_build_results_comment.py`` and ad-hoc
``size / 1024 / 1024`` divisions in ``download_artifacts.py`` and
``gstreamer_archive.py``.
"""

from __future__ import annotations

__all__ = ["format_bytes", "format_delta_bytes"]


def format_bytes(size_bytes: int | float) -> str:
    """Return *size_bytes* as a human-readable ``MB``/``GB`` string.

    Mirrors the historical CI helper: always uses MB up to 1024 MB,
    then switches to GB. Two decimal places, ``MB``/``GB`` suffix.
    """
    size_mb = size_bytes / 1024.0 / 1024.0
    if size_mb >= 1024:
        return f"{size_mb / 1024:.2f} GB"
    return f"{size_mb:.2f} MB"


def format_delta_bytes(delta_bytes: int | float) -> str:
    """Return a signed delta with ``(increase)``/``(decrease)``/``No change``.

    Used by build-results PR comments to summarize per-artifact size diffs.
    """
    delta_mb = delta_bytes / 1024.0 / 1024.0
    if delta_bytes > 0:
        return f"+{delta_mb:.2f} MB (increase)"
    if delta_bytes < 0:
        return f"{delta_mb:.2f} MB (decrease)"
    return "No change"
