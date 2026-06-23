"""Backwards-compat facade for the SettingsUI generator.

The implementation lives in `model` (dataclasses + JSON loader),
`metadata` (SettingsGroup.json + Q_PROPERTY discovery), and `emit`
(QML rendering). Tests and `generate_pages.py` import from here.
"""

from .emit import generate_page_qml, generate_pages_model_qml
from .metadata import (
    get_fact_type,
    has_enum_strings,
    load_settings_metadata,
    stem_to_accessor,
    valid_accessors,
)
from .model import (
    ControlDef,
    GroupDef,
    PageDef,
    load_page_def,
    parse_keywords,
    split_translated_list,
)

__all__ = [
    "ControlDef",
    "GroupDef",
    "PageDef",
    "generate_page_qml",
    "generate_pages_model_qml",
    "get_fact_type",
    "has_enum_strings",
    "load_page_def",
    "load_settings_metadata",
    "parse_keywords",
    "split_translated_list",
    "stem_to_accessor",
    "valid_accessors",
]
