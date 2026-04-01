"""Shared control rendering for QML generators.

Both the settings and config page generators produce QML for the same set
of Fact-bound control types (slider, checkbox, combobox, textfield, etc.).
This module contains the portable rendering functions so neither generator
duplicates the logic.

Each ``render_*`` function takes a *fact_ref* QML expression (the caller
decides whether it comes from ``QGroundControl.settingsManager.…`` or
``controller.getParameterFact(…)``), structural parameters from the JSON
definition, and an *indent* string.  It returns a ready-to-insert block of
QML lines.
"""

from __future__ import annotations

from dataclasses import dataclass, field


# --------------------------------------------------------------------------- #
# Shared data fragments — callers compose these into their own ControlDef
# --------------------------------------------------------------------------- #

@dataclass
class EnableCheckboxDef:
    """Optional enable/disable checkbox on a slider control."""
    checked: str = ""      # QML expression for checkbox state
    onClicked: str = ""    # QML expression executed when toggled


@dataclass
class ButtonDef:
    """Optional button adjacent to a control."""
    text: str = ""
    onClicked: str = ""
    enabled: str = ""


@dataclass
class RadioOptionDef:
    """A single option in a radio button group."""
    label: str = ""
    value: str = ""      # QML value to set when clicked
    checked: str = ""    # QML expression for checked state


@dataclass
class DialogButtonDef:
    """A button that opens a QGCPopupDialog from a hand-written QML component."""
    text: str = ""
    dialogComponent: str = ""      # QML type name, e.g. "CalcVoltageDividerDialog"
    dialogParams: dict[str, str] = field(default_factory=dict)  # key -> QML expression
    buttonAfter: bool = True        # when False and paired with a param, button appears before textfield


@dataclass
class ActionButtonDef:
    """A standalone button that calls a controller method."""
    text: str = ""
    onClicked: str = ""


@dataclass
class LinkedParamDef:
    """A parameter whose value is updated when a FactSlider changes."""
    param: str = ""          # vehicle parameter name
    expression: str = ""     # JS expression using ``value`` (e.g. "value", "value * 2")


@dataclass
class ToggleCheckboxDef:
    """A non-Fact checkbox with custom checked/onClicked logic."""
    checked: str = ""        # QML expression for checkbox state
    onChecked: str = ""      # QML statement when checked
    onUnchecked: str = ""    # QML statement when unchecked


# --------------------------------------------------------------------------- #
# Rendering helpers
# --------------------------------------------------------------------------- #

def qml_tr(text: str, context: str = "") -> str:
    """Return a QML translation call for *text*.

    When *context* is non-empty the result is ``qsTranslate("ctx", "text")``
    which uses an explicit translation context (the JSON filename).  When empty
    the result is the standard ``qsTr("text")`` which derives context from the
    QML filename.
    """
    if context:
        return f'qsTranslate("{context}", "{text}")'
    return f'qsTr("{text}")'


def render_label(
    indent: str,
    *,
    text: str = "",
    warning: bool = False,
    small_font: bool = False,
    tr_context: str = "",
) -> str:
    """Render a static ``QGCLabel`` (no fact binding)."""
    lines = [f"{indent}QGCLabel {{"]
    lines.append(f'{indent}    text: {qml_tr(text, tr_context)}')
    lines.append(f"{indent}    wrapMode: Text.WordWrap")
    lines.append(f"{indent}    Layout.fillWidth: true")
    lines.append(f"{indent}    Layout.preferredWidth: 0")
    if small_font:
        lines.append(f"{indent}    font.pointSize: ScreenTools.smallFontPointSize")
    if warning:
        lines.append(f"{indent}    color: qgcPal.warningText")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def render_slider(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    enable_checkbox: EnableCheckboxDef | None = None,
    button: ButtonDef | None = None,
    enable_when: str = "",
    allow_using_min_max: bool = False,
    slider_min: str = "",
    slider_max: str = "",
    tr_context: str = "",
) -> str:
    """Render a ``FactTextFieldSlider``, optionally with enable checkbox and button."""
    label_line = f'    label: {qml_tr(label, tr_context)}' if label else "    label: fact.label"
    inner_indent = indent
    has_button = button is not None and button.text
    if has_button:
        inner_indent = indent + "    "

    lines: list[str] = []
    if has_button:
        lines.append(f"{indent}RowLayout {{")
        lines.append(f"{indent}    Layout.fillWidth: true")
        lines.append(f"{indent}    spacing: ScreenTools.defaultFontPixelWidth")

    lines.append(f"{inner_indent}FactTextFieldSlider {{")
    lines.append(f"{inner_indent}    Layout.fillWidth: true")
    lines.append(f"{inner_indent}{label_line}")
    lines.append(f"{inner_indent}    fact: {fact_ref}")

    if allow_using_min_max:
        lines.append(f"{inner_indent}    allowUsingMinMax: true")

    if slider_min:
        lines.append(f"{inner_indent}    sliderMin: {slider_min}")
    if slider_max:
        lines.append(f"{inner_indent}    sliderMax: {slider_max}")

    if enable_checkbox:
        lines.append(f"{inner_indent}    showEnableCheckbox: true")
        if enable_checkbox.checked:
            lines.append(f"{inner_indent}    enableCheckBoxChecked: {enable_checkbox.checked}")
        if enable_checkbox.onClicked:
            lines.append(f"{inner_indent}    onEnableCheckboxClicked: {enable_checkbox.onClicked}")

    if enable_when:
        lines.append(f"{inner_indent}    enabled: {enable_when}")

    lines.append(f"{inner_indent}}}")

    if has_button:
        assert button is not None
        lines.append(f'{inner_indent}QGCButton {{')
        lines.append(f'{inner_indent}    text: {qml_tr(button.text, tr_context)}')
        lines.append(f'{inner_indent}    onClicked: {button.onClicked}')
        if button.enabled:
            lines.append(f'{inner_indent}    enabled: {button.enabled}')
        lines.append(f'{inner_indent}}}')
        lines.append(f"{indent}}}")

    return "\n".join(lines)


