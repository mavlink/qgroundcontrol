"""Generate QML vehicle config pages from JSON definitions.

Each page definition describes sections with controls that bind to vehicle
parameters via FactPanelController.  Sections can either list individual
controls (which are code-generated) or reference a hand-written QML
``component`` (escape-hatch for complex UIs).

This module is a re-export facade.  Implementation lives in:
  - ``.model``  — dataclasses + JSON loader
  - ``.emit``   — QML rendering
"""

from .emit import (
    _HEADER,
    _detect_control_type,
    _fact_ref,
    _inject_prop,
    _qml_component_section,
    _qml_control,
    _qml_disabled_companion_section,
    _qml_generated_section,
    _qml_repeat_count_property,
    _qml_repeat_section,
    _safe_id,
    _wrap_visible,
    generate_config_page_qml,
    get_section_names,
)
from .model import (
    ControlDef,
    DisabledSectionDef,
    PageDef,
    ParamDef,
    RepeatDef,
    SectionDef,
    _build_search_terms,
    _build_translatable_terms,
    _propagate_optional,
    _vis_expr,
    load_page_def,
)

__all__ = [
    "_HEADER",
    "ControlDef",
    "DisabledSectionDef",
    "PageDef",
    "ParamDef",
    "RepeatDef",
    "SectionDef",
    "_build_search_terms",
    "_build_translatable_terms",
    "_detect_control_type",
    "_fact_ref",
    "_inject_prop",
    "_propagate_optional",
    "_qml_component_section",
    "_qml_control",
    "_qml_disabled_companion_section",
    "_qml_generated_section",
    "_qml_repeat_count_property",
    "_qml_repeat_section",
    "_safe_id",
    "_vis_expr",
    "_wrap_visible",
    "generate_config_page_qml",
    "get_section_names",
    "load_page_def",
]
