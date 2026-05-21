"""LSP completion providers grouped by language/file type."""

from .fact import get_fact_json_completions
from .mavlink import (
    get_case_completions,
    get_decode_completions,
    get_message_id_completions,
    is_in_switch_context,
)

__all__ = [
    "get_case_completions",
    "get_decode_completions",
    "get_fact_json_completions",
    "get_message_id_completions",
    "is_in_switch_context",
]