def render_checkbox(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    enable_when: str = "",
    label_property: str = "text",
    label_source: str = "fact.label",
    qml_type: str = "FactCheckBox",
    tr_context: str = "",
) -> str:
    """Render a ``FactCheckBox`` (or variant like ``FactCheckBoxSlider``)."""
    label_line = f'    {label_property}: {qml_tr(label, tr_context)}' if label else f"    {label_property}: {label_source}"
    lines = [
        f"{indent}{qml_type} {{",
        f"{indent}    Layout.fillWidth: true",
        f"{indent}{label_line}",
        f"{indent}    fact: {fact_ref}",
    ]
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def render_combobox(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    enable_when: str = "",
    label_source: str = "fact.label",
    qml_type: str = "FactComboBox",
    combo_preferred_width: str = "",
    tr_context: str = "",
) -> str:
    """Render a ``FactComboBox`` (or ``LabelledFactComboBox``)."""
    if qml_type == "LabelledFactComboBox":
        label_line = f'    label: {qml_tr(label, tr_context)}' if label else f"    label: {label_source}"
    else:
        label_line = None

    lines = [f"{indent}{qml_type} {{"]
    if label_line:
        lines.append(f"{indent}{label_line}")
    lines.append(f"{indent}    Layout.fillWidth: true")
    if combo_preferred_width:
        lines.append(f"{indent}    comboBoxPreferredWidth: {combo_preferred_width}")
    lines.append(f"{indent}    fact: {fact_ref}")
    lines.append(f"{indent}    indexModel: false")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def render_textfield(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    enable_when: str = "",
    placeholder: str = "",
    label_source: str = "fact.label",
    qml_type: str = "FactTextField",
    extra_lines: list[str] | None = None,
    tr_context: str = "",
) -> str:
    """Render a ``FactTextField`` (or ``LabelledFactTextField``)."""
    if qml_type == "LabelledFactTextField":
        label_line = f'    label: {qml_tr(label, tr_context)}' if label else f"    label: {label_source}"
    else:
        label_line = None

    lines = [f"{indent}{qml_type} {{"]
    if label_line:
        lines.append(f"{indent}{label_line}")
    lines.append(f"{indent}    Layout.fillWidth: true")
    lines.append(f"{indent}    fact: {fact_ref}")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    if placeholder:
        lines.append(f'{indent}    textField.placeholderText: {qml_tr(placeholder, tr_context)}')
    if extra_lines:
        for el in extra_lines:
            lines.append(f"{indent}    {el}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def parse_enable_checkbox(data: dict) -> EnableCheckboxDef | None:
    """Parse an enableCheckbox dict from JSON into an EnableCheckboxDef."""
    if not data:
        return None
    return EnableCheckboxDef(
        checked=data.get("checked", ""),
        onClicked=data.get("onClicked", ""),
    )


def parse_button(data: dict) -> ButtonDef | None:
    """Parse a button dict from JSON into a ButtonDef."""
    if not data:
        return None
    return ButtonDef(
        text=data.get("text", ""),
        onClicked=data.get("onClicked", ""),
        enabled=data.get("enabled", ""),
    )


def parse_radio_options(data: list | None) -> list[RadioOptionDef]:
    """Parse a list of radio option dicts from JSON."""
    if not data:
        return []
    return [
        RadioOptionDef(
            label=opt.get("label", ""),
            value=str(opt.get("value", "")),
            checked=opt.get("checked", ""),
        )
        for opt in data
    ]


def render_radiogroup(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    options: list[RadioOptionDef],
    enable_when: str = "",
    raw: bool = False,
    optional: bool = False,
    tr_context: str = "",
) -> str:
    """Render a group of ``QGCRadioButton`` controls with an optional label."""
    value_prop = "rawValue" if raw else "value"
    lines: list[str] = []
    if label:
        lines.append(f'{indent}QGCLabel {{')
        lines.append(f'{indent}    text: {qml_tr(label, tr_context)}')
        lines.append(f'{indent}}}')
    inner = indent + "    "
    lines.append(f'{indent}ColumnLayout {{')
    lines.append(f'{indent}    spacing: 0')
    for opt in options:
        lines.append(f'{inner}QGCRadioButton {{')
        lines.append(f'{inner}    text: {qml_tr(opt.label, tr_context)}')
        if opt.checked:
            if optional:
                lines.append(f'{inner}    checked: {fact_ref} ? {opt.checked} : false')
            else:
                lines.append(f'{inner}    checked: {opt.checked}')
        if optional:
            lines.append(f'{inner}    onClicked: if ({fact_ref}) {{ {fact_ref}.{value_prop} = {opt.value} }}')
        else:
            lines.append(f'{inner}    onClicked: {fact_ref}.{value_prop} = {opt.value}')
        if enable_when:
            lines.append(f'{inner}    enabled: {enable_when}')
        lines.append(f'{inner}}}')
    lines.append(f'{indent}}}')
    return "\n".join(lines)


def parse_dialog_button(data: dict | None) -> DialogButtonDef | None:
    """Parse a dialogButton dict from JSON into a DialogButtonDef."""
    if not data:
        return None
    return DialogButtonDef(
        text=data.get("text", ""),
        dialogComponent=data.get("dialogComponent", ""),
        dialogParams=data.get("dialogParams", {}),
        buttonAfter=data.get("buttonAfter", True),
    )


def parse_action_button(data: dict | None) -> ActionButtonDef | None:
    """Parse an actionButton dict from JSON into an ActionButtonDef."""
    if not data:
        return None
    return ActionButtonDef(
        text=data.get("text", ""),
        onClicked=data.get("onClicked", ""),
    )


def render_dialog_button(
    indent: str,
    *,
    dialog_button: DialogButtonDef,
    factory_id: str,
    enable_when: str = "",
    tr_context: str = "",
) -> str:
    """Render a QGCPopupDialogFactory + Component wrapper + QGCButton that opens a dialog."""
    comp_id = f"{factory_id}Component"
    lines: list[str] = []

    # Factory
    lines.append(f"{indent}QGCPopupDialogFactory {{")
    lines.append(f"{indent}    id: {factory_id}")
    lines.append(f"{indent}    dialogComponent: {comp_id}")
    lines.append(f"{indent}}}")

    # Component wrapper
    lines.append(f"{indent}Component {{")
    lines.append(f"{indent}    id: {comp_id}")
    lines.append(f"{indent}    {dialog_button.dialogComponent} {{ }}")
    lines.append(f"{indent}}}")

    # Button
    params_js = ", ".join(
        f'"{k}": {v}' for k, v in dialog_button.dialogParams.items()
    )
    open_arg = f"{{ {params_js} }}" if params_js else ""

    lines.append(f"{indent}QGCButton {{")
    lines.append(f'{indent}    text: {qml_tr(dialog_button.text, tr_context)}')
    lines.append(f"{indent}    onClicked: {factory_id}.open({open_arg})")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")

    return "\n".join(lines)


def render_action_button(
    indent: str,
    *,
    action_button: ActionButtonDef,
    enable_when: str = "",
    tr_context: str = "",
) -> str:
    """Render a standalone ``QGCButton`` that calls a controller method."""
    lines: list[str] = []
    lines.append(f"{indent}QGCButton {{")
    lines.append(f'{indent}    text: {qml_tr(action_button.text, tr_context)}')
    lines.append(f"{indent}    onClicked: {action_button.onClicked}")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def render_bitmask_checkbox(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    bit_mask: int = 0,
    enable_when: str = "",
    tr_context: str = "",
) -> str:
    """Render a ``FactBitMaskCheckBoxSlider``."""
    lines = [f"{indent}FactBitMaskCheckBoxSlider {{"]
    lines.append(f"{indent}    Layout.fillWidth: true")
    if label:
        lines.append(f'{indent}    text: {qml_tr(label, tr_context)}')
    lines.append(f"{indent}    fact: {fact_ref}")
    lines.append(f"{indent}    bitMask: {bit_mask}")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def render_bitmask(
    fact_ref: str,
    indent: str,
    *,
    first_entry_is_all: bool = False,
    enable_when: str = "",
) -> str:
    """Render a ``FactBitmask`` widget."""
    lines = [f"{indent}FactBitmask {{"]
    lines.append(f"{indent}    fact: {fact_ref}")
    if first_entry_is_all:
        lines.append(f"{indent}    firstEntryIsAll: true")
    lines.append(f"{indent}    Layout.preferredWidth: 0")
    lines.append(f"{indent}    Layout.fillWidth: true")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def render_toggle_checkbox(
    indent: str,
    *,
    label: str = "",
    toggle: ToggleCheckboxDef,
    enable_when: str = "",
    optional: bool = False,
    fact_ref: str = "",
    tr_context: str = "",
) -> str:
    """Render a ``QGCCheckBoxSlider`` with custom checked/onClicked logic."""
    lines = [f"{indent}QGCCheckBoxSlider {{"]
    lines.append(f"{indent}    Layout.fillWidth: true")
    if label:
        lines.append(f'{indent}    text: {qml_tr(label, tr_context)}')
    if toggle.checked:
        if optional and fact_ref:
            lines.append(f"{indent}    checked: {fact_ref} ? {toggle.checked} : false")
        else:
            lines.append(f"{indent}    checked: {toggle.checked}")
    on_parts = []
    if toggle.onChecked and toggle.onUnchecked:
        on_parts.append(f"if (checked) {{ {toggle.onChecked} }} else {{ {toggle.onUnchecked} }}")
    elif toggle.onChecked:
        on_parts.append(f"if (checked) {{ {toggle.onChecked} }}")
    elif toggle.onUnchecked:
        on_parts.append(f"if (!checked) {{ {toggle.onUnchecked} }}")
    if on_parts:
        if optional and fact_ref:
            lines.append(f"{indent}    onClicked: if ({fact_ref}) {{ {on_parts[0]} }}")
        else:
            lines.append(f"{indent}    onClicked: {on_parts[0]}")
    if enable_when:
        lines.append(f"{indent}    enabled: {enable_when}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)


def parse_toggle_checkbox(data: dict | None) -> ToggleCheckboxDef | None:
    """Parse a toggleCheckbox dict from JSON."""
    if not data:
        return None
    return ToggleCheckboxDef(
        checked=data.get("checked", ""),
        onChecked=data.get("onChecked", ""),
        onUnchecked=data.get("onUnchecked", ""),
    )


def parse_linked_params(data: dict | None) -> list[LinkedParamDef]:
    """Parse a linkedParams dict from JSON.

    Input is ``{"PARAM_NAME": "expression", ...}``.
    """
    if not data:
        return []
    return [
        LinkedParamDef(param=name, expression=expr)
        for name, expr in data.items()
    ]


def render_factslider(
    fact_ref: str,
    indent: str,
    *,
    label: str = "",
    description: str = "",
    from_: str = "",
    to: str = "",
    major_tick_step_size: str = "",
    decimal_places: str = "",
    linked_params: list[LinkedParamDef] | None = None,
    show_when: str = "",
    enable_when: str = "",
    tr_context: str = "",
) -> str:
    """Render a ``FactSlider`` wrapped in a ``SettingsGroupLayout``."""
    lines: list[str] = []
    inner = indent + "    "

    # Wrap in SettingsGroupLayout for consistent bordered look
    lines.append(f"{indent}SettingsGroupLayout {{")
    lines.append(f"{indent}    Layout.fillWidth: true")
    lines.append(f"{indent}    Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 60")
    if label:
        lines.append(f"{indent}    heading: {qml_tr(label, tr_context)}")
    if description:
        lines.append(f"{indent}    headingDescription: {qml_tr(description, tr_context)}")
    if show_when:
        lines.append(f"{indent}    visible: {show_when}")

    lines.append(f"")
    lines.append(f"{inner}FactSlider {{")
    lines.append(f"{inner}    Layout.fillWidth: true")
    lines.append(f"{inner}    fact: {fact_ref}")

    if from_:
        lines.append(f"{inner}    from: {from_}")
    if to:
        lines.append(f"{inner}    to: {to}")
    if major_tick_step_size:
        lines.append(f"{inner}    majorTickStepSize: {major_tick_step_size}")
    else:
        lines.append(f"{inner}    majorTickStepSize: {fact_ref}.increment")
    if decimal_places:
        lines.append(f"{inner}    decimalPlaces: {decimal_places}")

    if linked_params:
        lines.append(f"{inner}    onValueChanged: {{")
        for lp in linked_params:
            lines.append(f'{inner}        controller.getParameterFact(-1, "{lp.param}").rawValue = {lp.expression}')
        lines.append(f"{inner}    }}")

    if enable_when:
        lines.append(f"{inner}    enabled: {enable_when}")

    lines.append(f"{inner}}}")
    lines.append(f"{indent}}}")
    return "\n".join(lines)
