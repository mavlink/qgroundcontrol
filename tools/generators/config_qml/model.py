"""Data model and JSON loading for config QML page definitions."""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path  # noqa: TC003

from ..common.controls import (
    ActionButtonDef,
    BaseControlDef,
    DialogButtonDef,
    LinkedParamDef,
    RadioOptionDef,
    ToggleCheckboxDef,
    parse_action_button,
    parse_button,
    parse_dialog_button,
    parse_enable_checkbox,
    parse_linked_params,
    parse_radio_options,
    parse_toggle_checkbox,
)


@dataclass
class ControlDef(BaseControlDef):
    """A single control inside a config section.

    Inherits setting/label/control/showWhen/enableWhen/component/enableCheckbox/
    button from BaseControlDef; the fields below are config-specific.
    """

    param: str = ""  # vehicle parameter name via FactPanelController
    optional: bool = False  # true: param may not exist, pass false to getParameterFact
    sliderMin: str = ""  # explicit slider min override
    sliderMax: str = ""  # explicit slider max override
    options: list[RadioOptionDef] = field(default_factory=list)  # radiogroup options
    enumValues: list[tuple[int | float | str, str]] = field(
        default_factory=list
    )  # combobox: manual [(value, label), ...]
    dialogButton: DialogButtonDef | None = None  # button that opens a popup dialog
    actionButton: ActionButtonDef | None = None  # standalone button calling a method
    warning: bool = False  # label: use warning color
    raw: bool = False  # radiogroup: use rawValue instead of value
    bitMask: int = 0  # bitmaskCheckbox: the bitmask value
    firstEntryIsAll: bool = False  # bitmask: first entry is "all" toggle
    toggleCheckbox: ToggleCheckboxDef | None = None  # toggleCheckbox: custom checked/onClicked
    indent: bool = False  # indent control with left margin
    smallFont: bool = False  # label: use small font size
    description: str = ""  # factslider: help text above slider
    sliderFrom: str = ""  # factslider: min override
    sliderTo: str = ""  # factslider: max override
    majorTickStepSize: str = ""  # factslider: tick interval
    decimalPlaces: str = ""  # factslider: decimal places
    linkedParams: list[LinkedParamDef] = field(default_factory=list)  # factslider: coupled params


@dataclass
class DisabledSectionDef:
    """Config for the companion section showing disabled repeat items."""

    heading: str = ""  # heading text for the disabled section
    enabledParamValue: str = (
        ""  # binding name for value to set when enabling (empty = use combobox)
    )


@dataclass
class RepeatDef:
    """Repeat a section for each indexed parameter instance."""

    paramPrefix: str = ""  # e.g. "BAT"
    probePostfix: str = ""  # e.g. "_SOURCE" — used to discover count
    startIndex: int = 1
    firstIndexOmitsNumber: bool = False  # when True, index 1 -> "BAT" not "BAT1"
    indexing: str = ""  # custom indexing mode, e.g. "apm_battery"
    enableParam: str = ""  # param that must be != disabledParamValue for section to show
    disabledParamValue: str = (
        ""  # binding name for the "disabled" value (required when enableParam is set)
    )
    disabledSection: DisabledSectionDef | None = None  # companion section for disabled items


@dataclass
class SectionDef:
    """A section within a config page — either generated or a component escape."""

    title: str = ""
    image: str = ""  # qrc path for section icon
    controls: list[ControlDef] = field(default_factory=list)
    component: str = ""  # escape hatch: hand-written QML component name
    showWhen: str = ""
    repeat: RepeatDef | None = None  # repeat for indexed params
    keywords: list[str] = field(default_factory=list)  # extra search terms for section filtering


@dataclass
class ParamDef:
    """A parameter fact lookup declaration."""

    name: str  # vehicle parameter name (e.g. "FS_GCS_ENABLE")
    required: bool = (
        False  # True = getParameterFact(-1, name), False = getParameterFact(-1, name, false)
    )
    existsOnly: bool = False  # True = parameterExists(-1, name) — boolean check, not a fact


@dataclass
class PageDef:
    """Top-level page definition loaded from JSON."""

    json_filename: str = ""  # e.g. "Power.VehicleConfig.json" — used as translation context
    constants: dict[str, int | float | str] = field(
        default_factory=dict
    )  # name -> literal value (readonly)
    params: dict[str, ParamDef] = field(default_factory=dict)  # name -> param fact lookup
    bindings: dict[str, str] = field(default_factory=dict)  # name -> QML expression
    sections: list[SectionDef] = field(default_factory=list)
    imports: list[str] = field(default_factory=list)  # extra QML import lines
    controllerType: str = "FactPanelController"  # QML type for the controller


