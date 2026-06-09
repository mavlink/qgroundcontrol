"""Generate QML vehicle config pages from JSON definitions.

Each page definition describes sections with controls that bind to vehicle
parameters via FactPanelController.  Sections can either list individual
controls (which are code-generated) or reference a hand-written QML
``component`` (escape-hatch for complex UIs).

This module is a re-export facade.  Implementation lives in:
  - ``.model``  — dataclasses + JSON loader
  - ``.emit``   — QML rendering
"""

from .emit import generate_config_page_qml, get_section_names
from .model import (
    ControlDef,
    DisabledSectionDef,
    PageDef,
    ParamDef,
    RepeatDef,
    SectionDef,
    load_page_def,
)

__all__ = [
    "ControlDef",
    "DisabledSectionDef",
    "PageDef",
    "ParamDef",
    "RepeatDef",
    "SectionDef",
    "generate_config_page_qml",
    "get_section_names",
    "load_page_def",
]
