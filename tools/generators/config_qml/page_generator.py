"""Generate QML vehicle config pages from JSON definitions.

Each page definition describes sections with controls that bind to vehicle
parameters via FactPanelController.  Sections can either list individual
controls (which are code-generated) or reference a hand-written QML
``component`` (escape-hatch for complex UIs).
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path

from ..common.controls import (
    EnableCheckboxDef,
    ButtonDef,
    DialogButtonDef,
    ActionButtonDef,
    RadioOptionDef,
    ToggleCheckboxDef,
    LinkedParamDef,
    parse_enable_checkbox,
    parse_button,
    parse_dialog_button,
    parse_action_button,
    parse_radio_options,
    parse_toggle_checkbox,
    parse_linked_params,
    qml_tr,
    render_label,
    render_slider,
    render_checkbox,
    render_combobox,
    render_textfield,
    render_radiogroup,
    render_dialog_button,
    render_action_button,
    render_bitmask_checkbox,
    render_bitmask,
    render_toggle_checkbox,
    render_factslider,
)


# --------------------------------------------------------------------------- #
# Data model
# --------------------------------------------------------------------------- #

@dataclass
class ControlDef:
    """A single control inside a config section."""
    param: str = ""            # vehicle parameter name via FactPanelController
    setting: str = ""          # settings path, e.g. "flyViewSettings.showObstacleDistanceOverlay"
    label: str = ""
    control: str = ""          # combobox | textfield | checkbox | slider | dialogButton (auto-detected if empty)
    showWhen: str = ""
    enableWhen: str = ""
    optional: bool = False     # true: param may not exist, pass false to getParameterFact
    sliderMin: str = ""        # explicit slider min override
    sliderMax: str = ""        # explicit slider max override
    enableCheckbox: EnableCheckboxDef | None = None  # slider: checked/onClicked
    button: ButtonDef | None = None                  # adjacent button
    options: list[RadioOptionDef] = field(default_factory=list)  # radiogroup options
    dialogButton: DialogButtonDef | None = None      # button that opens a popup dialog
    actionButton: ActionButtonDef | None = None       # standalone button calling a method
    warning: bool = False                            # label: use warning color
    raw: bool = False                                # radiogroup: use rawValue instead of value
    bitMask: int = 0                                 # bitmaskCheckbox: the bitmask value
    firstEntryIsAll: bool = False                    # bitmask: first entry is "all" toggle
    toggleCheckbox: ToggleCheckboxDef | None = None  # toggleCheckbox: custom checked/onClicked
    indent: bool = False                              # indent control with left margin
    smallFont: bool = False                           # label: use small font size
    description: str = ""                             # factslider: help text above slider
    sliderFrom: str = ""                              # factslider: min override
    sliderTo: str = ""                                # factslider: max override
    majorTickStepSize: str = ""                       # factslider: tick interval
    decimalPlaces: str = ""                           # factslider: decimal places
    linkedParams: list[LinkedParamDef] = field(default_factory=list)  # factslider: coupled params
    component: str = ""                               # component: inline hand-written QML component


@dataclass
class DisabledSectionDef:
    """Config for the companion section showing disabled repeat items."""
    heading: str = ""                # heading text for the disabled section
    enabledParamValue: str = ""      # binding name for value to set when enabling (empty = use combobox)


@dataclass
class RepeatDef:
    """Repeat a section for each indexed parameter instance."""
    paramPrefix: str = ""            # e.g. "BAT"
    probePostfix: str = ""           # e.g. "_SOURCE" — used to discover count
    startIndex: int = 1
    firstIndexOmitsNumber: bool = False  # when True, index 1 -> "BAT" not "BAT1"
    indexing: str = ""               # custom indexing mode, e.g. "apm_battery"
    enableParam: str = ""            # param that must be != disabledParamValue for section to show
    disabledParamValue: str = ""     # binding name for the "disabled" value (required when enableParam is set)
    disabledSection: DisabledSectionDef | None = None  # companion section for disabled items


@dataclass
class SectionDef:
    """A section within a config page — either generated or a component escape."""
    title: str = ""
    image: str = ""            # qrc path for section icon
    controls: list[ControlDef] = field(default_factory=list)
    component: str = ""        # escape hatch: hand-written QML component name
    showWhen: str = ""
    repeat: RepeatDef | None = None  # repeat for indexed params
    keywords: list[str] = field(default_factory=list)  # extra search terms for section filtering


@dataclass
class ParamDef:
    """A parameter fact lookup declaration."""
    name: str           # vehicle parameter name (e.g. "FS_GCS_ENABLE")
    required: bool = False   # True = getParameterFact(-1, name), False = getParameterFact(-1, name, false)
    existsOnly: bool = False # True = parameterExists(-1, name) — boolean check, not a fact


@dataclass
class PageDef:
    """Top-level page definition loaded from JSON."""
    json_filename: str = ""    # e.g. "Power.VehicleConfig.json" — used as translation context
    constants: dict[str, int | float | str] = field(default_factory=dict)  # name -> literal value (readonly)
    params: dict[str, ParamDef] = field(default_factory=dict)  # name -> param fact lookup
    bindings: dict[str, str] = field(default_factory=dict)   # name -> QML expression
    sections: list[SectionDef] = field(default_factory=list)
    imports: list[str] = field(default_factory=list)  # extra QML import lines
    controllerType: str = "FactPanelController"  # QML type for the controller


# --------------------------------------------------------------------------- #
# JSON loading
# --------------------------------------------------------------------------- #

def load_page_def(json_path: Path) -> PageDef:
    """Load a config page definition from a JSON file."""
    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)

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
            controls.append(ControlDef(
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
            ))
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
            if repeat_def.enableParam:
                if not repeat_def.disabledParamValue:
                    raise ValueError(
                        f"Section '{sec_data.get('title', '')}': enableParam requires "
                        f"disabledParamValue to be specified"
                    )
        sections.append(SectionDef(
            title=sec_data.get("title", ""),
            image=sec_data.get("image", ""),
            controls=controls,
            component=sec_data.get("component", ""),
            showWhen=sec_data.get("showWhen", ""),
            repeat=repeat_def,
            keywords=sec_data.get("keywords", []),
        ))
    return PageDef(json_filename=json_path.name, constants=constants, params=params, bindings=bindings, sections=sections, imports=extra_imports,
                   controllerType=data.get("controllerType", "FactPanelController"))


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
        # Control labels
        for ctrl in sec.controls:
            if ctrl.label:
                terms.add(ctrl.label.lower())
        # Param names (the param key from JSON, lowercased)
        for ctrl in sec.controls:
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


# --------------------------------------------------------------------------- #
# QML generation helpers
# --------------------------------------------------------------------------- #

_HEADER = """\
// This file is auto-generated. Do not edit.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
"""


def _detect_control_type(ctrl: ControlDef) -> str:
    """Detect the best control type for a parameter when not explicitly set."""
    if ctrl.control:
        return ctrl.control
    # Default to textfield; combobox/checkbox can be specified explicitly
    return "textfield"


def _fact_ref(ctrl: ControlDef, indexed: bool = False) -> str:
    """Return the QML expression for the control's fact binding.

    When *indexed* is True the control lives inside a Repeater delegate and
    ``ctrl.param`` is a postfix (e.g. ``_SOURCE``).  The full param name is
    built at runtime from ``_fullParamName`` which is a JS function defined
    by the repeat section.
    """
    if ctrl.setting:
        return f"QGroundControl.settingsManager.{ctrl.setting}"
    if indexed:
        if ctrl.optional:
            return f'controller.getParameterFact(-1, _fullParamName("{ctrl.param}"), false)'
        return f'controller.getParameterFact(-1, _fullParamName("{ctrl.param}"))'
    if ctrl.optional:
        return f'controller.getParameterFact(-1, "{ctrl.param}", false)'
    return f'controller.getParameterFact(-1, "{ctrl.param}")'


def _qml_control(ctrl: ControlDef, indent: str, *, indexed: bool = False, dialog_counter: list[int] | None = None, tr_context: str = "") -> str:
    """Generate QML for a single control inside a ConfigSection ColumnLayout."""
    fact_ref = _fact_ref(ctrl, indexed=indexed)
    control_type = _detect_control_type(ctrl)

    def _apply_indent(qml: str) -> str:
        """Wrap output with Layout.leftMargin when ctrl.indent is set."""
        if ctrl.indent:
            qml = _inject_prop(qml, f"{indent}    Layout.leftMargin: ScreenTools.defaultFontPixelWidth * 2")
        return qml

    # Inline component escape hatch — hand-written QML component
    if control_type == "component" and ctrl.component:
        lines: list[str] = []
        lines.append(f"{indent}{ctrl.component} {{")
        lines.append(f"{indent}    controller: controller")
        lines.append(f"{indent}    Layout.fillWidth: true")
        if ctrl.showWhen:
            lines.append(f"{indent}    visible: {ctrl.showWhen}")
        lines.append(f"{indent}}}")
        return _apply_indent("\n".join(lines))

    # Static label — no fact binding
    if control_type == "label":
        qml = render_label(
            indent,
            text=ctrl.label,
            warning=ctrl.warning,
            small_font=ctrl.smallFont,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Dialog button — standalone button that opens a popup dialog
    if control_type == "dialogButton" and ctrl.dialogButton:
        if dialog_counter is None:
            dialog_counter = [0]
        factory_id = f"_dlgFactory{dialog_counter[0]}"
        dialog_counter[0] += 1

        parts: list[str] = []

        # If the control also has a param, render textfield + button in a RowLayout
        if ctrl.param:
            # Factory and component go first (siblings, not inside the row)
            comp_id = f"{factory_id}Component"
            parts.append(f"{indent}QGCPopupDialogFactory {{")
            parts.append(f"{indent}    id: {factory_id}")
            parts.append(f"{indent}    dialogComponent: {comp_id}")
            parts.append(f"{indent}}}")
            parts.append(f"{indent}Component {{")
            parts.append(f"{indent}    id: {comp_id}")
            parts.append(f"{indent}    {ctrl.dialogButton.dialogComponent} {{ }}")
            parts.append(f"{indent}}}")

            # RowLayout with textfield + button
            params_js = ", ".join(
                f'"{k}": {v}' for k, v in ctrl.dialogButton.dialogParams.items()
            )
            open_arg = f"{{ {params_js} }}" if params_js else ""

            ri = indent + "    "  # row-inner indent

            parts.append(f"{indent}RowLayout {{")
            parts.append(f"{indent}    Layout.fillWidth: true")
            parts.append(f"{indent}    spacing: ScreenTools.defaultFontPixelWidth")

            if not ctrl.dialogButton.buttonAfter:
                # Label (fill width), Button, plain FactTextField
                label_text = ctrl.label if ctrl.label else f"{fact_ref}.shortDescription"
                label_qtr = qml_tr(label_text, tr_context) if ctrl.label else label_text
                parts.append(f'{ri}QGCLabel {{')
                parts.append(f'{ri}    text: {label_qtr}')
                parts.append(f'{ri}    Layout.fillWidth: true')
                parts.append(f'{ri}}}')
                parts.append(f'{ri}QGCButton {{')
                parts.append(f'{ri}    text: {qml_tr(ctrl.dialogButton.text, tr_context)}')
                parts.append(f'{ri}    onClicked: {factory_id}.open({open_arg})')
                if ctrl.enableWhen:
                    parts.append(f'{ri}    enabled: {ctrl.enableWhen}')
                parts.append(f'{ri}}}')
                parts.append(f'{ri}FactTextField {{')
                parts.append(f'{ri}    fact: {fact_ref}')
                if ctrl.enableWhen:
                    parts.append(f'{ri}    enabled: {ctrl.enableWhen}')
                parts.append(f'{ri}}}')
            else:
                # LabelledFactTextField, then Button
                tf = render_textfield(
                    fact_ref, ri,
                    label=ctrl.label,
                    enable_when=ctrl.enableWhen,
                    label_source=f"{fact_ref}.shortDescription",
                    qml_type="LabelledFactTextField",
                    tr_context=tr_context,
                )
                parts.append(tf)
                parts.append(f'{ri}QGCButton {{')
                parts.append(f'{ri}    text: {qml_tr(ctrl.dialogButton.text, tr_context)}')
                parts.append(f'{ri}    onClicked: {factory_id}.open({open_arg})')
                if ctrl.enableWhen:
                    parts.append(f'{ri}    enabled: {ctrl.enableWhen}')
                parts.append(f'{ri}}}')

            parts.append(f"{indent}}}")

            qml = "\n".join(parts)
        else:
            qml = render_dialog_button(
                indent,
                dialog_button=ctrl.dialogButton,
                factory_id=factory_id,
                enable_when=ctrl.enableWhen,
                tr_context=tr_context,
            )

        if ctrl.showWhen:
            # Wrap in a ColumnLayout with visibility
            qml = f"{indent}ColumnLayout {{\n{indent}    visible: {ctrl.showWhen}\n" + \
                  "\n".join(f"    {l}" for l in qml.splitlines()) + \
                  f"\n{indent}}}"
        return _apply_indent(qml)

    # Action button — standalone button calling a controller method
    if control_type == "actionButton" and ctrl.actionButton:
        qml = render_action_button(
            indent,
            action_button=ctrl.actionButton,
            enable_when=ctrl.enableWhen,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # FactSlider — visual slider with optional description and linked params
    if control_type == "factslider":
        qml = render_factslider(
            fact_ref, indent,
            label=ctrl.label,
            description=ctrl.description,
            from_=ctrl.sliderFrom,
            to=ctrl.sliderTo,
            major_tick_step_size=ctrl.majorTickStepSize,
            decimal_places=ctrl.decimalPlaces,
            linked_params=ctrl.linkedParams if ctrl.linkedParams else None,
            show_when=ctrl.showWhen,
            enable_when=ctrl.enableWhen,
            tr_context=tr_context,
        )
        return _apply_indent(qml)

    # Slider — has its own label
    if control_type == "slider":
        qml = render_slider(
            fact_ref, indent,
            label=ctrl.label,
            enable_checkbox=ctrl.enableCheckbox,
            button=ctrl.button,
            enable_when=ctrl.enableWhen,
            allow_using_min_max=True,
            slider_min=ctrl.sliderMin,
            slider_max=ctrl.sliderMax,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Radio group — label + radio buttons
    if control_type == "radiogroup":
        qml = render_radiogroup(
            fact_ref, indent,
            label=ctrl.label,
            options=ctrl.options,
            enable_when=ctrl.enableWhen,
            raw=ctrl.raw,
            optional=ctrl.optional,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            # Wrap the label + column in a ColumnLayout with visibility
            inner_qml = render_radiogroup(
                fact_ref, indent + "    ",
                label=ctrl.label,
                options=ctrl.options,
                enable_when=ctrl.enableWhen,
                raw=ctrl.raw,
                optional=ctrl.optional,
                tr_context=tr_context,
            )
            qml = (
                f"{indent}ColumnLayout {{\n"
                f"{indent}    visible: {ctrl.showWhen}\n"
                f"{indent}    spacing: 0\n"
                f"{inner_qml}\n"
                f"{indent}}}"
            )
        return _apply_indent(qml)

    # Bitmask checkbox — FactBitMaskCheckBoxSlider
    if control_type == "bitmaskCheckbox":
        qml = render_bitmask_checkbox(
            fact_ref, indent,
            label=ctrl.label,
            bit_mask=ctrl.bitMask,
            enable_when=ctrl.enableWhen,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Bitmask — full FactBitmask widget
    if control_type == "bitmask":
        qml = render_bitmask(
            fact_ref, indent,
            first_entry_is_all=ctrl.firstEntryIsAll,
            enable_when=ctrl.enableWhen,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Toggle checkbox — QGCCheckBoxSlider with custom logic
    if control_type == "toggleCheckbox" and ctrl.toggleCheckbox:
        qml = render_toggle_checkbox(
            indent,
            label=ctrl.label,
            toggle=ctrl.toggleCheckbox,
            enable_when=ctrl.enableWhen,
            optional=ctrl.optional,
            fact_ref=fact_ref,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Checkbox — FactCheckBoxSlider includes label
    if control_type == "checkbox":
        qml = render_checkbox(
            fact_ref, indent,
            label=ctrl.label,
            enable_when=ctrl.enableWhen,
            label_source=f"{fact_ref}.shortDescription",
            qml_type="FactCheckBoxSlider",
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Combobox — LabelledFactComboBox includes label
    if control_type == "combobox":
        qml = render_combobox(
            fact_ref, indent,
            label=ctrl.label,
            enable_when=ctrl.enableWhen,
            label_source=f"{fact_ref}.shortDescription",
            qml_type="LabelledFactComboBox",
            combo_preferred_width="ScreenTools.defaultFontPixelWidth * 30",
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    # Textfield — LabelledFactTextField includes label
    qml = render_textfield(
        fact_ref, indent,
        label=ctrl.label,
        enable_when=ctrl.enableWhen,
        label_source=f"{fact_ref}.shortDescription",
        qml_type="LabelledFactTextField",
        tr_context=tr_context,
    )
    if ctrl.showWhen:
        qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
    return _apply_indent(qml)


def _inject_prop(qml: str, prop_line: str) -> str:
    """Insert a property line after the opening brace of a QML block."""
    qml_lines = qml.split("\n")
    qml_lines.insert(1, prop_line)
    return "\n".join(qml_lines)


def _wrap_visible(qml: str, expr: str, indent: str) -> str:
    """Wrap a QML block in an Item with a visible binding."""
    lines = [f"{indent}Item {{"]
    lines.append(f"{indent}    visible: {expr}")
    lines.append(f"{indent}    Layout.fillWidth: true")
    for line in qml.splitlines():
        lines.append(f"    {line}" if line.strip() else line)
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def _qml_generated_section(sec: SectionDef, sec_idx: int, tr_context: str = "") -> str:
    """Generate QML for a section with auto-generated controls."""
    ind = "                "  # base indent inside outerColumn
    lines: list[str] = []

    name_vis = f'sectionMatchesFilter("{sec.title}")'
    show_vis = f"{name_vis} && {sec.showWhen}" if sec.showWhen else name_vis

    lines.append(f'{ind}ConfigSection {{')
    lines.append(f'{ind}    Layout.fillWidth: true')
    lines.append(f'{ind}    visible: {show_vis}')
    lines.append(f'{ind}    heading: {qml_tr(sec.title, tr_context)}')
    if sec.image:
        lines.append(f'{ind}    iconSource: "{sec.image}"')

    ctrl_indent = ind + "    "
    for ctrl in sec.controls:
        lines.append("")
        lines.append(_qml_control(ctrl, ctrl_indent, tr_context=tr_context))

    lines.append(f'{ind}}}')

    return "\n".join(lines)


def _qml_component_section(sec: SectionDef, sec_idx: int, tr_context: str = "") -> str:
    """Generate QML for a component escape-hatch section."""
    ind = "                "
    lines: list[str] = []

    name_vis = f'sectionMatchesFilter("{sec.title}")'
    show_vis = f"{name_vis} && {sec.showWhen}" if sec.showWhen else name_vis

    lines.append(f'{ind}QGCLabel {{')
    lines.append(f'{ind}    text: {qml_tr(sec.title, tr_context)} + " ⚙"')
    lines.append(f'{ind}    visible: {show_vis}')
    lines.append(f'{ind}}}')

    # Wrap escape hatch in a Row with a colored left bar
    lines.append(f'{ind}Row {{')
    lines.append(f'{ind}    visible: {show_vis}')
    lines.append(f'{ind}    spacing: 0')
    lines.append(f'{ind}    Rectangle {{')
    lines.append(f'{ind}        width: 3')
    lines.append(f'{ind}        height: parent.height')
    lines.append(f'{ind}        color: "orange"')
    lines.append(f'{ind}    }}')
    lines.append(f'{ind}    {sec.component} {{')
    lines.append(f'{ind}        controller: controller')
    lines.append(f'{ind}    }}')
    lines.append(f'{ind}}}')

    return "\n".join(lines)


def _safe_id(name: str) -> str:
    """Convert a section name to a safe QML identifier."""
    return "".join(c if c.isalnum() else "_" for c in name).lower()


def _qml_repeat_count_property(sec: SectionDef) -> str:
    """Generate the count property for a repeat section (emitted at Item level)."""
    rep = sec.repeat
    assert rep is not None
    ind = "            "  # Item-level indent
    safe = _safe_id(sec.title)

    if rep.indexing == "apm_battery":
        # APM battery indexing: 0->"BATT_", 1->"BATT2_", ..., 8->"BATT9_", 9->"BATTA_", ...
        lines: list[str] = []
        lines.append(f"{ind}function _battPrefixForIndex(_i) {{")
        lines.append(f'{ind}    if (_i === 0) return "{rep.paramPrefix}_"')
        lines.append(f'{ind}    if (_i <= 8) return "{rep.paramPrefix}" + (_i + 1) + "_"')
        lines.append(f'{ind}    return "{rep.paramPrefix}" + String.fromCharCode(65 + _i - 9) + "_"')
        lines.append(f"{ind}}}")
        lines.append(f"{ind}function _battLabelForIndex(_i) {{")
        lines.append(f"{ind}    if (_i <= 8) return String(_i + 1)")
        lines.append(f"{ind}    return String.fromCharCode(65 + _i - 9)")
        lines.append(f"{ind}}}")
        lines.append(f"{ind}property int _{safe}Count: {{")
        lines.append(f"{ind}    var _i = 0")
        lines.append(f"{ind}    while (_i < 16) {{")
        lines.append(f'{ind}        if (!controller.parameterExists(-1, _battPrefixForIndex(_i) + "{rep.probePostfix}"))')
        lines.append(f"{ind}            return _i")
        lines.append(f"{ind}        _i++")
        lines.append(f"{ind}    }}")
        lines.append(f"{ind}    return 16")
        lines.append(f"{ind}}}")
        return "\n".join(lines)

    probe_param = f'"{rep.paramPrefix}" + _idx + "{rep.probePostfix}"'
    if rep.firstIndexOmitsNumber:
        idx_expr = f'(_i === {rep.startIndex}) ? "" : _i'
    else:
        idx_expr = "_i"

    lines: list[str] = []
    lines.append(f"{ind}property int _{safe}Count: {{")
    lines.append(f"{ind}    var _i = {rep.startIndex}")
    lines.append(f"{ind}    while (true) {{")
    lines.append(f"{ind}        var _idx = {idx_expr}")
    lines.append(f"{ind}        if (!controller.parameterExists(-1, {probe_param}))")
    lines.append(f"{ind}            return _i - {rep.startIndex}")
    lines.append(f"{ind}        _i++")
    lines.append(f"{ind}    }}")
    lines.append(f"{ind}}}")
    return "\n".join(lines)


def _qml_repeat_section(sec: SectionDef, sec_idx: int, tr_context: str = "") -> str:
    """Generate QML for a repeated (indexed) section using a Repeater."""
    rep = sec.repeat
    assert rep is not None
    ind = "                "  # base indent inside outerColumn

    name_vis = f'sectionMatchesFilter(heading)'
    show_vis = f"{name_vis} && {sec.showWhen}" if sec.showWhen else name_vis
    safe = _safe_id(sec.title)

    lines: list[str] = []

    # Repeater
    lines.append(f"{ind}Repeater {{")
    lines.append(f"{ind}    model: _{safe}Count")
    lines.append("")
    lines.append(f"{ind}    ConfigSection {{")
    lines.append(f"{ind}        Layout.fillWidth: true")
    if rep.enableParam:
        dv = rep.disabledParamValue
        enable_expr = f'controller.getParameterFact(-1, _fullParamName("{rep.enableParam}")).value !== {dv}'
        lines.append(f"{ind}        visible: {show_vis} && {enable_expr}")
    else:
        lines.append(f"{ind}        visible: {show_vis}")

    # heading — include index when count > 1
    heading_base = sec.title.replace("{index}", '" + _displayIndex + "')
    if "{index}" in sec.title:
        lines.append(f'{ind}        heading: {qml_tr(heading_base, tr_context)}')
    else:
        lines.append(f'{ind}        heading: _{safe}Count > 1 ? {qml_tr(sec.title, tr_context)} + " " + _displayIndex : {qml_tr(sec.title, tr_context)}')

    if sec.image:
        lines.append(f'{ind}        iconSource: "{sec.image}"')

    # Internal properties for index math
    lines.append("")
    if rep.indexing == "apm_battery":
        lines.append(f"{ind}        property int _rawIndex: index")
        lines.append(f"{ind}        property string _prefix: _battPrefixForIndex(_rawIndex)")
        lines.append(f"{ind}        property string _displayIndex: _battLabelForIndex(_rawIndex)")
        lines.append(f'{ind}        function _fullParamName(postfix) {{ return _prefix + postfix }}')
    else:
        lines.append(f"{ind}        property int _rawIndex: index + {rep.startIndex}")
        if rep.firstIndexOmitsNumber:
            lines.append(f'{ind}        property string _indexStr: (_rawIndex === {rep.startIndex}) ? "" : String(_rawIndex)')
        else:
            lines.append(f"{ind}        property string _indexStr: String(_rawIndex)")
        lines.append(f'{ind}        property string _displayIndex: String(_rawIndex)')
        lines.append(f'{ind}        function _fullParamName(postfix) {{ return "{rep.paramPrefix}" + _indexStr + postfix }}')

    ctrl_indent = ind + "        "
    dialog_counter = [0]
    for ctrl in sec.controls:
        lines.append("")
        lines.append(_qml_control(ctrl, ctrl_indent, indexed=True, dialog_counter=dialog_counter, tr_context=tr_context))

    lines.append(f"{ind}    }}")
    lines.append(f"{ind}}}")

    # Companion section for disabled items
    if rep.enableParam and rep.disabledSection:
        lines.append("")
        lines.append(_qml_disabled_companion_section(sec, tr_context=tr_context))

    return "\n".join(lines)


def _qml_disabled_companion_section(sec: SectionDef, tr_context: str = "") -> str:
    """Emit a compact section showing only the enable-param control for disabled items."""
    rep = sec.repeat
    assert rep is not None
    ind = "                "  # base indent inside outerColumn
    safe = _safe_id(sec.title)
    dv = rep.disabledParamValue

    # camelCase version of safe id for use in property names
    camel = "".join(w.capitalize() for w in safe.split("_") if w)

    lines: list[str] = []
    lines.append(f"{ind}ConfigSection {{")
    lines.append(f"{ind}    Layout.fillWidth: true")

    # Visible when sectionNameFilter allows it AND any item is disabled
    lines.append(f'{ind}    visible: sectionMatchesFilter("{rep.disabledSection.heading}") && _hasDisabled{camel}')
    lines.append(f"{ind}    property bool _hasDisabled{camel}: {{")
    lines.append(f"{ind}        for (var i = 0; i < _{safe}Count; i++) {{")
    if rep.indexing == "apm_battery":
        lines.append(f'{ind}            if (controller.getParameterFact(-1, _battPrefixForIndex(i) + "{rep.enableParam}").value === {dv})')
    else:
        prefix_expr = f'"{rep.paramPrefix}"'
        if rep.firstIndexOmitsNumber:
            lines.append(f"{ind}            var idx = (i + {rep.startIndex}) === {rep.startIndex} ? \"\" : String(i + {rep.startIndex})")
        else:
            lines.append(f"{ind}            var idx = String(i + {rep.startIndex})")
        lines.append(f'{ind}            if (controller.getParameterFact(-1, {prefix_expr} + idx + "{rep.enableParam}").value === {dv})')
    lines.append(f"{ind}                return true")
    lines.append(f"{ind}        }}")
    lines.append(f"{ind}        return false")
    lines.append(f"{ind}    }}")

    lines.append(f'{ind}    heading: {qml_tr(rep.disabledSection.heading, tr_context)}')

    # Repeater with one control per disabled item
    lines.append(f"")
    lines.append(f"{ind}    Repeater {{")
    lines.append(f"{ind}        model: _{safe}Count")
    lines.append(f"")
    if rep.disabledSection.enabledParamValue:
        # FactCheckBoxSlider: toggling on sets enabledParamValue, toggling off sets 0
        lines.append(f"{ind}        FactCheckBoxSlider {{")

        if rep.indexing == "apm_battery":
            fact_expr = f'controller.getParameterFact(-1, _battPrefixForIndex(index) + "{rep.enableParam}")'
            label_expr = f'_{safe}Count > 1 ? {qml_tr(sec.title, tr_context)} + " " + _battLabelForIndex(index) : {qml_tr(sec.title, tr_context)}'
        else:
            if rep.firstIndexOmitsNumber:
                lines.append(f"{ind}            property string _idx: (index + {rep.startIndex}) === {rep.startIndex} ? \"\" : String(index + {rep.startIndex})")
            else:
                lines.append(f"{ind}            property string _idx: String(index + {rep.startIndex})")
            fact_expr = f'controller.getParameterFact(-1, "{rep.paramPrefix}" + _idx + "{rep.enableParam}")'
            label_expr = f'_{safe}Count > 1 ? {qml_tr(sec.title, tr_context)} + " " + String(index + {rep.startIndex}) : {qml_tr(sec.title, tr_context)}'

        lines.append(f"{ind}            visible: {fact_expr}.value === {dv}")
        lines.append(f"{ind}            text: {label_expr}")
        lines.append(f"{ind}            fact: {fact_expr}")
        lines.append(f"{ind}            checkedValue: {rep.disabledSection.enabledParamValue}")
        lines.append(f"{ind}            uncheckedValue: {dv}")
        lines.append(f"{ind}            Layout.fillWidth: true")
        lines.append(f"{ind}        }}")
    else:
        # LabelledFactComboBox: let user pick any value
        lines.append(f"{ind}        LabelledFactComboBox {{")

        if rep.indexing == "apm_battery":
            lines.append(f'{ind}            visible: controller.getParameterFact(-1, _battPrefixForIndex(index) + "{rep.enableParam}").value === {dv}')
            lines.append(f'{ind}            label: _{safe}Count > 1 ? {qml_tr(sec.title, tr_context)} + " " + _battLabelForIndex(index) : {qml_tr(sec.title, tr_context)}')
            lines.append(f'{ind}            fact: controller.getParameterFact(-1, _battPrefixForIndex(index) + "{rep.enableParam}")')
        else:
            if rep.firstIndexOmitsNumber:
                lines.append(f"{ind}            property string _idx: (index + {rep.startIndex}) === {rep.startIndex} ? \"\" : String(index + {rep.startIndex})")
            else:
                lines.append(f"{ind}            property string _idx: String(index + {rep.startIndex})")
            lines.append(f'{ind}            visible: controller.getParameterFact(-1, "{rep.paramPrefix}" + _idx + "{rep.enableParam}").value === {dv}')
            lines.append(f'{ind}            label: _{safe}Count > 1 ? {qml_tr(sec.title, tr_context)} + " " + String(index + {rep.startIndex}) : {qml_tr(sec.title, tr_context)}')
            lines.append(f'{ind}            fact: controller.getParameterFact(-1, "{rep.paramPrefix}" + _idx + "{rep.enableParam}")')

        lines.append(f"{ind}            Layout.fillWidth: true")
        lines.append(f"{ind}            comboBoxPreferredWidth: ScreenTools.defaultFontPixelWidth * 30")
        lines.append(f"{ind}            indexModel: false")
        lines.append(f"{ind}        }}")
    lines.append(f"{ind}    }}")
    lines.append(f"{ind}}}")

    return "\n".join(lines)


# --------------------------------------------------------------------------- #
# Public API
# --------------------------------------------------------------------------- #

def generate_config_page_qml(page: PageDef) -> str:
    """Generate a complete QML file for a vehicle config page."""
    _propagate_optional(page)
    lines: list[str] = [_HEADER]
    for imp in page.imports:
        lines.append(f"import {imp}")
    if page.imports:
        lines.append("")
    lines.append("SetupPage {")
    lines.append("    id: configPage")
    lines.append("    pageComponent: pageComponent")
    lines.append("")
    lines.append("    Component {")
    lines.append("        id: pageComponent")
    lines.append("")
    lines.append("        Item {")
    lines.append("            width: Math.max(availableWidth, outerColumn.width)")
    lines.append("            height: outerColumn.height")
    lines.append("")
    lines.append(f"            {page.controllerType} {{")
    lines.append("                id: controller")
    lines.append("            }")
    lines.append("")
    lines.append("            property real _margins: ScreenTools.defaultFontPixelHeight")
    lines.append("")

    # Emit page-level constants as readonly properties
    for name, expr in page.constants.items():
        lines.append(f"            readonly property var {name}: {expr}")
    if page.constants:
        lines.append("")

    # Emit page-level param lookups
    for name, p in page.params.items():
        if p.existsOnly:
            lines.append(f'            property var {name}: controller.parameterExists(-1, "{p.name}")')
        elif p.required:
            lines.append(f'            property var {name}: controller.getParameterFact(-1, "{p.name}")')
        else:
            lines.append(f'            property var {name}: controller.getParameterFact(-1, "{p.name}", false)')
    if page.params:
        lines.append("")

    # Emit page-level bindings as QML properties
    for name, expr in page.bindings.items():
        lines.append(f"            property var {name}: {expr}")
    if page.bindings:
        lines.append("")

    # Emit repeat count properties at Item level (visible to Repeater delegates)
    for sec in page.sections:
        if sec.repeat:
            lines.append(_qml_repeat_count_property(sec))
            lines.append("")


    lines.append("            property string sectionNameFilter: \"\"")
    lines.append("")

    # Build search terms index for keyword matching
    search_terms = _build_search_terms(page)
    lines.append("            readonly property var _searchTerms: ({")
    for i, (title, terms) in enumerate(search_terms.items()):
        terms_str = ", ".join(f'"{t}"' for t in terms)
        comma = "," if i < len(search_terms) - 1 else ""
        lines.append(f'                "{title}": [{terms_str}]{comma}')
    lines.append("            })")
    lines.append("")

    # Translatable terms — original-case strings run through qsTranslate() at search time
    tr_terms = _build_translatable_terms(page)
    tr_ctx = page.json_filename
    lines.append("            readonly property var _translatableSearchTerms: ({")
    for i, (title, terms) in enumerate(tr_terms.items()):
        terms_str = ", ".join(f'"{t}"' for t in terms)
        comma = "," if i < len(tr_terms) - 1 else ""
        lines.append(f'                "{title}": [{terms_str}]{comma}')
    lines.append("            })")
    lines.append("")

    lines.append("            function sectionMatchesFilter(sectionTitle) {")
    lines.append("                if (sectionNameFilter === \"\") return true")
    lines.append("                if (sectionNameFilter === sectionTitle) return true")
    lines.append("                var filter = sectionNameFilter.toLowerCase()")
    lines.append("                var terms = _searchTerms[sectionTitle]")
    lines.append("                if (terms) {")
    lines.append("                    for (var i = 0; i < terms.length; i++) {")
    lines.append("                        if (terms[i].indexOf(filter) >= 0) return true")
    lines.append("                    }")
    lines.append("                }")
    lines.append("                var trTerms = _translatableSearchTerms[sectionTitle]")
    lines.append("                if (trTerms) {")
    lines.append("                    for (var j = 0; j < trTerms.length; j++) {")
    lines.append(f'                        if (qsTranslate("{tr_ctx}", trTerms[j]).toLowerCase().indexOf(filter) >= 0) return true')
    lines.append("                    }")
    lines.append("                }")
    lines.append("                return false")
    lines.append("            }")
    lines.append("")

    lines.append("            function sectionVisible(name) {")
    # Build a switch that returns visibility per section.
    # Merge showWhen conditions for sections that share the same name
    # so that the section is visible when ANY variant matches.
    visible_conditions: dict[str, list[str]] = {}
    for sec in page.sections:
        if sec.showWhen:
            visible_conditions.setdefault(sec.title, []).append(sec.showWhen)
    for name, conditions in visible_conditions.items():
        merged = " || ".join(dict.fromkeys(conditions))  # deduplicate, preserve order
        lines.append(f'                if (name === "{name}") return {merged}')
    lines.append("                return true")
    lines.append("            }")
    lines.append("")
    lines.append("            property real _maxLeftMargin: ScreenTools.defaultFontPixelWidth * 20")
    lines.append("")
    lines.append("            ColumnLayout {")
    lines.append("                id: outerColumn")
    lines.append("                spacing: _margins * 1.25")
    lines.append("                anchors.left: parent.left")
    lines.append("                anchors.leftMargin: Math.min((parent.width - width) / 2, _maxLeftMargin)")

    tr_ctx = page.json_filename
    for sec_idx, sec in enumerate(page.sections):
        lines.append("")
        if sec.repeat:
            lines.append(_qml_repeat_section(sec, sec_idx, tr_context=tr_ctx))
        elif sec.component:
            lines.append(_qml_component_section(sec, sec_idx, tr_context=tr_ctx))
        else:
            lines.append(_qml_generated_section(sec, sec_idx, tr_context=tr_ctx))

    lines.append("            }")
    lines.append("        }")
    lines.append("    }")
    lines.append("}")
    lines.append("")

    return "\n".join(lines)


def get_section_names(page: PageDef) -> list[str]:
    """Return the display names of all sections in a page."""
    names = [sec.title for sec in page.sections if sec.title]
    for sec in page.sections:
        if sec.repeat and sec.repeat.disabledSection:
            names.append(sec.repeat.disabledSection.heading)
    return names