def _vis_expr(show_when: str, optional: bool, param: str, fact_ref: str) -> str:
    """Return the QML visibility expression for a control.

    When both a fact-existence guard and an explicit showWhen are needed, they
    are composed with && so the control is only visible when the param exists
    *and* the showWhen condition is met.
    """
    fact_guard = f"{fact_ref} !== null" if optional and param else ""
    if fact_guard and show_when:
        return f"{fact_guard} && ({show_when})"
    return show_when or fact_guard


def _parse_enum_value_entry(entry: object, ctrl_name: str) -> tuple[int | float | str, str]:
    """Validate and unpack a single enumValues entry, giving a clear error on bad JSON."""
    if not isinstance(entry, dict):
        raise ValueError(
            f"enumValues entry must be a JSON object in control '{ctrl_name}', got: {entry!r}"
        )
    if "value" not in entry:
        raise ValueError(f"enumValues entry missing 'value' key in control '{ctrl_name}': {entry}")
    if "label" not in entry:
        raise ValueError(f"enumValues entry missing 'label' key in control '{ctrl_name}': {entry}")
    value = entry["value"]
    label = entry["label"]
    # bool is a subclass of int in Python — reject it explicitly to catch JSON true/false
    if isinstance(value, bool) or not isinstance(value, (int, float, str)):
        raise ValueError(
            f"enumValues 'value' must be a number or string in control '{ctrl_name}', got: {value!r}"
        )
    if not isinstance(label, str):
        raise ValueError(
            f"enumValues 'label' must be a string in control '{ctrl_name}', got: {label!r}"
        )
    return (value, label)


def load_page_def(json_path: Path) -> PageDef:
    """Load a config page definition from a JSON file."""
    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)
    return _build_page_def(data, json_path.name)


def _build_page_def(data: dict, json_filename: str) -> PageDef:
    """Build a PageDef from already-parsed JSON data (no I/O)."""
    constants = data.get("constants", {})
    params: dict[str, ParamDef] = {}
    for pname, pval in data.get("params", {}).items():
        if isinstance(pval, str):
            params[pname] = ParamDef(name=pval)
        else:
            params[pname] = ParamDef(
                name=pval["name"],
                required=pval.get("required", False),
                existsOnly=pval.get("existsOnly", False),
            )
    bindings = data.get("bindings", {})
    extra_imports = data.get("imports", [])
    sections: list[SectionDef] = []
    for sec_data in data.get("sections", []):
        controls: list[ControlDef] = []
        for ctrl_data in sec_data.get("controls", []):
            controls.append(
                ControlDef(
                    param=ctrl_data.get("param", ""),
                    setting=ctrl_data.get("setting", ""),
                    label=ctrl_data.get("label", ""),
                    control=ctrl_data.get("control", ""),
                    showWhen=ctrl_data.get("showWhen", ""),
                    enableWhen=ctrl_data.get("enableWhen", ""),
                    optional=ctrl_data.get("optional", False),
                    sliderMin=str(ctrl_data.get("sliderMin", "")),
                    sliderMax=str(ctrl_data.get("sliderMax", "")),
                    enableCheckbox=parse_enable_checkbox(ctrl_data.get("enableCheckbox")),
                    button=parse_button(ctrl_data.get("button")),
                    options=parse_radio_options(ctrl_data.get("options")),
                    enumValues=[
                        (
                            _parse_enum_value_entry(
                                entry, ctrl_data.get("param") or ctrl_data.get("label", "")
                            )
                        )
                        for entry in (ctrl_data.get("enumValues") or [])
                    ],
                    dialogButton=parse_dialog_button(ctrl_data.get("dialogButton")),
                    actionButton=parse_action_button(ctrl_data.get("actionButton")),
                    warning=ctrl_data.get("warning", False),
                    raw=ctrl_data.get("raw", False),
                    bitMask=ctrl_data.get("bitMask", 0),
                    firstEntryIsAll=ctrl_data.get("firstEntryIsAll", False),
                    toggleCheckbox=parse_toggle_checkbox(ctrl_data.get("toggleCheckbox")),
                    indent=ctrl_data.get("indent", False),
                    smallFont=ctrl_data.get("smallFont", False),
                    description=ctrl_data.get("description", ""),
                    sliderFrom=str(ctrl_data.get("sliderFrom", "")),
                    sliderTo=str(ctrl_data.get("sliderTo", "")),
                    majorTickStepSize=str(ctrl_data.get("majorTickStepSize", "")),
                    decimalPlaces=str(ctrl_data.get("decimalPlaces", "")),
                    linkedParams=parse_linked_params(ctrl_data.get("linkedParams")),
                    component=ctrl_data.get("component", ""),
                )
            )
        repeat_data = sec_data.get("repeat")
        repeat_def = None
        if repeat_data:
            ds_data = repeat_data.get("disabledSection")
            ds_def = None
            if ds_data:
                ds_def = DisabledSectionDef(
                    heading=ds_data.get("heading", ""),
                    enabledParamValue=str(ds_data.get("enabledParamValue", "")),
                )
            repeat_def = RepeatDef(
                paramPrefix=repeat_data.get("paramPrefix", ""),
                probePostfix=repeat_data.get("probePostfix", ""),
                startIndex=repeat_data.get("startIndex", 1),
                firstIndexOmitsNumber=repeat_data.get("firstIndexOmitsNumber", False),
                indexing=repeat_data.get("indexing", ""),
                enableParam=repeat_data.get("enableParam", ""),
                disabledParamValue=str(repeat_data.get("disabledParamValue", "")),
                disabledSection=ds_def,
            )
            if repeat_def.enableParam and not repeat_def.disabledParamValue:
                raise ValueError(
                    f"Section '{sec_data.get('title', '')}': enableParam requires "
                    f"disabledParamValue to be specified"
                )
        sections.append(
            SectionDef(
                title=sec_data.get("title", ""),
                image=sec_data.get("image", ""),
                controls=controls,
                component=sec_data.get("component", ""),
                showWhen=sec_data.get("showWhen", ""),
                repeat=repeat_def,
                keywords=sec_data.get("keywords", []),
            )
        )
    return PageDef(
        json_filename=json_filename,
        constants=constants,
        params=params,
        bindings=bindings,
        sections=sections,
        imports=extra_imports,
        controllerType=data.get("controllerType", "FactPanelController"),
    )


