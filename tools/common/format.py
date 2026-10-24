"""Human-readable formatters for byte counts, deltas, counts, durations, percentages, and names."""

from __future__ import annotations

from typing import Any

__all__ = [
    "MAX_NAME_CHARS",
    "clip",
    "format_bytes",
    "format_count",
    "format_delta_bytes",
    "format_duration",
    "format_percent",
    "format_ratio",
]


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
    """Return a signed size delta in MB (``+1.25 MB``, ``-0.50 MB``), or ``unchanged``."""
    if delta_bytes == 0:
        return "unchanged"
    sign = "+" if delta_bytes > 0 else "-"
    return f"{sign}{format_bytes(abs(delta_bytes))}"


def format_count(value: int) -> str:
    """Return an integer with thousands separators (``12,345``)."""
    return f"{value:,}"


def format_duration(seconds: float | None) -> str:
    """Return a compact duration: ``0.4s``, ``42s``, ``3m 05s``, ``1h 02m``; ``n/a`` for None."""
    if seconds is None or seconds < 0:
        return "n/a"
    if seconds < 10:
        return f"{seconds:.1f}s"
    whole = round(seconds)
    if whole < 60:
        return f"{whole}s"
    minutes, secs = divmod(whole, 60)
    if minutes < 60:
        return f"{minutes}m {secs:02d}s"
    hours, minutes = divmod(minutes, 60)
    return f"{hours}h {minutes:02d}m"


def format_percent(value: float, *, digits: int = 1) -> str:
    """Return ``value`` as a percentage string (``93.4%``)."""
    return f"{value:.{digits}f}%"


def format_ratio(part: int, total: int) -> str:
    """Return ``part of total (percent)``, e.g. ``1,234 of 1,300 (94.9%)``; ``0 of 0`` when empty."""
    if total <= 0:
        return f"{format_count(part)} of {format_count(total)}"
    return f"{format_count(part)} of {format_count(total)} ({format_percent(100.0 * part / total)})"


# Names from pull requests and build outputs (tests, jobs, files) are clipped wherever shown.
MAX_NAME_CHARS = 120


def clip(value: Any, limit: int = MAX_NAME_CHARS) -> str:
    """Return *value* on one line, shortened to *limit* characters."""
    text = " ".join(str(value).split())
    return text if len(text) <= limit else f"{text[: limit - 1]}…"