def _propagate_optional(page: PageDef) -> None:
    """Mark controls as optional when their param is not required in the params section.

    Non-required params use ``getParameterFact(-1, name, false)`` which returns
    null when the param doesn't exist.  Controls referencing such params must
    also use the optional form so the inline ``fact:`` binding doesn't error
    on vehicle types that lack the parameter.
    """
    required_params: set[str] = set()
    for p in page.params.values():
        if p.required:
            required_params.add(p.name)
    for sec in page.sections:
        for ctrl in sec.controls:
            if ctrl.param and ctrl.param not in required_params:
                ctrl.optional = True


def _build_search_terms(page: PageDef) -> dict[str, list[str]]:
    """Build a mapping of section title → list of lowercase search terms.

    Search terms are derived from: section title words, control labels,
    referenced param names, and explicit keywords from JSON.
    These are always matched in English (lowercased).
    """
    result: dict[str, list[str]] = {}
    for sec in page.sections:
        terms: set[str] = set()
        # Title words
        for word in sec.title.lower().split():
            terms.add(word)
        # Explicit keywords
        for kw in sec.keywords:
            terms.add(kw.lower())
        for ctrl in sec.controls:
            if ctrl.label:
                terms.add(ctrl.label.lower())
            if ctrl.param:
                terms.add(ctrl.param.lower())
        # Disabled section heading if repeat
        if sec.repeat and sec.repeat.disabledSection:
            for word in sec.repeat.disabledSection.heading.lower().split():
                terms.add(word)
        result[sec.title] = sorted(terms)
    return result


def _build_translatable_terms(page: PageDef) -> dict[str, list[str]]:
    """Build a mapping of section title → list of original-case translatable strings.

    These are passed through qsTranslate() at runtime so the search works in
    the user's language.  Param names are excluded (not translatable).
    """
    result: dict[str, list[str]] = {}
    for sec in page.sections:
        terms: set[str] = set()
        terms.add(sec.title)
        for kw in sec.keywords:
            terms.add(kw)
        for ctrl in sec.controls:
            if ctrl.label:
                terms.add(ctrl.label)
        if sec.repeat and sec.repeat.disabledSection:
            terms.add(sec.repeat.disabledSection.heading)
        result[sec.title] = sorted(terms)
    return result
